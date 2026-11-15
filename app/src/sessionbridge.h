// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef SESSIONBRIDGE_H
#define SESSIONBRIDGE_H

#include <QDateTime>
#include <QHash>
#include <QScopedPointer>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "engine.h"
#include "model.h"
#include "polledbridge.h"
#include "sessioncontrols.h"
#include "sessionlog.h"
#include "waitcue.h"

class Services;
class SpeechBridge;
class SpeechRun;

// Runs a workout with the session engine of the core (R-TMR-1 to R-TMR-11) for QML and writes the
// session log (R-HST-1, R-HST-2). Exposed as the context property "sessionBridge", created in
// main() with the Services. It is silent: it reports the events, with their scheduled times, for
// the sounds, the pulse and the vibration of the session page (R-ALR-12).
//
// The poll (33 ms, PolledBridge) runs only while time runs (lead-in, a running step, a repetition
// step that waits for "done") and the app is active. Every time is read from the clock (R-TMR-5).
// Pausing when the app becomes inactive is the page's job (R-TMR-8, R-TMR-9). The workout is read
// once at the start; the saved workout is never changed (R-TMR-10).
class SessionBridge : public PolledBridge
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    // A session is on: lead-in, running, waiting for a tap or for "done", or paused.
    Q_PROPERTY(bool inProgress READ inProgress NOTIFY stateChanged)
    // Time passes: the session is not paused, waiting for a tap, finished or idle.
    Q_PROPERTY(bool running READ isRunning NOTIFY stateChanged)
    // The session can be paused now (everything in progress that is not paused already).
    Q_PROPERTY(bool isPaused READ isPaused NOTIFY stateChanged)
    Q_PROPERTY(bool isFinished READ isFinished NOTIFY stateChanged)
    Q_PROPERTY(bool pausable READ pausable NOTIFY stateChanged)
    Q_PROPERTY(QString workoutName READ workoutName NOTIFY stateChanged)
    // Lead-in length in seconds and the seconds at which a tick is reported (R-ALR-1); set before start().
    Q_PROPERTY(int leadInSec READ leadInSec WRITE setLeadInSec)
    Q_PROPERTY(QVariantList tickSeconds READ tickSeconds WRITE setTickSeconds)

    // Position (R-TMR-6).
    Q_PROPERTY(int stepNumber READ stepNumber NOTIFY stepChanged)
    Q_PROPERTY(int stepCount READ stepCount NOTIFY stepChanged)
    Q_PROPERTY(int blockNumber READ blockNumber NOTIFY stepChanged)
    Q_PROPERTY(int blockCount READ blockCount NOTIFY stepChanged)
    Q_PROPERTY(int roundNumber READ roundNumber NOTIFY stepChanged)
    Q_PROPERTY(int roundCount READ roundCount NOTIFY stepChanged)
    // The current and the next step: {type, exerciseKey, exerciseName, icon, colour, ending
    // ("duration", "reps" or "tempo"), durationSec, reps, startOnTap, insertedRest}; empty for no next step.
    Q_PROPERTY(QVariantMap current READ current NOTIFY stepChanged)
    Q_PROPERTY(QVariantMap next READ next NOTIFY stepChanged)
    // The exercise whose file the session offers (R-TMR-12): that of the current step, or of the
    // next one during a rest; empty for none.
    Q_PROPERTY(QString exerciseFileKey READ exerciseFileKey NOTIFY stepChanged)
    // The current step waits for a tap (R-WRK-11); true also while the session is paused there.
    Q_PROPERTY(bool waitingToStart READ waitingToStart NOTIFY stepChanged)
    // "leadin", "waiting", "timed", "reps" or "tempo": what the page shows.
    Q_PROPERTY(QString phaseKind READ phaseKind NOTIFY phaseKindChanged)

    // Times, in milliseconds, new at every poll.
    Q_PROPERTY(qint64 stepRemainingMs READ stepRemainingMs NOTIFY timeChanged)
    Q_PROPERTY(qint64 stepElapsedMs READ stepElapsedMs NOTIFY timeChanged)
    Q_PROPERTY(qint64 stepPlannedMs READ stepPlannedMs NOTIFY stepChanged)
    Q_PROPERTY(qint64 leadInRemainingMs READ leadInRemainingMs NOTIFY timeChanged)
    Q_PROPERTY(qint64 workoutRemainingMs READ workoutRemainingMs NOTIFY timeChanged)
    // Some later step has no planned time (repetitions without a tempo, waits), so the remaining
    // time counts the timed steps only.
    Q_PROPERTY(bool workoutRemainingOpen READ workoutRemainingOpen NOTIFY timeChanged)
    // The time of the session so far, without lead-ins and pauses.
    Q_PROPERTY(qint64 workoutElapsedMs READ workoutElapsedMs NOTIFY timeChanged)
    Q_PROPERTY(int totalSec READ totalSec NOTIFY stateChanged)

    // What the three buttons do now (R-UI-2): the play button is "pause", "resume", "start" or
    // "none"; a tap on skip is "next", "done", "end-set" or "none"; the long presses of skip (the
    // next block) and of stop are enabled or not.
    Q_PROPERTY(QString playAction READ playAction NOTIFY controlsChanged)
    Q_PROPERTY(QString skipKind READ skipKind NOTIFY controlsChanged)
    Q_PROPERTY(bool skipBlockEnabled READ skipBlockEnabled NOTIFY controlsChanged)
    Q_PROPERTY(bool stopEnabled READ stopEnabled NOTIFY controlsChanged)

    // The tempo of a paced step.
    Q_PROPERTY(int repetition READ repetition NOTIFY timeChanged)
    Q_PROPERTY(int repetitionCount READ repetitionCount NOTIFY stepChanged)
    Q_PROPERTY(int phase READ phase NOTIFY timeChanged)
    Q_PROPERTY(int phaseCount READ phaseCount NOTIFY stepChanged)
    Q_PROPERTY(QString phaseName READ phaseName NOTIFY timeChanged)
    Q_PROPERTY(qreal repetitionFraction READ repetitionFraction NOTIFY timeChanged)
    Q_PROPERTY(QVariantList phaseStarts READ phaseStarts NOTIFY stepChanged)
    // The voice is being prepared before the session starts (R-ALR-10): the share done, 0 to 1.
    Q_PROPERTY(qreal prepareProgress READ prepareProgress NOTIFY prepareProgressChanged)

public:
    enum State { Idle, LeadIn, Running, WaitingForReps, WaitingToStart, Paused, Finished, Preparing };
    Q_ENUM(State)

    explicit SessionBridge(Services *services, QObject *parent = nullptr);
    ~SessionBridge() override;

    State state() const { return state_; }
    bool inProgress() const;
    bool isRunning() const { return state_ == LeadIn || state_ == Running || state_ == WaitingForReps; }
    bool isPaused() const { return state_ == Paused; }
    bool isFinished() const { return state_ == Finished; }
    bool pausable() const { return isRunning() || state_ == WaitingToStart; }
    QString workoutName() const { return workout_.name; }
    int leadInSec() const { return leadInSec_; }
    void setLeadInSec(int seconds) { leadInSec_ = seconds; }
    QVariantList tickSeconds() const { return tickSeconds_; }
    void setTickSeconds(const QVariantList &seconds) { tickSeconds_ = seconds; }

    int stepNumber() const;
    int stepCount() const;
    int blockNumber() const;
    int blockCount() const;
    int roundNumber() const;
    int roundCount() const;
    QVariantMap current() const { return current_; }
    QVariantMap next() const { return next_; }
    QString exerciseFileKey() const { return exerciseFileKey_; }
    bool waitingToStart() const { return waitingToStart_; }
    QString phaseKind() const;

    qint64 stepRemainingMs() const;
    qint64 stepElapsedMs() const;
    qint64 stepPlannedMs() const;
    qint64 leadInRemainingMs() const;
    qint64 workoutRemainingMs() const;
    bool workoutRemainingOpen() const;
    qint64 workoutElapsedMs() const;
    int totalSec() const { return totalSec_; }
    QString playAction() const;
    QString skipKind() const;
    bool skipBlockEnabled() const { return controls().skipHold; }
    bool stopEnabled() const { return controls().stopHold; }

    int repetition() const;
    int repetitionCount() const;
    int phase() const;
    int phaseCount() const;
    QString phaseName() const;
    qreal repetitionFraction() const;
    QVariantList phaseStarts() const { return phaseStarts_; }
    qreal prepareProgress() const;
    // The speech of the app; without it the session is silent as before.
    void setSpeech(SpeechBridge *speech) { speech_ = speech; }

    // The clock the beats are scheduled on, in milliseconds.
    Q_INVOKABLE qint64 clockMs() const;
    // Starts the session of a saved workout with the lead-in of its first step. False, and nothing
    // starts, when a session is on or the workout cannot be read or run.
    Q_INVOKABLE bool start(int workoutId);
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    // A tap on skip: the next step; on a repetition step without a tempo it is "done" and on a
    // paced step it ends the step early (R-TMR-3).
    Q_INVOKABLE void skip();
    // A long press on skip: the first step of the next block, or the end of the workout in the last one.
    Q_INVOKABLE void skipBlock();
    // Ends the session: it is logged as stopped and the bridge is idle again.
    Q_INVOKABLE void stop();
    // Begins the session without the voice while it is being prepared (R-ALR-10).
    Q_INVOKABLE void startWithoutVoice();
    // The tap that starts a step that waits for one (R-WRK-11).
    Q_INVOKABLE void startWaitingStep();
    // Leaves the finished state: the bridge is idle again.
    Q_INVOKABLE void close();

signals:
    void stateChanged();
    // What the buttons do changed (the state or the step).
    void controlsChanged();
    // The step, the position or the plan changed.
    void stepChanged();
    // What the page shows changed. A step can change what is shown without a change of the state
    // (a paced step followed by a rest keeps the state Running), so this is not tied to stateChanged.
    void phaseKindChanged();
    // Emitted at every poll while time runs, and after every action: the time properties have new values.
    void timeChanged();

    // The events of the core, for the sounds and the vibration. type is "work", "rest", "warmup" or "cooldown".
    void stepStarted(const QString &type);
    void leadInTick(int remainingSeconds);
    void lastSecondsTick(int remainingSeconds);
    void roundChanged();
    // The cue of a step that waits for a tap: when the wait begins and every few seconds until it ends.
    void waitCue();
    // kind is "first", "middle" or "last"; phase is 0 without phases; atMs is the scheduled time.
    void beat(const QString &kind, int phase, int repetition, qint64 atMs);
    void paused();
    void resumed();
    void finished();
    void stopped();
    void prepareProgressChanged();

protected:
    bool wantsPolling() const override { return isRunning(); }
    void poll() override;

private:
    void handle(const QList<EngineEvent> &events);
    void begin();
    void checkPrepared();
    void voiceFinished();
    void sync();
    void setState(State state);
    void refreshStep();
    void updatePhaseKind();
    SessionControls::Controls controls() const;
    void pollWaitCue();
    QVariantMap stepMap(int index) const;
    void writeLog(EndState endState);
    const PlanStep &currentPlanStep() const;

    Services *services_;
    SpeechBridge *speech_ = nullptr;
    QScopedPointer<SpeechRun> speechRun_;
    QTimer prepareTimer_;
    // The cue of a step that waits for a tap, checked on a timer only while a step waits.
    QTimer waitTimer_;
    WaitCue waitCue_;
    QScopedPointer<SessionEngine> engine_;
    State state_ = Idle;
    Workout workout_;
    QHash<QString, Exercise> exercises_;
    qint64 startedAtMs_ = 0;
    bool logged_ = false;
    int totalSec_ = 0;
    int leadInSec_ = 3;
    QVariantList tickSeconds_;

    int shownStep_ = -1;
    int shownPlanSize_ = -1;
    QString shownKind_;
    bool waitingToStart_ = false;
    QVariantMap current_;
    QVariantMap next_;
    QString exerciseFileKey_;
    QVariantList phaseStarts_;
};

#endif

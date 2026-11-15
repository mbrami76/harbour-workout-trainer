// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef ENGINE_H
#define ENGINE_H

#include <QList>
#include <QSet>

#include "clock.h"
#include "workoutlimits.h"
#include "plan.h"
#include "spokencues.h"

// WaitingToStart: the step waits for the user's tap before its lead-in begins (R-WRK-11). The
// first step of the plan never waits: the tap that opens the session is its trigger.
// WaitingForVoice: the lead-in of a step is over but its spoken announcement is not finished; the step
// timer starts when voiceFinished() is called (R-ALR-6).
enum class SessionState { Ready, LeadIn, Running, WaitingForReps, Paused, Finished, Stopped, WaitingToStart,
                          WaitingForVoice };

// WaitingForStart: the engine holds before a step that starts on a tap (never the first step). Beat: one beat of a tempo
// step (R-WRK-10). Announce: a spoken announcement is due (R-ALR-6, R-ALR-11); only with
// EngineConfig::announcements. Cue: a spoken cue of the running step is due (R-ALR-15); only with
// EngineConfig::spokenCues.
enum class EventType { StepStarted, Tick, RoundChanged, Finished, Stopped, WaitingForStart, Beat, Announce, Cue };

// atMs is the scheduled time of the event on the engine's clock, not the time of the call that
// reported it. stepIndex is the plan position. remainingSec and inLeadIn are set for ticks only;
// repetition (from 1), phase (1 to 8, 0 without phases) and accent for beats only.
struct EngineEvent {
    EventType type = EventType::StepStarted;
    qint64 atMs = 0;
    int stepIndex = 0;
    int remainingSec = 0;
    bool inLeadIn = false;
    int repetition = 0;
    int phase = 0;
    bool accent = false;
    // Announce only: the step to introduce (-1 for none), whether the sentence that begins a rest
    // comes first, and whether this is the same announcement again after a pause (it is spoken
    // from its start; the rest sentence is not repeated).
    int announceIndex = -1;
    bool restSentence = false;
    bool repeated = false;
    // Cue only: which cue, and for the repetitions-left calls the repetition it falls at (in
    // `repetition`) and the number of repetitions left.
    SpokenCues::Kind cue = SpokenCues::Kind::Halfway;
    int cueCount = 0;
};

struct EngineConfig {
    int leadInSec = Limits::defaultLeadInSec;
    // Remaining whole seconds at which a tick is emitted (R-ALR-1).
    QList<int> tickSeconds = QList<int>() << 10 << 5 << 3 << 2 << 1;
    // A rest step has no lead-in, except when it is resumed with this many seconds or fewer left.
    int restResumeLeadInSec = 5;
    // Report the announcements that introduce the steps with an exercise and hold each such step
    // until the caller says the announcement is finished. Off: no event, no hold.
    bool announcements = false;
    // Report the spoken cues of a running step (R-ALR-15) at the times of its halo cues.
    bool spokenCues = false;
};

// Active time spent in one step reached by the session: lead-in and pauses are excluded.
struct StepRecord {
    int stepIndex = 0;
    qint64 activeMs = 0;
};

// Session state machine (R-TMR-1 to R-TMR-6). It owns no timer: the caller invokes
// update() periodically and after every user action. Remaining times come from the clock, never
// from the number of calls. Every call returns the events it produced, in order.
class SessionEngine
{
public:
    // A ready-made plan.
    SessionEngine(const Clock *clock, const QList<PlanStep> &plan,
                  const EngineConfig &config = EngineConfig());
    // Plan expanded from the workout; the workout itself is never changed.
    SessionEngine(const Clock *clock, const Workout &workout,
                  const EngineConfig &config = EngineConfig());
    // The same, with the tempos of the exercises (R-WRK-10) as a snapshot by exercise key. The
    // engine never reads the database: a change of a saved metronome or of the link of an exercise
    // during the session does not reach the running plan. An exercise without an entry runs its
    // repetition steps as plain repetition steps.
    SessionEngine(const Clock *clock, const Workout &workout, const ExerciseTempos &tempos,
                  const EngineConfig &config = EngineConfig());

    // Ready: begins the session; the lead-in of the first step follows at once, whatever its start
    // mode says. WaitingToStart: begins the lead-in of the waiting step, which is the tap of
    // R-WRK-11.
    QList<EngineEvent> start();
    QList<EngineEvent> pause();
    QList<EngineEvent> resume();
    QList<EngineEvent> next();
    // Skips to the first step of the next block, or ends the session when the running block is the
    // last one (R-TMR-3, R-TMR-4).
    QList<EngineEvent> nextBlock();
    QList<EngineEvent> done();
    QList<EngineEvent> stop();
    QList<EngineEvent> update();
    // The announcement that was reported with an Announce event is finished (or cannot be spoken):
    // a step that waits for it starts now. Without a pending announcement nothing happens.
    QList<EngineEvent> voiceFinished();
    // An announcement is pending: reported and not yet finished.
    bool voicePending() const { return voicePending_; }
    // Turns the announcements on or off before start() (the voice may turn out to be unavailable
    // after the engine was made).
    void setAnnouncements(bool on) { announcements_ = on; }
    // Turns the spoken cues on or off before start().
    void setSpokenCues(bool on) { spokenCues_ = on; }

    // Milliseconds from the start of the set of the current tempo step to the point shown: the
    // running time while it runs, the start of the repetition that is run again while it is paused
    // or in a lead-in after a pause, 0 otherwise or for a step without a tempo.
    qint64 tempoElapsedMs() const;

    SessionState state() const { return state_; }
    int stepIndex() const { return index_; }
    int stepCount() const { return plan_.size(); }
    const PlanStep &currentStep() const { return plan_.at(index_); }
    // The plan the session runs; the log is built from it.
    const QList<PlanStep> &plan() const { return plan_; }
    qint64 stepRemainingMs() const;
    qint64 leadInRemainingMs() const;
    qint64 currentStepActiveMs() const;
    // Active time of the whole session so far: every step reached, lead-ins and pauses excluded.
    qint64 elapsedActiveMs() const;
    // Planned time still to run, without lead-ins: the rest of the current step and every later
    // timed or tempo step. Repetition steps without a tempo and waits for a tap have no planned
    // time; *open tells whether the run has any.
    qint64 plannedRemainingMs(bool *open = nullptr) const;
    const QList<StepRecord> &records() const { return records_; }

private:
    void init(const EngineConfig &config);
    bool waitsForTap() const;
    void startStep(qint64 at, QList<EngineEvent> &events);
    void emitBeats(qint64 now, QList<EngineEvent> &events);
    void emitPeriodCues(qint64 now, QList<EngineEvent> &events);
    bool isLive() const;
    bool hasLeadIn(const PlanStep &step) const;
    bool resumeHasLeadIn() const;
    void advance(qint64 now, QList<EngineEvent> &events);
    void emitTicks(qint64 endMs, qint64 baseMs, QSet<int> &done, bool inLeadIn, qint64 now,
                   QList<EngineEvent> &events);
    void moveTo(int index, qint64 at, QList<EngineEvent> &events);
    void selectStep(int index);
    void beginPhase(qint64 at, QList<EngineEvent> &events);
    void startLeadIn(qint64 at);
    void afterLeadIn(qint64 at, QList<EngineEvent> &events);
    void enterStep(qint64 at, QList<EngineEvent> &events);
    void finish(qint64 at, QList<EngineEvent> &events);
    void accrue(qint64 ms);
    void jump(int index, QList<EngineEvent> &events);
    void announce(qint64 at, QList<EngineEvent> &events);
    static bool announces(const PlanStep &step);
    EngineEvent event(EventType type, qint64 at) const;

    const Clock *clock_;
    QList<PlanStep> plan_;
    QList<int> ticks_;
    qint64 leadInMs_;
    qint64 restResumeLeadInMs_;

    SessionState state_ = SessionState::Ready;
    int index_ = 0;
    qint64 leadEnd_ = 0;
    qint64 leadTotal_ = 0;
    qint64 stepEnd_ = 0;
    qint64 tickBase_ = 0;
    qint64 segmentStart_ = 0;
    qint64 pendingRemaining_ = 0;
    bool resuming_ = false;
    SessionState resumeState_ = SessionState::Running;
    QSet<int> tickedLead_;
    QSet<int> tickedStep_;
    bool hasStarted_ = false;
    int lastBlock_ = 0;
    int lastRound_ = 0;
    QList<StepRecord> records_;

    bool tapGiven_ = false;
    qint64 tempoStart_ = 0;
    int nextRep_ = 0;
    int nextBeat_ = 0;
    int pausedRep_ = 0;

    bool announcements_ = false;
    bool spokenCues_ = false;
    // The period cues already reported in the running step, by kind, and the last repetition whose
    // repetitions-left call was reported (a repetition run again after a pause is not called again).
    QSet<int> cuedStep_;
    int cuedRep_ = 0;
    bool voicePending_ = false;
    // The step the pending announcement introduces, and the step announced last (-1 for none).
    int voiceStep_ = -1;
    int announcedStep_ = -1;
};

#endif

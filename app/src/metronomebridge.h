// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef METRONOMEBRIDGE_H
#define METRONOMEBRIDGE_H

#include <QScopedPointer>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "metronome.h"
#include "polledbridge.h"

class Services;

// Drives the metronome run of the core (R-MET-1) for QML. Exposed as the context property
// "metronomeBridge", created in main() with the Services. It is silent: it only reports the
// events, with the beat kind, for the sounds of a later task and for the pulse and the vibration
// of the run page.
//
// The poll (33 ms, PolledBridge) runs only while a run is going and the app is active. Every time
// is read from the clock (R-TMR-5, R-MET-5). A beat carries its scheduled time on the clock
// (`atMs`, comparable with clockMs()), so the pulse can start from that time and not from the
// moment the signal arrives (R-ALR-12). Pausing when the app becomes inactive is the page's job
// (R-TMR-8, R-TMR-9).
//
// A configuration is a map: form ("reps-total", "time-per-rep" or "phases"), reps, totalSec
// (reps-total), repHalfSec (time-per-rep, in half seconds), phases (a list of 1 to 8 numbers of
// half seconds), phaseNames (a list of strings, one per phase, empty for no name; optional), sets,
// restSec, leadInSec; name (optional, the name of the saved metronome that is run, for the run page).
class MetronomeBridge : public PolledBridge
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool running READ isRunning NOTIFY stateChanged)
    Q_PROPERTY(bool inProgress READ inProgress NOTIFY stateChanged)
    // The name given with the configuration of the run in progress; empty for a run from fresh settings.
    Q_PROPERTY(QString name READ name NOTIFY stateChanged)
    // "leadin", "set" or "rest": the part of the run that is on, also while it is paused.
    Q_PROPERTY(QString section READ section NOTIFY timeChanged)
    Q_PROPERTY(int set READ set NOTIFY timeChanged)
    Q_PROPERTY(int setCount READ setCount NOTIFY timeChanged)
    Q_PROPERTY(int repetition READ repetition NOTIFY timeChanged)
    Q_PROPERTY(int repetitionCount READ repetitionCount NOTIFY timeChanged)
    Q_PROPERTY(int phase READ phase NOTIFY timeChanged)
    Q_PROPERTY(int phaseCount READ phaseCount NOTIFY timeChanged)
    // The name of the running phase; empty without a name (the page shows "Phase n").
    Q_PROPERTY(QString phaseName READ phaseName NOTIFY timeChanged)
    // The elapsed share of the running repetition, 0 to 1.
    Q_PROPERTY(qreal repetitionFraction READ repetitionFraction NOTIFY timeChanged)
    // Where each phase starts in a repetition, as shares 0 to 1 (a single 0 without phases).
    Q_PROPERTY(QVariantList phaseStarts READ phaseStarts NOTIFY stateChanged)
    Q_PROPERTY(qint64 remainingMs READ remainingMs NOTIFY timeChanged)
    Q_PROPERTY(qint64 restRemainingMs READ restRemainingMs NOTIFY timeChanged)
    Q_PROPERTY(qint64 restMs READ restMs NOTIFY timeChanged)
    Q_PROPERTY(qint64 leadInRemainingMs READ leadInRemainingMs NOTIFY timeChanged)
    Q_PROPERTY(bool canAddSet READ canAddSet NOTIFY timeChanged)
    Q_PROPERTY(bool canRemoveSet READ canRemoveSet NOTIFY timeChanged)

public:
    // Idle: no run. The others: a run is in progress (Paused: frozen).
    enum State { Idle, LeadIn, Running, Resting, Paused };
    Q_ENUM(State)

    explicit MetronomeBridge(Services *services, QObject *parent = nullptr);

    State state() const { return state_; }
    bool isRunning() const { return state_ == LeadIn || state_ == Running || state_ == Resting; }
    bool inProgress() const { return state_ != Idle; }
    QString name() const { return name_; }
    QString section() const;
    int set() const;
    int setCount() const;
    int repetition() const;
    int repetitionCount() const;
    int phase() const;
    int phaseCount() const;
    QString phaseName() const;
    qreal repetitionFraction() const;
    QVariantList phaseStarts() const { return phaseStarts_; }
    qint64 remainingMs() const;
    qint64 restRemainingMs() const;
    qint64 restMs() const { return restMs_; }
    qint64 leadInRemainingMs() const;
    bool canAddSet() const;
    bool canRemoveSet() const;

    // The clock the beats are scheduled on, in milliseconds.
    Q_INVOKABLE qint64 clockMs() const;
    // The number of characters of a phase name as the user sees them (the core's count).
    Q_INVOKABLE int nameLength(const QString &name) const;
    // Empty when the configuration is valid, else the first problem: "reps", "rep-short",
    // "rep-long", "phase-count", "phase-length", "phase-name", "sets", "rest" or "lead-in". The rules are the
    // ones of the core.
    Q_INVOKABLE QString problem(const QVariantMap &config) const;
    // Starts a run with the lead-in. False, and nothing starts, when a run is on or the
    // configuration has a problem.
    Q_INVOKABLE bool start(const QVariantMap &config);
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void stop();
    Q_INVOKABLE bool addSet();
    Q_INVOKABLE bool removeSet();

signals:
    void stateChanged();
    // Emitted at every poll while the run goes, and after every action: the properties have new values.
    void timeChanged();

    void leadInTick(int remainingSeconds);
    void setStarted(int set);
    void repetitionStarted(int set, int repetition);
    // kind is "first", "middle" or "last"; phase is 0 without phases; atMs is the scheduled time.
    void beat(const QString &kind, int phase, int repetition, qint64 atMs);
    void setEnded(int set);
    void restStarted(int set);
    void restTick(int remainingSeconds);
    void finished();
    void stopped();

protected:
    bool wantsPolling() const override { return state_ == LeadIn || state_ == Running || state_ == Resting; }
    void poll() override;

private:
    bool build(const QVariantMap &config, MetronomeConfig *out, QString *problem) const;
    void handle(const QList<MetronomeEvent> &events);
    void syncState();
    void setState(State state);

    Services *services_;
    QScopedPointer<Metronome> run_;
    State state_ = Idle;
    qint64 restMs_ = 0;
    QString name_;
    int phaseCount_ = 0;
    QStringList phaseNames_;
    QVariantList phaseStarts_;
};

#endif

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef INTERVALBRIDGE_H
#define INTERVALBRIDGE_H

#include <QScopedPointer>
#include <QString>
#include <QVariantList>

#include "intervaltimer.h"
#include "polledbridge.h"

class Services;

// Drives the endless work and recovery timer of the core (R-MOD-2) for QML. Exposed as the
// context property "intervalBridge", created in main() with the Services.
//
// The core timer owns no timer; this bridge polls it every 33 ms (30 frames per second) with the
// monotonic clock (PolledBridge). The poll runs only while the timer runs and the app is active, so
// an idle, paused or backgrounded timer costs nothing (R-NFR-2). Times are read from the clock, not
// counted from polls (R-TMR-5): a late poll shows the right time and reports the events that were
// due, with their own scheduled times. Pausing when the app becomes inactive is the page's job
// (R-TMR-8, R-TMR-9); the bridge only stops polling.
class IntervalBridge : public PolledBridge
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool running READ isRunning NOTIFY stateChanged)
    Q_PROPERTY(bool inProgress READ inProgress NOTIFY stateChanged)
    Q_PROPERTY(QString periodKind READ periodKind NOTIFY periodChanged)
    Q_PROPERTY(qint64 remainingMs READ remainingMs NOTIFY timeChanged)
    Q_PROPERTY(qint64 elapsedMs READ elapsedMs NOTIFY timeChanged)
    Q_PROPERTY(qint64 periodMs READ periodMs NOTIFY periodChanged)
    Q_PROPERTY(int completedWork READ completedWork NOTIFY periodChanged)
    // The seconds at which a last-seconds tick is reported (R-ALR-1); set before start().
    Q_PROPERTY(QVariantList tickSeconds READ tickSeconds WRITE setTickSeconds)

public:
    // Idle: no run. Running and Paused: a run is in progress.
    enum State { Idle, Running, Paused };
    Q_ENUM(State)

    explicit IntervalBridge(Services *services, QObject *parent = nullptr);

    State state() const { return state_; }
    bool isRunning() const { return state_ == Running; }
    // A run is on, paused or not.
    bool inProgress() const { return state_ != Idle; }
    // "work" or "recovery" (the kinds of StepStyle); "work" when idle.
    QString periodKind() const;
    qint64 remainingMs() const;
    qint64 elapsedMs() const;
    qint64 periodMs() const;
    int completedWork() const;
    QVariantList tickSeconds() const { return tickSeconds_; }
    void setTickSeconds(const QVariantList &seconds) { tickSeconds_ = seconds; }

    // Starts a run with the work period at once. False, and nothing starts, when a run is on or a
    // duration is outside the limits of the core (work 1 to 3600 s, recovery 0 to 600 s).
    Q_INVOKABLE bool start(int workSeconds, int recoverySeconds);
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    // Ends the run; the bridge is idle again.
    Q_INVOKABLE void stop();

signals:
    void stateChanged();
    // The period or its counters changed (a period started, or the run ended).
    void periodChanged();
    // Emitted at every poll while the timer runs; the time properties have new values.
    void timeChanged();

    // The events of the core, for the sounds and the vibration of a later task.
    void periodStarted(const QString &kind, int index);
    void periodKindChanged(const QString &kind);
    // The seconds left in the period at a last-seconds tick.
    void lastSecondsTick(int remainingSeconds);
    void stopped();

protected:
    bool wantsPolling() const override { return state_ == Running; }
    void poll() override;

private:
    void handle(const QList<IntervalEvent> &events);
    void setState(State state);

    Services *services_;
    QVariantList tickSeconds_;
    QScopedPointer<IntervalTimer> timer_;
    State state_ = Idle;
};

#endif

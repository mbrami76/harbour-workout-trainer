// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef STOPWATCHBRIDGE_H
#define STOPWATCHBRIDGE_H

#include "polledbridge.h"
#include "stopwatch.h"

class Services;

// The stopwatch of the timer screen (R-MOD-3) for QML, over the Stopwatch of the core. Exposed as
// the context property "stopwatchBridge"; the time attack bridge has a stopwatch of its own. Polled
// only while it runs and the app is active (PolledBridge). The stopwatch never ends by itself.
class StopwatchBridge : public PolledBridge
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool running READ isRunning NOTIFY stateChanged)
    // Running or paused.
    Q_PROPERTY(bool inProgress READ inProgress NOTIFY stateChanged)
    Q_PROPERTY(qint64 elapsedMs READ elapsedMs NOTIFY timeChanged)

public:
    enum State { Ready, Running, Paused };
    Q_ENUM(State)

    explicit StopwatchBridge(Services *services, QObject *parent = nullptr);

    State state() const { return state_; }
    bool isRunning() const { return state_ == Running; }
    bool inProgress() const { return state_ != Ready; }
    qint64 elapsedMs() const { return watch_.elapsedMs(); }

    Q_INVOKABLE void start();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    // Back to zero, from any state.
    Q_INVOKABLE void reset();

signals:
    void stateChanged();
    void timeChanged();
    // The stopwatch started from zero (R-ALR-1).
    void started();

protected:
    bool wantsPolling() const override { return state_ == Running; }
    void poll() override { emit timeChanged(); }

private:
    void setState(State state);

    Stopwatch watch_;
    State state_ = Ready;
};

#endif

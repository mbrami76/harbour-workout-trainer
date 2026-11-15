// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef COUNTDOWNBRIDGE_H
#define COUNTDOWNBRIDGE_H

#include "countdown.h"
#include <QVariantList>

#include "polledbridge.h"

class Services;

// The countdown of the timer screen (R-MOD-3) for QML, over the Countdown of the core. Exposed as
// the context property "countdownBridge". Polled every 33 ms only while it runs and the app is
// active (PolledBridge); times come from the clock. Pausing when the app becomes inactive is the
// page's job (R-TMR-8, R-TMR-9). It emits `reachedZero` once at the end; the sound is a later task.
class CountdownBridge : public PolledBridge
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool running READ isRunning NOTIFY stateChanged)
    // Running or paused.
    Q_PROPERTY(bool inProgress READ inProgress NOTIFY stateChanged)
    Q_PROPERTY(int durationSec READ durationSec NOTIFY durationChanged)
    Q_PROPERTY(qint64 remainingMs READ remainingMs NOTIFY timeChanged)
    Q_PROPERTY(qint64 elapsedMs READ elapsedMs NOTIFY timeChanged)

public:
    enum State { Ready, Running, Paused, Finished };
    Q_ENUM(State)

    explicit CountdownBridge(Services *services, QObject *parent = nullptr);

    State state() const { return state_; }
    bool isRunning() const { return state_ == Running; }
    bool inProgress() const { return state_ == Running || state_ == Paused; }
    int durationSec() const { return countdown_.durationSec(); }
    qint64 remainingMs() const { return countdown_.remainingMs(); }
    qint64 elapsedMs() const { return countdown_.elapsedMs(); }

    // Sets the duration, 1 to 5999 s, while the countdown is ready. False otherwise.
    Q_INVOKABLE bool setDuration(int seconds);
    Q_INVOKABLE void start();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    // Back to ready, from any state; the duration stays.
    Q_INVOKABLE void reset();
    // The seconds at which a tick is reported (R-ALR-1); set while the countdown is ready.
    Q_INVOKABLE void setTickSeconds(const QVariantList &seconds);

signals:
    void stateChanged();
    void durationChanged();
    void timeChanged();
    // The countdown has reached zero.
    void reachedZero();
    // The seconds left at a last-seconds tick (R-ALR-1).
    void lastSecondsTick(int remainingSeconds);

protected:
    bool wantsPolling() const override { return state_ == Running; }
    void poll() override;

private:
    void setState(State state);

    Countdown countdown_;
    State state_ = Ready;
};

#endif

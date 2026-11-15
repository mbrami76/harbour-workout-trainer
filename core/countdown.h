// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef COUNTDOWN_H
#define COUNTDOWN_H

#include <QList>
#include <QSet>

#include "clock.h"

enum class CountdownState { Ready, Running, Paused, Finished };

// Countdown of the timer mode (R-MOD-3, R-MOD-5). It owns no thread and no timer: the caller asks
// for the time whenever it draws, and calls update() to learn that the countdown has ended.
// Remaining and elapsed times come from the monotonic clock (R-TMR-5).
class Countdown
{
public:
    static const int minSeconds = 1;
    static const int maxSeconds = 359999;

    explicit Countdown(const Clock *clock) : clock_(clock) {}

    // Sets the duration. Allowed only in the Ready state and within the limits; otherwise false
    // and nothing changes.
    bool setDuration(int seconds);
    int durationSec() const { return int(durationMs_ / 1000); }

    // Each returns whether it changed the state.
    bool start();
    bool pause();
    bool resume();
    // Back to Ready with the same duration, from any state.
    void reset();

    // True exactly once, when the countdown has reached zero. Later calls return false.
    bool update();

    // The remaining whole seconds at which a tick is due (R-ALR-1): whole numbers from 1, each once
    // per run, only when smaller than the duration. Set before start().
    void setTickSeconds(const QList<int> &seconds);
    // The ticks whose time has passed since the last call, largest first. Call it before update()
    // so that the ticks of the last second come before the end.
    QList<int> takeTicks();

    CountdownState state() const;
    qint64 remainingMs() const;
    qint64 elapsedMs() const;

private:
    // Moves a countdown that has run out to Finished and arms the finished event.
    void settle();

    const Clock *clock_;
    qint64 durationMs_ = 0;
    CountdownState state_ = CountdownState::Ready;
    qint64 endMs_ = 0;
    qint64 remainingAtPauseMs_ = 0;
    bool finishedPending_ = false;
    QList<int> ticks_ = QList<int>() << 10 << 5 << 3 << 2 << 1;
    QSet<int> ticked_;
};

#endif

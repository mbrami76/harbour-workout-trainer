// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef STOPWATCH_H
#define STOPWATCH_H

#include "clock.h"

enum class StopwatchState { Ready, Running, Paused };

// Stopwatch of the timer mode and of time attack (R-MOD-3, R-MOD-4, R-MOD-5). It owns no thread
// and no timer: the caller asks for the elapsed time whenever it draws. The time comes from the
// monotonic clock (R-TMR-5) and does not move while the stopwatch is paused.
class Stopwatch
{
public:
    explicit Stopwatch(const Clock *clock) : clock_(clock) {}

    // Each returns whether it changed the state.
    bool start();
    bool pause();
    bool resume();
    // Back to Ready with zero elapsed time, from any state.
    void reset();
    // Ends a run (time attack): returns the elapsed milliseconds and goes back to Ready. Returns -1
    // and changes nothing when the stopwatch is Ready.
    qint64 stop();

    StopwatchState state() const { return state_; }
    qint64 elapsedMs() const;

private:
    const Clock *clock_;
    StopwatchState state_ = StopwatchState::Ready;
    qint64 segmentStartMs_ = 0;
    qint64 accruedMs_ = 0;
};

#endif

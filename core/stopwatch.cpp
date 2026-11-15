// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "stopwatch.h"

bool Stopwatch::start()
{
    if (state_ != StopwatchState::Ready)
        return false;
    accruedMs_ = 0;
    segmentStartMs_ = clock_->nowMs();
    state_ = StopwatchState::Running;
    return true;
}

bool Stopwatch::pause()
{
    if (state_ != StopwatchState::Running)
        return false;
    accruedMs_ += clock_->nowMs() - segmentStartMs_;
    state_ = StopwatchState::Paused;
    return true;
}

bool Stopwatch::resume()
{
    if (state_ != StopwatchState::Paused)
        return false;
    segmentStartMs_ = clock_->nowMs();
    state_ = StopwatchState::Running;
    return true;
}

void Stopwatch::reset()
{
    state_ = StopwatchState::Ready;
    accruedMs_ = 0;
    segmentStartMs_ = 0;
}

qint64 Stopwatch::stop()
{
    if (state_ == StopwatchState::Ready)
        return -1;
    const qint64 elapsed = elapsedMs();
    reset();
    return elapsed;
}

qint64 Stopwatch::elapsedMs() const
{
    if (state_ == StopwatchState::Running)
        return accruedMs_ + (clock_->nowMs() - segmentStartMs_);
    return accruedMs_;
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "countdown.h"

#include <algorithm>
#include <functional>

bool Countdown::setDuration(int seconds)
{
    if (state_ != CountdownState::Ready || seconds < minSeconds || seconds > maxSeconds)
        return false;
    durationMs_ = qint64(seconds) * 1000;
    return true;
}

void Countdown::settle()
{
    if (state_ == CountdownState::Running && clock_->nowMs() >= endMs_) {
        state_ = CountdownState::Finished;
        finishedPending_ = true;
    }
}

bool Countdown::start()
{
    if (state_ != CountdownState::Ready || durationMs_ <= 0)
        return false;
    endMs_ = clock_->nowMs() + durationMs_;
    ticked_.clear();
    state_ = CountdownState::Running;
    return true;
}

bool Countdown::pause()
{
    settle();
    if (state_ != CountdownState::Running)
        return false;
    remainingAtPauseMs_ = endMs_ - clock_->nowMs();
    state_ = CountdownState::Paused;
    return true;
}

bool Countdown::resume()
{
    if (state_ != CountdownState::Paused)
        return false;
    endMs_ = clock_->nowMs() + remainingAtPauseMs_;
    state_ = CountdownState::Running;
    return true;
}

void Countdown::reset()
{
    state_ = CountdownState::Ready;
    finishedPending_ = false;
    ticked_.clear();
    remainingAtPauseMs_ = 0;
    endMs_ = 0;
}

bool Countdown::update()
{
    settle();
    const bool pending = finishedPending_;
    finishedPending_ = false;
    return pending;
}

CountdownState Countdown::state() const
{
    if (state_ == CountdownState::Running && clock_->nowMs() >= endMs_)
        return CountdownState::Finished;
    return state_;
}

qint64 Countdown::remainingMs() const
{
    switch (state()) {
    case CountdownState::Ready:
        return durationMs_;
    case CountdownState::Running:
        return endMs_ - clock_->nowMs();
    case CountdownState::Paused:
        return remainingAtPauseMs_;
    case CountdownState::Finished:
        return 0;
    }
    return 0;
}

qint64 Countdown::elapsedMs() const
{
    return durationMs_ - remainingMs();
}

void Countdown::setTickSeconds(const QList<int> &seconds)
{
    ticks_.clear();
    for (int s : seconds) {
        if (s >= 1 && !ticks_.contains(s))
            ticks_.append(s);
    }
    std::sort(ticks_.begin(), ticks_.end(), std::greater<int>());
}

QList<int> Countdown::takeTicks()
{
    QList<int> due;
    settle();
    if (state_ != CountdownState::Running && state_ != CountdownState::Finished)
        return due;
    const qint64 now = clock_->nowMs();
    for (int seconds : ticks_) {
        const qint64 remaining = qint64(seconds) * 1000;
        if (remaining >= durationMs_ || ticked_.contains(seconds))
            continue;
        if (endMs_ - remaining > now)
            break;
        ticked_.insert(seconds);
        due.append(seconds);
    }
    return due;
}

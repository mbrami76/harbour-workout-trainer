// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "intervaltimer.h"

#include <algorithm>
#include <functional>

bool IntervalTimer::isValid(int workSec, int recoverySec)
{
    return workSec >= minWorkSec && workSec <= maxWorkSec
        && recoverySec >= minRecoverySec && recoverySec <= maxRecoverySec;
}

IntervalTimer::IntervalTimer(const Clock *clock, int workSec, int recoverySec, const QList<int> &tickSeconds)
    : clock_(clock),
      workMs_(qint64(workSec) * 1000),
      recoveryMs_(qint64(recoverySec) * 1000),
      valid_(isValid(workSec, recoverySec))
{
    for (int seconds : tickSeconds) {
        if (seconds >= 1 && !ticks_.contains(seconds))
            ticks_.append(seconds);
    }
    std::sort(ticks_.begin(), ticks_.end(), std::greater<int>());
}

qint64 IntervalTimer::durationOf(PeriodKind kind) const
{
    return kind == PeriodKind::Work ? workMs_ : recoveryMs_;
}

IntervalEvent IntervalTimer::event(IntervalEventType type, qint64 at) const
{
    IntervalEvent e;
    e.type = type;
    e.atMs = at;
    e.kind = kind_;
    e.periodIndex = periodCount_ - 1;
    return e;
}

void IntervalTimer::beginPeriod(PeriodKind kind, qint64 at, QList<IntervalEvent> &events)
{
    const bool changed = periodCount_ > 0 && kind != kind_;
    kind_ = kind;
    ++periodCount_;
    periodEnd_ = at + durationOf(kind);
    tickBase_ = durationOf(kind);
    ticked_.clear();
    if (changed)
        events.append(event(IntervalEventType::PeriodChanged, at));
    events.append(event(IntervalEventType::PeriodStarted, at));
}

void IntervalTimer::emitTicks(qint64 now, QList<IntervalEvent> &events)
{
    for (int seconds : ticks_) {
        const qint64 remaining = qint64(seconds) * 1000;
        if (remaining >= tickBase_ || ticked_.contains(seconds))
            continue;
        const qint64 at = periodEnd_ - remaining;
        if (at > now)
            break;
        IntervalEvent e = event(IntervalEventType::Tick, at);
        e.remainingSec = seconds;
        events.append(e);
        ticked_.insert(seconds);
    }
}

void IntervalTimer::advance(qint64 now, QList<IntervalEvent> &events)
{
    while (state_ == IntervalState::Running) {
        emitTicks(now, events);
        if (periodEnd_ > now)
            return;
        const qint64 at = periodEnd_;
        if (kind_ == PeriodKind::Work)
            ++completedWork_;
        // A work period is followed by recovery unless recovery is 0; recovery by work.
        const PeriodKind next = (kind_ == PeriodKind::Work && recoveryMs_ > 0) ? PeriodKind::Recovery
                                                                                : PeriodKind::Work;
        beginPeriod(next, at, events);
    }
}

QList<IntervalEvent> IntervalTimer::start()
{
    QList<IntervalEvent> events;
    if (state_ != IntervalState::Ready || !valid_)
        return events;
    const qint64 now = clock_->nowMs();
    state_ = IntervalState::Running;
    completedWork_ = 0;
    periodCount_ = 0;
    beginPeriod(PeriodKind::Work, now, events);
    advance(now, events);
    return events;
}

QList<IntervalEvent> IntervalTimer::pause()
{
    QList<IntervalEvent> events;
    advance(clock_->nowMs(), events);
    if (state_ != IntervalState::Running)
        return events;
    remainingAtPause_ = periodEnd_ - clock_->nowMs();
    state_ = IntervalState::Paused;
    return events;
}

QList<IntervalEvent> IntervalTimer::resume()
{
    QList<IntervalEvent> events;
    if (state_ != IntervalState::Paused)
        return events;
    const qint64 now = clock_->nowMs();
    periodEnd_ = now + remainingAtPause_;
    state_ = IntervalState::Running;
    advance(now, events);
    return events;
}

QList<IntervalEvent> IntervalTimer::stop()
{
    QList<IntervalEvent> events;
    advance(clock_->nowMs(), events);
    if (state_ != IntervalState::Running && state_ != IntervalState::Paused)
        return events;
    state_ = IntervalState::Stopped;
    events.append(event(IntervalEventType::Stopped, clock_->nowMs()));
    return events;
}

QList<IntervalEvent> IntervalTimer::update()
{
    QList<IntervalEvent> events;
    advance(clock_->nowMs(), events);
    return events;
}

qint64 IntervalTimer::periodRemainingMs() const
{
    switch (state_) {
    case IntervalState::Running:
        return qMax<qint64>(0, periodEnd_ - clock_->nowMs());
    case IntervalState::Paused:
        return remainingAtPause_;
    default:
        return 0;
    }
}

qint64 IntervalTimer::periodElapsedMs() const
{
    if (state_ != IntervalState::Running && state_ != IntervalState::Paused)
        return 0;
    return periodDurationMs() - periodRemainingMs();
}

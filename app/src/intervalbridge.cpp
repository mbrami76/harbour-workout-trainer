// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "intervalbridge.h"

#include "services.h"

namespace {

QString kindName(PeriodKind kind)
{
    return kind == PeriodKind::Work ? QStringLiteral("work") : QStringLiteral("recovery");
}

}

IntervalBridge::IntervalBridge(Services *services, QObject *parent)
    : PolledBridge(parent), services_(services)
{
}

QString IntervalBridge::periodKind() const
{
    return timer_ ? kindName(timer_->periodKind()) : QStringLiteral("work");
}

qint64 IntervalBridge::remainingMs() const
{
    return timer_ ? timer_->periodRemainingMs() : 0;
}

qint64 IntervalBridge::elapsedMs() const
{
    return timer_ ? timer_->periodElapsedMs() : 0;
}

qint64 IntervalBridge::periodMs() const
{
    return timer_ ? timer_->periodDurationMs() : 0;
}

int IntervalBridge::completedWork() const
{
    return timer_ ? timer_->completedWorkPeriods() : 0;
}

bool IntervalBridge::start(int workSeconds, int recoverySeconds)
{
    if (state_ != Idle || !IntervalTimer::isValid(workSeconds, recoverySeconds))
        return false;
    QList<int> ticks;
    for (const QVariant &v : tickSeconds_)
        ticks.append(v.toInt());
    if (ticks.isEmpty())
        timer_.reset(new IntervalTimer(services_->clock(), workSeconds, recoverySeconds));
    else
        timer_.reset(new IntervalTimer(services_->clock(), workSeconds, recoverySeconds, ticks));
    handle(timer_->start());
    setState(Running);
    emit periodChanged();
    emit timeChanged();
    return true;
}

void IntervalBridge::pause()
{
    if (state_ != Running)
        return;
    handle(timer_->pause());
    setState(Paused);
    emit periodChanged();
    emit timeChanged();
}

void IntervalBridge::resume()
{
    if (state_ != Paused)
        return;
    handle(timer_->resume());
    setState(Running);
    emit timeChanged();
}

void IntervalBridge::stop()
{
    if (state_ == Idle)
        return;
    handle(timer_->stop());
    timer_.reset();
    setState(Idle);
    emit periodChanged();
    emit timeChanged();
}

void IntervalBridge::poll()
{
    if (state_ != Running)
        return;
    const int before = timer_->periodCount();
    handle(timer_->update());
    if (timer_->periodCount() != before)
        emit periodChanged();
    emit timeChanged();
}

void IntervalBridge::setState(State state)
{
    if (state_ == state)
        return;
    state_ = state;
    updatePolling();
    emit stateChanged();
}

void IntervalBridge::handle(const QList<IntervalEvent> &events)
{
    for (const IntervalEvent &e : events) {
        switch (e.type) {
        case IntervalEventType::PeriodStarted:
            emit periodStarted(kindName(e.kind), e.periodIndex);
            break;
        case IntervalEventType::PeriodChanged:
            emit periodKindChanged(kindName(e.kind));
            break;
        case IntervalEventType::Tick:
            emit lastSecondsTick(e.remainingSec);
            break;
        case IntervalEventType::Stopped:
            emit stopped();
            break;
        }
    }
}

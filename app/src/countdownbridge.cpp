// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "countdownbridge.h"

#include "services.h"

CountdownBridge::CountdownBridge(Services *services, QObject *parent)
    : PolledBridge(parent), countdown_(services->clock())
{
}

bool CountdownBridge::setDuration(int seconds)
{
    if (!countdown_.setDuration(seconds))
        return false;
    emit durationChanged();
    emit timeChanged();
    return true;
}

void CountdownBridge::start()
{
    if (countdown_.start())
        setState(Running);
    emit timeChanged();
}

void CountdownBridge::pause()
{
    if (countdown_.pause())
        setState(Paused);
    poll();
}

void CountdownBridge::resume()
{
    if (countdown_.resume())
        setState(Running);
    emit timeChanged();
}

void CountdownBridge::reset()
{
    countdown_.reset();
    setState(Ready);
    emit timeChanged();
}

void CountdownBridge::setTickSeconds(const QVariantList &seconds)
{
    if (state_ != Ready)
        return;
    QList<int> list;
    for (const QVariant &v : seconds)
        list.append(v.toInt());
    countdown_.setTickSeconds(list);
}

void CountdownBridge::poll()
{
    for (int seconds : countdown_.takeTicks())
        emit lastSecondsTick(seconds);
    if (countdown_.update()) {
        setState(Finished);
        emit reachedZero();
    }
    emit timeChanged();
}

void CountdownBridge::setState(State state)
{
    if (state_ == state)
        return;
    state_ = state;
    updatePolling();
    emit stateChanged();
}

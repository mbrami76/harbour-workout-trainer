// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "stopwatchbridge.h"

#include "services.h"

StopwatchBridge::StopwatchBridge(Services *services, QObject *parent)
    : PolledBridge(parent), watch_(services->clock())
{
}

void StopwatchBridge::start()
{
    if (watch_.start()) {
        setState(Running);
        emit started();
    }
    emit timeChanged();
}

void StopwatchBridge::pause()
{
    if (watch_.pause())
        setState(Paused);
    emit timeChanged();
}

void StopwatchBridge::resume()
{
    if (watch_.resume())
        setState(Running);
    emit timeChanged();
}

void StopwatchBridge::reset()
{
    watch_.reset();
    setState(Ready);
    emit timeChanged();
}

void StopwatchBridge::setState(State state)
{
    if (state_ == state)
        return;
    state_ = state;
    updatePolling();
    emit stateChanged();
}

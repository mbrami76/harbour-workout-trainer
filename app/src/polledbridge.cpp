// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "polledbridge.h"

#include <QGuiApplication>

PolledBridge::PolledBridge(QObject *parent)
    : QObject(parent)
{
    pollTimer_.setInterval(33);
    pollTimer_.setTimerType(Qt::PreciseTimer);
    connect(&pollTimer_, &QTimer::timeout, this, &PolledBridge::onTimeout);
    connect(qApp, &QGuiApplication::applicationStateChanged, this, &PolledBridge::onApplicationStateChanged);
}

void PolledBridge::onApplicationStateChanged()
{
    updatePolling();
}

void PolledBridge::updatePolling()
{
    const bool wanted = wantsPolling() && QGuiApplication::applicationState() == Qt::ApplicationActive;
    if (wanted && !pollTimer_.isActive()) {
        // Catch up at once: what fell due while the app was inactive is reported.
        poll();
        pollTimer_.start();
    } else if (!wanted) {
        pollTimer_.stop();
    }
}

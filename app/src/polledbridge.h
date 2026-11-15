// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef POLLEDBRIDGE_H
#define POLLEDBRIDGE_H

#include <QObject>
#include <QTimer>

// The polling of the bridges of the timer modes: a QTimer of 33 ms (30 frames per second) that
// runs only while the bridge says it has something to poll (wantsPolling()) and the app is active.
// Paused, idle and backgrounded cost nothing (R-NFR-2). A subclass calls updatePolling() whenever
// its state changes, and does its work in poll(); the time itself always comes from the clock
// (R-TMR-5), never from the number of polls.
class PolledBridge : public QObject
{
    Q_OBJECT

public:
    explicit PolledBridge(QObject *parent = nullptr);

protected:
    // True while the timer of the bridge is running.
    virtual bool wantsPolling() const = 0;
    virtual void poll() = 0;
    void updatePolling();

private slots:
    void onApplicationStateChanged();
    void onTimeout() { poll(); }

private:
    QTimer pollTimer_;
};

#endif

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef DASHBOARDBRIDGE_H
#define DASHBOARDBRIDGE_H

#include <QObject>
#include <QStringList>
#include <QVariantMap>

class Services;

// The figures of the dashboard (R-DSH-1 to R-DSH-4) for QML, exposed as the context property
// "dashboard". The numbers come from Statistics over the session log, for the current time and the
// time zone of the device; nothing is cached, so every call reads the log again.
class DashboardBridge : public QObject
{
    Q_OBJECT

public:
    explicit DashboardBridge(Services *services, QObject *parent = nullptr);

    // {ok, sessionsInWeek, weeks: [period], months: [period], most: [group], least: [group]}.
    // A period is {first, last (ISO dates), days, totalSec, averageMinutes}; a group is {key, seconds}.
    // `ok` is false when the log cannot be read.
    Q_INVOKABLE QVariantMap overview();
    // The keys of the 5 groups with the least time over the last 5 sessions (R-DSH-4); an empty list
    // when the log holds fewer than 5 sessions.
    Q_INVOKABLE QStringList suggestedGroups();
    // The milliseconds since the end of the last completed session (R-UI-17), or -1 when there is
    // none or the log cannot be read. Read again at every call.
    Q_INVOKABLE qint64 sinceLastCompletedMs();

private:
    Services *services_;
};

#endif

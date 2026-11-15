// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "dashboardbridge.h"

#include <QDateTime>
#include <QVariantList>

#include "musclegroups.h"
#include "services.h"
#include "statistics.h"

namespace {

QVariantList periodsOf(const QList<Statistics::Period> &periods)
{
    QVariantList list;
    for (const Statistics::Period &p : periods) {
        QVariantMap map;
        map.insert(QStringLiteral("first"), p.firstDay.toString(Qt::ISODate));
        map.insert(QStringLiteral("last"), p.lastDay.toString(Qt::ISODate));
        map.insert(QStringLiteral("days"), p.days);
        map.insert(QStringLiteral("totalSec"), p.totalSec);
        map.insert(QStringLiteral("averageMinutes"), p.averageMinutes);
        list.append(map);
    }
    return list;
}

QVariantList groupsOf(const QList<Statistics::GroupTime> &groups)
{
    QVariantList list;
    for (const Statistics::GroupTime &g : groups) {
        QVariantMap map;
        map.insert(QStringLiteral("key"), MuscleKeys::keyOf(g.group));
        map.insert(QStringLiteral("seconds"), g.seconds);
        list.append(map);
    }
    return list;
}

}

DashboardBridge::DashboardBridge(Services *services, QObject *parent)
    : QObject(parent), services_(services)
{
}

QVariantMap DashboardBridge::overview()
{
    QVariantMap map;
    const Statistics::Overview o = Statistics::overview(services_->database(), QDateTime::currentMSecsSinceEpoch(),
                                                        QTimeZone::systemTimeZone());
    map.insert(QStringLiteral("ok"), !o.failed);
    map.insert(QStringLiteral("sessionsInWeek"), o.sessionsInWeek);
    map.insert(QStringLiteral("weeks"), periodsOf(o.weeks));
    map.insert(QStringLiteral("months"), periodsOf(o.months));
    map.insert(QStringLiteral("most"), groupsOf(o.mostTrained));
    map.insert(QStringLiteral("least"), groupsOf(o.leastTrained));
    return map;
}

QStringList DashboardBridge::suggestedGroups()
{
    MuscleSet set = 0;
    if (!Statistics::suggestedGroups(services_->database(), &set))
        return QStringList();
    return MuscleKeys::keysOf(set);
}

qint64 DashboardBridge::sinceLastCompletedMs()
{
    qint64 end = 0;
    if (!services_->sessionLog()->lastCompletedEndMs(&end))
        return -1;
    return qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - end);
}

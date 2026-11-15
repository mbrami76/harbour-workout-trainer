// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "timeformatter.h"

#include "timeformat.h"

#include <QVariantMap>

QString TimeFormatter::countdown(qint64 ms) const
{
    if (ms < 0)
        ms = 0;
    return TimeFormat::countdown(int((ms + 999) / 1000));
}

QString TimeFormatter::elapsed(qint64 ms) const
{
    if (ms < 0)
        ms = 0;
    return TimeFormat::countdown(int(ms / 1000));
}

namespace {

QVariantList toList(const QList<TimeFormat::Part> &parts)
{
    QVariantList list;
    for (const TimeFormat::Part &part : parts) {
        QVariantMap map;
        map.insert(QStringLiteral("text"), part.text);
        map.insert(QStringLiteral("large"), part.large);
        list.append(map);
    }
    return list;
}

}

QVariantList TimeFormatter::countdownParts(qint64 ms) const
{
    return toList(TimeFormat::parts(int((qMax<qint64>(ms, 0) + 999) / 1000)));
}

QVariantList TimeFormatter::elapsedParts(qint64 ms) const
{
    return toList(TimeFormat::parts(int(qMax<qint64>(ms, 0) / 1000)));
}

QString TimeFormatter::precise(qint64 ms) const
{
    return TimeFormat::precise(ms);
}

QVariantMap TimeFormatter::ago(qint64 ms) const
{
    const TimeFormat::Ago a = TimeFormat::ago(ms);
    static const char *const kinds[] = {"now", "minutes", "hoursMinutes", "daysHours"};
    QVariantMap map;
    map.insert(QStringLiteral("kind"), QLatin1String(kinds[a.kind]));
    map.insert(QStringLiteral("major"), a.major);
    map.insert(QStringLiteral("minor"), a.minor);
    return map;
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "timeformat.h"

QString TimeFormat::countdown(int seconds)
{
    if (seconds < 0)
        seconds = 0;

    const int hours = seconds / 3600;
    const int minutes = (seconds % 3600) / 60;
    const int secs = seconds % 60;

    if (hours > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(secs, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(secs, 2, 10, QLatin1Char('0'));
}

QList<TimeFormat::Part> TimeFormat::parts(int seconds)
{
    if (seconds < 0)
        seconds = 0;
    const int hours = seconds / 3600;
    const int minutes = (seconds % 3600) / 60;
    const int secs = seconds % 60;
    const QString mm = QStringLiteral("%1").arg(minutes, 2, 10, QLatin1Char('0'));
    const QString ss = QStringLiteral("%1").arg(secs, 2, 10, QLatin1Char('0'));

    QList<Part> list;
    Part first;
    Part second;
    if (hours > 0) {
        first.text = QString::number(hours);
        first.large = true;
        second.text = QLatin1Char(':') + mm + QLatin1Char(':') + ss;
    } else if (minutes > 0) {
        first.text = QString::number(minutes);
        first.large = true;
        second.text = QLatin1Char(':') + ss;
    } else {
        first.text = QStringLiteral("0:");
        second.text = ss;
        second.large = true;
    }
    list.append(first);
    list.append(second);
    return list;
}

TimeFormat::Ago TimeFormat::ago(qint64 ms)
{
    Ago result;
    const qint64 minutes = ms / 60000;
    if (ms < 0 || minutes < 1)
        return result;
    if (minutes < 60) {
        result.kind = Ago::Minutes;
        result.major = int(minutes);
    } else if (minutes < 24 * 60) {
        result.kind = Ago::HoursMinutes;
        result.major = int(minutes / 60);
        result.minor = int(minutes % 60);
    } else {
        result.kind = Ago::DaysHours;
        result.major = int(minutes / (24 * 60));
        result.minor = int((minutes / 60) % 24);
    }
    return result;
}

QString TimeFormat::precise(qint64 ms)
{
    if (ms < 0)
        ms = 0;
    const qint64 hundredths = (ms % 1000) / 10;
    const QString cc = QStringLiteral("%1").arg(hundredths, 2, 10, QLatin1Char('0'));
    // Whole seconds shown as "m:ss" below one hour, as in countdown() but without a leading zero
    // on the minutes.
    const int seconds = int(ms / 1000);
    QString whole = countdown(seconds);
    if (seconds < 3600 && whole.startsWith(QLatin1Char('0')))
        whole.remove(0, 1);
    return whole + QLatin1Char('.') + cc;
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "statusline.h"

#include <QFile>

namespace StatusLine {

QString timeText(const QDateTime &now, const QLocale &locale)
{
    return locale.toString(now.time(), QLocale::ShortFormat);
}

QString dateText(const QDateTime &now, const QLocale &locale)
{
    return locale.toString(now.date(), QStringLiteral("ddd d MMM"));
}

qint64 msToNextMinute(const QDateTime &now)
{
    const QTime t = now.time();
    const qint64 into = qint64(t.second()) * 1000 + t.msec();
    return qMax<qint64>(1, 60000 - into);
}

int parseBattery(const QByteArray &text)
{
    bool ok = false;
    const int value = text.trimmed().toInt(&ok);
    return ok && value >= 0 && value <= 100 ? value : -1;
}

int readBattery(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return -1;
    return parseBattery(file.read(16));
}

}

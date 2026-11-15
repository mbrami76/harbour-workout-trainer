// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef STATUSLINE_H
#define STATUSLINE_H

#include <QByteArray>
#include <QDateTime>
#include <QLocale>
#include <QString>

// The values of the status line of the session screen (R-UI-15). Pure logic apart from reading the
// battery file.
namespace StatusLine {

// The file that holds the charge in percent.
const char batteryFile[] = "/sys/class/power_supply/battery/capacity";

// The time of day in the short format of the locale.
QString timeText(const QDateTime &now, const QLocale &locale);
// The short date of the locale: the weekday, the day and the month, abbreviated.
QString dateText(const QDateTime &now, const QLocale &locale);
// Milliseconds from the given time to the start of the next minute, at least 1.
qint64 msToNextMinute(const QDateTime &now);
// The charge in percent from the text of the battery file, or -1 when it is not a number from 0 to 100.
int parseBattery(const QByteArray &text);
// The charge read from a file; -1 when the file cannot be read or holds no valid value.
int readBattery(const QString &path);

}

#endif

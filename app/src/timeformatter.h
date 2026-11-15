// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef TIMEFORMATTER_H
#define TIMEFORMATTER_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

// Turns milliseconds into the digits of a timer, with the formatter of the core (TimeFormat), so
// that no mode formats time on its own. Exposed to QML as the context property "timeFormatter".
class TimeFormatter : public QObject
{
    Q_OBJECT

public:
    explicit TimeFormatter(QObject *parent = nullptr) : QObject(parent) {}

    // Count-down text: a time that is not over shows the next whole second, so that "0:00" appears
    // only when the period has ended. "mm:ss", or "h:mm:ss" from one hour.
    Q_INVOKABLE QString countdown(qint64 ms) const;
    // Direct text: the whole seconds that have passed.
    Q_INVOKABLE QString elapsed(qint64 ms) const;
    // The parts of the digits (R-UI-9) as a list of {text, large}, for the count-down rounding and
    // for the direct rounding.
    Q_INVOKABLE QVariantList countdownParts(qint64 ms) const;
    Q_INVOKABLE QVariantList elapsedParts(qint64 ms) const;
    // A result with hundredths, "m:ss.cc".
    Q_INVOKABLE QString precise(qint64 ms) const;
    // The time since an event as {kind, major, minor}: kind is "now", "minutes", "hoursMinutes" or
    // "daysHours" (R-UI-17).
    Q_INVOKABLE QVariantMap ago(qint64 ms) const;
};

#endif

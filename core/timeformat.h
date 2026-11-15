// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef TIMEFORMAT_H
#define TIMEFORMAT_H

#include <QList>
#include <QString>

// Formatting of durations for display.
namespace TimeFormat {

// Countdown text for a whole number of seconds: "mm:ss" below one hour,
// "h:mm:ss" from 3600 s. A negative value is shown as zero.
QString countdown(int seconds);

// One piece of the digits of a time, with its separator, and whether it is drawn large (R-UI-9).
struct Part {
    QString text;
    bool large = false;
};

// The time as parts on one line. From 1 h the hours are large (as many digits as needed) and
// ":mm:ss" is small; from 1 min to under 1 h the minutes are large (one digit under 10 min) and
// ":ss" is small; under 1 min "0:" is small and the seconds are large. The seconds always have two
// digits, and no hours are shown below one hour. A negative value is zero.
QList<Part> parts(int seconds);

// A result with hundredths: "m:ss.cc", or "h:mm:ss.cc" from one hour. Milliseconds are cut, not
// rounded, to the hundredth. A negative value is zero.
QString precise(qint64 ms);

// The time since an event, as the largest unit that fits and the next smaller one (R-UI-17).
struct Ago {
    enum Kind { JustNow, Minutes, HoursMinutes, DaysHours };
    Kind kind = JustNow;
    int major = 0;
    int minor = 0;
};

// Under 1 min: JustNow; under 1 h: minutes; under 1 d: hours and minutes; from 1 d: days and
// hours. Parts are cut, not rounded. A negative value is JustNow.
Ago ago(qint64 ms);

}

#endif

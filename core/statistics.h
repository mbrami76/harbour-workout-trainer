// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef STATISTICS_H
#define STATISTICS_H

#include <QDate>
#include <QList>
#include <QTimeZone>

#include "database.h"
#include "model.h"

// Figures of the dashboard over the session log (R-DSH-1 to R-DSH-4). The clock and the time zone
// are arguments: nothing here reads the current time. Only steps with an exercise count (work,
// warm-up and cool-down); rest steps and steps of 0 s do not. A session belongs to the calendar
// day, in the given zone, on which it started.
namespace Statistics {

// A span of whole calendar days, first to last inclusive, with the exercise time it holds.
struct Period {
    QDate firstDay;
    QDate lastDay;
    // Days of the span that have started (7 for a week, the days up to today for the current month).
    int days = 0;
    qint64 totalSec = 0;
    // Minutes per day: the total divided by all the days of the span, rest days included (R-DSH-2).
    double averageMinutes = 0.0;
};

struct GroupTime {
    MuscleGroup group = MuscleGroup::Core;
    qint64 seconds = 0;
};

struct Overview {
    // The last 7 days (today and the 6 before) first, then the 4 spans of 7 days before them.
    QList<Period> weeks;
    // The current month so far first, then the 6 calendar months before it.
    QList<Period> months;
    // For the last 7 days, over the active groups (R-DSH-3): at most 5 with time, most first, and
    // the 5 with the least, least first.
    QList<GroupTime> mostTrained;
    QList<GroupTime> leastTrained;
    // Sessions that started in the last 7 days.
    int sessionsInWeek = 0;
    bool failed = false;
};

const int weekSpans = 5;
const int monthSpans = 7;
const int listLength = 5;
const int suggestionSessions = 5;

Overview overview(Database *database, qint64 nowMs, const QTimeZone &zone);

// R-DSH-4: the 5 active groups with the least time over the last 5 logged sessions, ranked as in
// R-DSH-3. Returns false when the log holds fewer than 5 sessions or cannot be read.
bool suggestedGroups(Database *database, MuscleSet *groups);

// First instant of a calendar day in the zone, in ms since the epoch. A day that starts in a gap of a
// daylight-saving change starts at the first instant after the gap.
qint64 startOfDayMs(const QDate &day, const QTimeZone &zone);

}

#endif

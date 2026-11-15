// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "statistics.h"

#include <QDateTime>
#include <QHash>
#include <QSqlQuery>
#include <algorithm>

#include "musclegroups.h"

namespace {

using namespace Statistics;

// The steps that count: an exercise, not a rest, some time.
const char *const countedSteps =
    "st.type <> 'rest' AND st.exercise_name <> '' AND st.actual_sec > 0";

// The last instant (ms of the epoch) at which each group was trained; absent for a group never trained.
bool lastTrained(Database *database, QHash<int, qint64> *last)
{
    QSqlQuery q;
    if (!database->run(QStringLiteral("SELECT st.muscles, MAX(s.started_at) FROM session_step st "
                                      "JOIN session s ON s.id = st.session_id WHERE %1 AND st.muscles <> 0 "
                                      "GROUP BY st.muscles").arg(QLatin1String(countedSteps)),
                       QVariantList(), &q))
        return false;
    const MuscleSet active = activeMuscles();
    while (q.next()) {
        const MuscleSet set = q.value(0).toULongLong() & active;
        const qint64 at = q.value(1).toLongLong();
        for (int position = 0; position < muscleGroupCount(); ++position) {
            if (set & (MuscleSet(1) << position) && at > last->value(position, -1))
                last->insert(position, at);
        }
    }
    return true;
}

// Ranks the active groups by time, least first, ties by the group trained longest ago (a group never
// trained is the longest ago), then by position.
QList<GroupTime> rank(const QHash<int, qint64> &seconds, const QHash<int, qint64> &last)
{
    QList<GroupTime> all;
    const MuscleSet active = activeMuscles();
    for (int position = 0; position < muscleGroupCount(); ++position) {
        if (!(active & (MuscleSet(1) << position)))
            continue;
        GroupTime g;
        g.group = static_cast<MuscleGroup>(position);
        g.seconds = seconds.value(position, 0);
        all.append(g);
    }
    std::stable_sort(all.begin(), all.end(), [&last](const GroupTime &a, const GroupTime &b) {
        if (a.seconds != b.seconds)
            return a.seconds < b.seconds;
        const qint64 la = last.value(static_cast<int>(a.group), -1);
        const qint64 lb = last.value(static_cast<int>(b.group), -1);
        return la < lb;
    });
    return all;
}

void addGroups(QHash<int, qint64> *seconds, MuscleSet set, qint64 sec)
{
    set &= activeMuscles();
    for (int position = 0; position < muscleGroupCount(); ++position) {
        if (set & (MuscleSet(1) << position))
            (*seconds)[position] += sec;
    }
}

Period makePeriod(const QDate &first, const QDate &last, int days)
{
    Period p;
    p.firstDay = first;
    p.lastDay = last;
    p.days = days;
    return p;
}

}

qint64 Statistics::startOfDayMs(const QDate &day, const QTimeZone &zone)
{
    QDateTime start(day, QTime(0, 0), zone);
    if (!start.isValid()) {
        // Midnight does not exist (a change that skips it): the day starts at the first instant after the gap.
        start = QDateTime(day, QTime(1, 0), zone);
    }
    return start.toMSecsSinceEpoch();
}

Overview Statistics::overview(Database *database, qint64 nowMs, const QTimeZone &zone)
{
    Overview result;
    const QDate today = QDateTime::fromMSecsSinceEpoch(nowMs, zone).date();
    for (int i = 0; i < weekSpans; ++i) {
        const QDate last = today.addDays(-7 * i);
        result.weeks.append(makePeriod(last.addDays(-6), last, 7));
    }
    const QDate monthStart(today.year(), today.month(), 1);
    for (int i = 0; i < monthSpans; ++i) {
        const QDate first = monthStart.addMonths(-i);
        result.months.append(i == 0 ? makePeriod(first, today, today.day())
                                    : makePeriod(first, first.addMonths(1).addDays(-1), first.daysInMonth()));
    }

    QDate oldest = result.weeks.last().firstDay;
    if (result.months.last().firstDay < oldest)
        oldest = result.months.last().firstDay;
    const qint64 from = startOfDayMs(oldest, zone);
    const qint64 to = startOfDayMs(today.addDays(1), zone);

    // The boundaries of every span in ms.
    QList<qint64> weekFrom, weekTo, monthFrom, monthTo;
    for (const Period &p : result.weeks) {
        weekFrom << startOfDayMs(p.firstDay, zone);
        weekTo << startOfDayMs(p.lastDay.addDays(1), zone);
    }
    for (const Period &p : result.months) {
        monthFrom << startOfDayMs(p.firstDay, zone);
        monthTo << startOfDayMs(p.lastDay.addDays(1), zone);
    }

    QSqlQuery q;
    if (!database->run(QStringLiteral("SELECT s.started_at, st.actual_sec, st.muscles FROM session s "
                                      "JOIN session_step st ON st.session_id = s.id "
                                      "WHERE s.started_at >= ? AND s.started_at < ? AND %1")
                           .arg(QLatin1String(countedSteps)),
                       QVariantList() << from << to, &q)) {
        result.failed = true;
        return result;
    }
    QHash<int, qint64> weekSeconds;
    while (q.next()) {
        const qint64 at = q.value(0).toLongLong();
        const qint64 sec = q.value(1).toLongLong();
        const MuscleSet muscles = q.value(2).toULongLong();
        for (int i = 0; i < weekSpans; ++i) {
            if (at >= weekFrom.at(i) && at < weekTo.at(i))
                result.weeks[i].totalSec += sec;
        }
        for (int i = 0; i < monthSpans; ++i) {
            if (at >= monthFrom.at(i) && at < monthTo.at(i))
                result.months[i].totalSec += sec;
        }
        if (at >= weekFrom.at(0) && at < weekTo.at(0))
            addGroups(&weekSeconds, muscles, sec);
    }
    for (Period &p : result.weeks)
        p.averageMinutes = p.totalSec / 60.0 / p.days;
    for (Period &p : result.months)
        p.averageMinutes = p.totalSec / 60.0 / p.days;

    if (!database->run(QStringLiteral("SELECT COUNT(*) FROM session WHERE started_at >= ? AND started_at < ?"),
                       QVariantList() << weekFrom.at(0) << weekTo.at(0), &q) || !q.next()) {
        result.failed = true;
        return result;
    }
    result.sessionsInWeek = q.value(0).toInt();

    QHash<int, qint64> last;
    if (!lastTrained(database, &last)) {
        result.failed = true;
        return result;
    }
    const QList<GroupTime> ranked = rank(weekSeconds, last);
    for (int i = 0; i < ranked.size() && result.leastTrained.size() < listLength; ++i)
        result.leastTrained.append(ranked.at(i));
    // The most trained: by time, most first; ties by the group trained longest ago, then position.
    QList<GroupTime> byTime;
    for (const GroupTime &g : ranked) {
        if (g.seconds > 0)
            byTime.append(g);
    }
    std::stable_sort(byTime.begin(), byTime.end(), [&last](const GroupTime &a, const GroupTime &b) {
        if (a.seconds != b.seconds)
            return a.seconds > b.seconds;
        return last.value(static_cast<int>(a.group), -1) < last.value(static_cast<int>(b.group), -1);
    });
    for (int i = 0; i < byTime.size() && i < listLength; ++i)
        result.mostTrained.append(byTime.at(i));
    return result;
}

bool Statistics::suggestedGroups(Database *database, MuscleSet *groups)
{
    QSqlQuery q;
    if (!database->run(QStringLiteral("SELECT id FROM session ORDER BY started_at DESC, id DESC LIMIT %1")
                           .arg(suggestionSessions), QVariantList(), &q))
        return false;
    QList<int> ids;
    while (q.next())
        ids.append(q.value(0).toInt());
    if (ids.size() < suggestionSessions)
        return false;
    QStringList marks;
    for (int id : ids)
        marks << QString::number(id);
    if (!database->run(QStringLiteral("SELECT st.muscles, SUM(st.actual_sec) FROM session_step st WHERE "
                                      "st.session_id IN (%1) AND %2 GROUP BY st.muscles")
                           .arg(marks.join(QLatin1Char(',')), QLatin1String(countedSteps)),
                       QVariantList(), &q))
        return false;
    QHash<int, qint64> seconds;
    while (q.next())
        addGroups(&seconds, q.value(0).toULongLong(), q.value(1).toLongLong());
    QHash<int, qint64> last;
    if (!lastTrained(database, &last))
        return false;
    const QList<GroupTime> ranked = rank(seconds, last);
    MuscleSet set = 0;
    for (int i = 0; i < ranked.size() && i < listLength; ++i)
        set |= muscleBit(ranked.at(i).group);
    *groups = set;
    return true;
}

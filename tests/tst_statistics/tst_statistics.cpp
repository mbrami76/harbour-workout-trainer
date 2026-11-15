// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "musclegroups.h"
#include "sessionlog.h"
#include "statistics.h"
#include "testdb.h"

namespace {

const QTimeZone utc(QByteArrayLiteral("UTC"));

qint64 at(const QDate &d, int hour, int minute, const QTimeZone &zone)
{
    return QDateTime(d, QTime(hour, minute), zone).toMSecsSinceEpoch();
}

MuscleSet bits(std::initializer_list<MuscleGroup> groups)
{
    MuscleSet s = 0;
    for (MuscleGroup g : groups)
        s |= muscleBit(g);
    return s;
}

LoggedStep step(int position, MuscleSet muscles, int sec, StepType type = StepType::Work, const QString &name = QStringLiteral("Move"))
{
    LoggedStep s;
    s.position = position;
    s.exerciseName = name;
    s.muscles = muscles;
    s.type = type;
    s.actualSec = sec;
    return s;
}

}

class TstStatistics : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void emptyLogGivesZeroes();
    void spansAreNamedByTheirDays();
    void oneSessionCountsInTodayAndTheMonth();
    void restAndZeroAndNamelessStepsDoNotCount();
    void warmupAndCooldownCount();
    void stoppedSessionsCount();
    void midnightBoundariesOfTheDay();
    void sixDaysBeforeIsInsideSevenDaysBeforeIsOut();
    void weeksSplitAtSevenDayBoundaries();
    void monthBoundaries();
    void yearEnd();
    void zoneWithAnOffsetMovesTheDay();
    void daylightSavingDays();
    void averagePerDayUsesAllDaysOfTheSpan();
    void stepWithSeveralGroupsCountsForEach();
    void retiredGroupIsIgnored();
    void leastTrainedPutsUntrainedGroupsFirstThenLongestAgo();
    void mostTrainedSkipsGroupsWithoutTime();
    void tiesFollowTheOrderOfTheGroups();
    void suggestionNeedsFiveSessions();
    void suggestionUsesTheLastFiveSessionsOnly();
    void suggestionRanksLikeTheWeekList();

private:
    void add(qint64 startedAtMs, const QList<LoggedStep> &steps, EndState state = EndState::Completed);
    Statistics::Overview overviewAt(qint64 now, const QTimeZone &zone = utc);
    TestDb t_;
};

void TstStatistics::init()
{
    QVERIFY(t_.dir.isValid());
    t_.db.close();
    QDir dir(t_.dir.path());
    dir.removeRecursively();
    QVERIFY(t_.open());
}

void TstStatistics::add(qint64 startedAtMs, const QList<LoggedStep> &steps, EndState state)
{
    LogEntry e;
    e.workoutName = QStringLiteral("Test");
    e.startedAtMs = startedAtMs;
    e.endState = state;
    e.steps = steps;
    for (const LoggedStep &s : steps)
        e.totalSec += s.actualSec;
    QVERIFY(SessionLog(&t_.db).add(e) == StoreStatus::Ok);
}

Statistics::Overview TstStatistics::overviewAt(qint64 now, const QTimeZone &zone)
{
    const Statistics::Overview o = Statistics::overview(&t_.db, now, zone);
    return o;
}

void TstStatistics::emptyLogGivesZeroes()
{
    const Statistics::Overview o = overviewAt(at(QDate(2026, 5, 15), 12, 0, utc));
    QVERIFY(!o.failed);
    QCOMPARE(o.weeks.size(), 5);
    QCOMPARE(o.months.size(), 7);
    for (const Statistics::Period &p : o.weeks + o.months) {
        QCOMPARE(p.totalSec, qint64(0));
        QCOMPARE(p.averageMinutes, 0.0);
    }
    QCOMPARE(o.sessionsInWeek, 0);
    QVERIFY(o.mostTrained.isEmpty());
    // Every group is untrained: the least list holds the first five in the order of the groups.
    QCOMPARE(o.leastTrained.size(), 5);
    QCOMPARE(int(o.leastTrained.first().group), int(MuscleGroup::Shoulders));
    MuscleSet set = 0;
    QVERIFY(!Statistics::suggestedGroups(&t_.db, &set));
}

void TstStatistics::spansAreNamedByTheirDays()
{
    const Statistics::Overview o = overviewAt(at(QDate(2026, 5, 15), 12, 0, utc));
    QCOMPARE(o.weeks.at(0).firstDay, QDate(2026, 5, 9));
    QCOMPARE(o.weeks.at(0).lastDay, QDate(2026, 5, 15));
    QCOMPARE(o.weeks.at(1).firstDay, QDate(2026, 5, 2));
    QCOMPARE(o.weeks.at(1).lastDay, QDate(2026, 5, 8));
    QCOMPARE(o.weeks.at(4).firstDay, QDate(2026, 4, 11));
    QCOMPARE(o.weeks.at(4).lastDay, QDate(2026, 4, 17));
    QCOMPARE(o.months.at(0).firstDay, QDate(2026, 5, 1));
    QCOMPARE(o.months.at(0).lastDay, QDate(2026, 5, 15));
    QCOMPARE(o.months.at(0).days, 15);
    QCOMPARE(o.months.at(1).firstDay, QDate(2026, 4, 1));
    QCOMPARE(o.months.at(1).lastDay, QDate(2026, 4, 30));
    QCOMPARE(o.months.at(1).days, 30);
    QCOMPARE(o.months.at(6).firstDay, QDate(2025, 11, 1));
    QCOMPARE(o.months.at(6).lastDay, QDate(2025, 11, 30));
}

void TstStatistics::oneSessionCountsInTodayAndTheMonth()
{
    add(at(QDate(2026, 5, 15), 8, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 600));
    const Statistics::Overview o = overviewAt(at(QDate(2026, 5, 15), 12, 0, utc));
    QCOMPARE(o.weeks.at(0).totalSec, qint64(600));
    QCOMPARE(o.weeks.at(1).totalSec, qint64(0));
    QCOMPARE(o.months.at(0).totalSec, qint64(600));
    QCOMPARE(o.months.at(1).totalSec, qint64(0));
    QCOMPARE(o.sessionsInWeek, 1);
    QCOMPARE(o.mostTrained.size(), 1);
    QCOMPARE(int(o.mostTrained.first().group), int(MuscleGroup::Core));
    QCOMPARE(o.mostTrained.first().seconds, qint64(600));
}

void TstStatistics::restAndZeroAndNamelessStepsDoNotCount()
{
    add(at(QDate(2026, 5, 15), 8, 0, utc),
        QList<LoggedStep>() << step(0, 0, 60, StepType::Rest, QString())
                            << step(1, bits({MuscleGroup::Core}), 0)
                            << step(2, 0, 45, StepType::Work, QString())
                            << step(3, bits({MuscleGroup::Core}), 30, StepType::Rest)
                            << step(4, bits({MuscleGroup::Core}), 20));
    const Statistics::Overview o = overviewAt(at(QDate(2026, 5, 15), 12, 0, utc));
    QCOMPARE(o.weeks.at(0).totalSec, qint64(20));
}

void TstStatistics::warmupAndCooldownCount()
{
    add(at(QDate(2026, 5, 15), 8, 0, utc),
        QList<LoggedStep>() << step(0, bits({MuscleGroup::Calves}), 100, StepType::Warmup)
                            << step(1, bits({MuscleGroup::Calves}), 50, StepType::Cooldown));
    QCOMPARE(overviewAt(at(QDate(2026, 5, 15), 12, 0, utc)).weeks.at(0).totalSec, qint64(150));
}

void TstStatistics::stoppedSessionsCount()
{
    add(at(QDate(2026, 5, 15), 8, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 90), EndState::Stopped);
    QCOMPARE(overviewAt(at(QDate(2026, 5, 15), 12, 0, utc)).weeks.at(0).totalSec, qint64(90));
}

void TstStatistics::midnightBoundariesOfTheDay()
{
    const QDate today(2026, 5, 15);
    add(at(today, 0, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 10));
    add(at(today.addDays(-1), 23, 59, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 100));
    const Statistics::Overview o = overviewAt(at(today, 0, 0, utc));
    // On the first instant of the day, "now" is that day: both sessions are in the last 7 days.
    QCOMPARE(o.weeks.at(0).totalSec, qint64(110));
    // One minute before midnight the new day has not started: a session that starts later is not counted yet.
    const Statistics::Overview before = overviewAt(at(today.addDays(-1), 23, 59, utc));
    QCOMPARE(before.weeks.at(0).lastDay, today.addDays(-1));
    QCOMPARE(before.weeks.at(0).totalSec, qint64(100));
}

void TstStatistics::sixDaysBeforeIsInsideSevenDaysBeforeIsOut()
{
    const QDate today(2026, 5, 15);
    add(at(today.addDays(-6), 0, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 10));
    add(at(today.addDays(-7), 23, 59, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 1000));
    const Statistics::Overview o = overviewAt(at(today, 12, 0, utc));
    QCOMPARE(o.weeks.at(0).totalSec, qint64(10));
    QCOMPARE(o.weeks.at(1).totalSec, qint64(1000));
    // The group list covers only the last 7 days.
    QCOMPARE(o.mostTrained.first().seconds, qint64(10));
}

void TstStatistics::weeksSplitAtSevenDayBoundaries()
{
    const QDate today(2026, 5, 15);
    for (int w = 0; w < 5; ++w) {
        add(at(today.addDays(-7 * w), 10, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 10 * (w + 1)));
        add(at(today.addDays(-7 * w - 6), 10, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 1000 * (w + 1)));
    }
    // Older than five spans: ignored by the weeks.
    add(at(today.addDays(-35), 10, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 77777));
    const Statistics::Overview o = overviewAt(at(today, 12, 0, utc));
    for (int w = 0; w < 5; ++w)
        QCOMPARE(o.weeks.at(w).totalSec, qint64(10 * (w + 1) + 1000 * (w + 1)));
}

void TstStatistics::monthBoundaries()
{
    const QDate today(2026, 3, 1);
    add(at(QDate(2026, 3, 1), 0, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 1));
    add(at(QDate(2026, 2, 28), 23, 59, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 10));
    add(at(QDate(2026, 2, 1), 0, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 100));
    add(at(QDate(2026, 1, 31), 23, 59, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 1000));
    add(at(QDate(2025, 9, 1), 0, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 10000));
    add(at(QDate(2025, 8, 31), 23, 59, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 99999));
    const Statistics::Overview o = overviewAt(at(today, 12, 0, utc));
    QCOMPARE(o.months.at(0).days, 1);
    QCOMPARE(o.months.at(0).totalSec, qint64(1));
    QCOMPARE(o.months.at(1).lastDay, QDate(2026, 2, 28));
    QCOMPARE(o.months.at(1).totalSec, qint64(110));
    QCOMPARE(o.months.at(2).totalSec, qint64(1000));
    QCOMPARE(o.months.at(6).firstDay, QDate(2025, 9, 1));
    QCOMPARE(o.months.at(6).totalSec, qint64(10000));
}

void TstStatistics::yearEnd()
{
    add(at(QDate(2025, 12, 31), 23, 59, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 5));
    add(at(QDate(2026, 1, 1), 0, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 7));
    const Statistics::Overview o = overviewAt(at(QDate(2026, 1, 2), 12, 0, utc));
    QCOMPARE(o.months.at(0).firstDay, QDate(2026, 1, 1));
    QCOMPARE(o.months.at(0).totalSec, qint64(7));
    QCOMPARE(o.months.at(1).firstDay, QDate(2025, 12, 1));
    QCOMPARE(o.months.at(1).lastDay, QDate(2025, 12, 31));
    QCOMPARE(o.months.at(1).totalSec, qint64(5));
    QCOMPARE(o.weeks.at(0).totalSec, qint64(12));
}

void TstStatistics::zoneWithAnOffsetMovesTheDay()
{
    const QTimeZone plus(10 * 3600);
    // 23:30 UTC on 14 May is 09:30 on 15 May at +10 h.
    const qint64 start = at(QDate(2026, 5, 14), 23, 30, utc);
    add(start, QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 60));
    const qint64 now = at(QDate(2026, 5, 15), 12, 0, plus);
    const Statistics::Overview inPlus = overviewAt(now, plus);
    QCOMPARE(inPlus.weeks.at(0).lastDay, QDate(2026, 5, 15));
    QCOMPARE(inPlus.months.at(0).totalSec, qint64(60));
    // The same instant seen in UTC is the previous month's last day only at a month change: here 1 June at +10 h.
    const qint64 early = at(QDate(2026, 5, 31), 20, 0, utc);
    add(early, QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 1000));
    const Statistics::Overview june = overviewAt(at(QDate(2026, 6, 1), 12, 0, plus), plus);
    QCOMPARE(june.months.at(0).totalSec, qint64(1000));
    const Statistics::Overview juneUtc = overviewAt(at(QDate(2026, 6, 1), 12, 0, utc), utc);
    QCOMPARE(juneUtc.months.at(0).totalSec, qint64(0));
}

void TstStatistics::daylightSavingDays()
{
    const QTimeZone rome(QByteArrayLiteral("Europe/Rome"));
    if (!rome.isValid())
        QSKIP("time zone data not available");
    // 29 March 2026: the clocks go forward at 02:00, the day has 23 hours. 25 October 2026: 25 hours.
    const QDate spring(2026, 3, 29);
    QCOMPARE((Statistics::startOfDayMs(spring.addDays(1), rome) - Statistics::startOfDayMs(spring, rome)) / 3600000, qint64(23));
    const QDate autumn(2026, 10, 25);
    QCOMPARE((Statistics::startOfDayMs(autumn.addDays(1), rome) - Statistics::startOfDayMs(autumn, rome)) / 3600000, qint64(25));
    // A session at 00:30 on the day of the change and one at 23:30 the day before.
    add(at(spring, 0, 30, rome), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 3));
    add(at(spring.addDays(-1), 23, 30, rome), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 40));
    const Statistics::Overview o = overviewAt(at(spring, 22, 0, rome), rome);
    QCOMPARE(o.weeks.at(0).lastDay, spring);
    QCOMPARE(o.weeks.at(0).totalSec, qint64(43));
    // Seen from the day before, only the earlier session counts and the later one has not happened.
    const Statistics::Overview before = overviewAt(at(spring.addDays(-1), 23, 45, rome), rome);
    QCOMPARE(before.weeks.at(0).totalSec, qint64(40));
    // Autumn: a session in the repeated hour belongs to the day of the change.
    add(at(autumn, 23, 30, rome), QList<LoggedStep>() << step(0, bits({MuscleGroup::Calves}), 500));
    const Statistics::Overview fall = overviewAt(at(autumn, 23, 45, rome), rome);
    QCOMPARE(fall.weeks.at(0).totalSec, qint64(500));
}

void TstStatistics::averagePerDayUsesAllDaysOfTheSpan()
{
    add(at(QDate(2026, 5, 15), 8, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 1260));
    add(at(QDate(2026, 4, 10), 8, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 3000));
    const Statistics::Overview o = overviewAt(at(QDate(2026, 5, 15), 12, 0, utc));
    QCOMPARE(o.weeks.at(0).averageMinutes, 3.0);
    // 21 min over 15 days so far.
    QCOMPARE(o.months.at(0).averageMinutes, 21.0 / 15.0);
    // 50 min over the 30 days of April.
    QCOMPARE(o.months.at(1).averageMinutes, 50.0 / 30.0);
}

void TstStatistics::stepWithSeveralGroupsCountsForEach()
{
    add(at(QDate(2026, 5, 15), 8, 0, utc),
        QList<LoggedStep>() << step(0, bits({MuscleGroup::Core, MuscleGroup::Glutes, MuscleGroup::Obliques}), 120));
    const Statistics::Overview o = overviewAt(at(QDate(2026, 5, 15), 12, 0, utc));
    QCOMPARE(o.weeks.at(0).totalSec, qint64(120));
    QCOMPARE(o.mostTrained.size(), 3);
    for (const Statistics::GroupTime &g : o.mostTrained)
        QCOMPARE(g.seconds, qint64(120));
}

void TstStatistics::retiredGroupIsIgnored()
{
    add(at(QDate(2026, 5, 15), 8, 0, utc),
        QList<LoggedStep>() << step(0, bits({MuscleGroup::Chest, MuscleGroup::Shins}), 60));
    const Statistics::Overview o = overviewAt(at(QDate(2026, 5, 15), 12, 0, utc));
    QCOMPARE(o.weeks.at(0).totalSec, qint64(60));
    QCOMPARE(o.mostTrained.size(), 1);
    QCOMPARE(int(o.mostTrained.first().group), int(MuscleGroup::Shins));
    for (const Statistics::GroupTime &g : o.leastTrained)
        QVERIFY(!MuscleKeys::isRetired(g.group));
}

void TstStatistics::leastTrainedPutsUntrainedGroupsFirstThenLongestAgo()
{
    const QDate today(2026, 5, 15);
    // Every active group is trained 100 s in the last 7 days, except Glutes (0 s, trained 3 weeks ago)
    // and Calves (0 s, trained 10 weeks ago) and Shins (never).
    MuscleSet all = activeMuscles();
    all &= ~bits({MuscleGroup::Glutes, MuscleGroup::Calves, MuscleGroup::Shins});
    add(at(today, 8, 0, utc), QList<LoggedStep>() << step(0, all, 100));
    add(at(today.addDays(-21), 8, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Glutes}), 10));
    add(at(today.addDays(-70), 8, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Calves}), 10));
    const Statistics::Overview o = overviewAt(at(today, 12, 0, utc));
    QCOMPARE(o.leastTrained.size(), 5);
    // Never trained first, then trained longest ago, then the group trained 3 weeks ago.
    QCOMPARE(int(o.leastTrained.at(0).group), int(MuscleGroup::Shins));
    QCOMPARE(int(o.leastTrained.at(1).group), int(MuscleGroup::Calves));
    QCOMPARE(int(o.leastTrained.at(2).group), int(MuscleGroup::Glutes));
    QCOMPARE(o.leastTrained.at(2).seconds, qint64(0));
    // The next two have 100 s: the one trained longest ago among them comes first (all equal: the order of the groups).
    QCOMPARE(o.leastTrained.at(3).seconds, qint64(100));
}

void TstStatistics::mostTrainedSkipsGroupsWithoutTime()
{
    const QDate today(2026, 5, 15);
    add(at(today, 8, 0, utc),
        QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 500) << step(1, bits({MuscleGroup::Glutes}), 400)
                            << step(2, bits({MuscleGroup::Calves}), 300) << step(3, bits({MuscleGroup::Lats}), 200)
                            << step(4, bits({MuscleGroup::Biceps}), 100) << step(5, bits({MuscleGroup::Triceps}), 50));
    const Statistics::Overview o = overviewAt(at(today, 12, 0, utc));
    QCOMPARE(o.mostTrained.size(), 5);
    QCOMPARE(int(o.mostTrained.at(0).group), int(MuscleGroup::Core));
    QCOMPARE(int(o.mostTrained.at(4).group), int(MuscleGroup::Biceps));
}

void TstStatistics::tiesFollowTheOrderOfTheGroups()
{
    // Equal time and trained at the same moment: the order of the groups decides.
    add(at(QDate(2026, 5, 15), 8, 0, utc),
        QList<LoggedStep>() << step(0, bits({MuscleGroup::Hamstrings, MuscleGroup::Glutes, MuscleGroup::Core}), 60));
    const Statistics::Overview o = overviewAt(at(QDate(2026, 5, 15), 12, 0, utc));
    QCOMPARE(int(o.mostTrained.at(0).group), int(MuscleGroup::Core));
    QCOMPARE(int(o.mostTrained.at(1).group), int(MuscleGroup::Glutes));
    QCOMPARE(int(o.mostTrained.at(2).group), int(MuscleGroup::Hamstrings));
}

void TstStatistics::suggestionNeedsFiveSessions()
{
    const QDate today(2026, 5, 15);
    MuscleSet set = 0;
    for (int i = 0; i < 4; ++i)
        add(at(today.addDays(-i), 8, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 60));
    QVERIFY(!Statistics::suggestedGroups(&t_.db, &set));
    add(at(today.addDays(-4), 8, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Core}), 60), EndState::Stopped);
    QVERIFY(Statistics::suggestedGroups(&t_.db, &set));
    // Core is trained, so it is not among the five least.
    QVERIFY(!(set & muscleBit(MuscleGroup::Core)));
    int count = 0;
    for (int p = 0; p < 63; ++p)
        count += (set >> p) & 1;
    QCOMPARE(count, 5);
}

void TstStatistics::suggestionUsesTheLastFiveSessionsOnly()
{
    const QDate today(2026, 5, 15);
    // An old session trains Shins; the last five train all active groups except Shins and Calves.
    add(at(today.addDays(-60), 8, 0, utc), QList<LoggedStep>() << step(0, bits({MuscleGroup::Shins}), 5000));
    MuscleSet most = activeMuscles() & ~bits({MuscleGroup::Shins, MuscleGroup::Calves});
    for (int i = 0; i < 5; ++i)
        add(at(today.addDays(-i), 8, 0, utc), QList<LoggedStep>() << step(0, most, 60));
    MuscleSet set = 0;
    QVERIFY(Statistics::suggestedGroups(&t_.db, &set));
    QVERIFY(set & muscleBit(MuscleGroup::Shins));
    QVERIFY(set & muscleBit(MuscleGroup::Calves));
}

void TstStatistics::suggestionRanksLikeTheWeekList()
{
    const QDate today(2026, 5, 15);
    for (int i = 0; i < 5; ++i)
        add(at(today.addDays(-i), 8, 0, utc), QList<LoggedStep>() << step(0, activeMuscles() & ~bits({MuscleGroup::Lats}), 60));
    MuscleSet set = 0;
    QVERIFY(Statistics::suggestedGroups(&t_.db, &set));
    // Lats has no time: first. The other four are equal at 300 s and never differ in last time: the order of the groups.
    QVERIFY(set & muscleBit(MuscleGroup::Lats));
    const Statistics::Overview o = overviewAt(at(today, 12, 0, utc));
    MuscleSet fromList = 0;
    for (const Statistics::GroupTime &g : o.leastTrained)
        fromList |= muscleBit(g.group);
    QCOMPARE(set, fromList);
}

QTEST_APPLESS_MAIN(TstStatistics)
#include "tst_statistics.moc"

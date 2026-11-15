// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "fakeclock.h"
#include "intervaltimer.h"

namespace {

typedef QList<IntervalEvent> Events;

int count(const Events &events, IntervalEventType type)
{
    int n = 0;
    for (const IntervalEvent &e : events) {
        if (e.type == type)
            ++n;
    }
    return n;
}

Events ofType(const Events &events, IntervalEventType type)
{
    Events found;
    for (const IntervalEvent &e : events) {
        if (e.type == type)
            found.append(e);
    }
    return found;
}

// Runs the timer in steps of stepMs for totalMs and collects every event.
Events run(IntervalTimer &timer, FakeClock &clock, qint64 totalMs, qint64 stepMs)
{
    Events all;
    for (qint64 t = 0; t < totalMs; t += stepMs) {
        clock.advance(qMin(stepMs, totalMs - t));
        all += timer.update();
    }
    return all;
}

}

class TstIntervalTimer : public QObject
{
    Q_OBJECT

private slots:
    void limitsAreEnforced();
    void invalidTimerDoesNotStart();
    void startsAtOnceWithWork();
    void periodsAlternateWithoutEnd();
    void zeroRecoveryMeansWorkOnly();
    void periodStartedAndChangedEvents();
    void eventTimesAreScheduledTimes();
    void ticksAtTheRightRemainingTimes();
    void ticksSkipPeriodsThatAreTooShort();
    void ticksAreConfigurable();
    void ticksRepeatInEveryPeriod();
    void tenThousandPeriodsDoNotDrift();
    void irregularUpdatesDoNotDrift();
    void oneLateUpdateCatchesUp();
    void remainingAndElapsedTimes();
    void completedWorkPeriodsAreCounted();
    void pauseDuringWork();
    void pauseDuringRecovery();
    void pauseDoesNotRepeatTicks();
    void pauseBetweenTicksKeepsTheRest();
    void pauseAtTheBoundary();
    void pauseWhenNotRunningDoesNothing();
    void stopFromRunningAndPaused();
    void stopReportsEarlierEventsFirst();
    void stoppedTimerStaysStopped();
    void startTwiceDoesNothing();
    void limitCases();
};

void TstIntervalTimer::limitsAreEnforced()
{
    QVERIFY(IntervalTimer::isValid(1, 0));
    QVERIFY(IntervalTimer::isValid(3600, 600));
    QVERIFY(IntervalTimer::isValid(30, 10));
    QVERIFY(!IntervalTimer::isValid(0, 10));
    QVERIFY(!IntervalTimer::isValid(-1, 10));
    QVERIFY(!IntervalTimer::isValid(3601, 10));
    QVERIFY(!IntervalTimer::isValid(30, -1));
    QVERIFY(!IntervalTimer::isValid(30, 601));
}

void TstIntervalTimer::invalidTimerDoesNotStart()
{
    FakeClock clock;
    const int bad[][2] = { { 0, 10 }, { 3601, 10 }, { 30, -1 }, { 30, 601 } };
    for (const auto &pair : bad) {
        IntervalTimer timer(&clock, pair[0], pair[1]);
        QVERIFY(!timer.valid());
        QVERIFY(timer.start().isEmpty());
        QVERIFY(timer.state() == IntervalState::Ready);
        clock.advance(100000);
        QVERIFY(timer.update().isEmpty());
        QCOMPARE(timer.periodCount(), 0);
    }
    IntervalTimer good(&clock, 30, 10);
    QVERIFY(good.valid());
}

void TstIntervalTimer::startsAtOnceWithWork()
{
    FakeClock clock;
    clock.set(4000);
    IntervalTimer timer(&clock, 20, 10);
    QVERIFY(timer.state() == IntervalState::Ready);
    const Events events = timer.start();
    QVERIFY(timer.state() == IntervalState::Running);
    QVERIFY(timer.periodKind() == PeriodKind::Work);
    QCOMPARE(events.size(), 1);
    QVERIFY(events.first().type == IntervalEventType::PeriodStarted);
    QVERIFY(events.first().kind == PeriodKind::Work);
    QCOMPARE(events.first().atMs, qint64(4000));
    QCOMPARE(events.first().periodIndex, 0);
    QCOMPARE(timer.periodRemainingMs(), qint64(20000));
    QCOMPARE(timer.periodCount(), 1);
}

void TstIntervalTimer::periodsAlternateWithoutEnd()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 5, 3);
    timer.start();
    const Events events = run(timer, clock, 8 * 100 * 1000, 250);
    const Events started = ofType(events, IntervalEventType::PeriodStarted);
    QCOMPARE(started.size(), 200);
    for (int i = 0; i < started.size(); ++i) {
        QCOMPARE(started.at(i).periodIndex, i + 1);
        QVERIFY(started.at(i).kind == (i % 2 == 0 ? PeriodKind::Recovery : PeriodKind::Work));
    }
    QVERIFY(timer.state() == IntervalState::Running);
}

void TstIntervalTimer::zeroRecoveryMeansWorkOnly()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 4, 0);
    Events events = timer.start();
    events += run(timer, clock, 40000, 100);
    const Events started = ofType(events, IntervalEventType::PeriodStarted);
    QCOMPARE(started.size(), 11);
    for (const IntervalEvent &e : started)
        QVERIFY(e.kind == PeriodKind::Work);
    // No change of kind, so no change event.
    QCOMPARE(count(events, IntervalEventType::PeriodChanged), 0);
    QCOMPARE(timer.completedWorkPeriods(), 10);
    QVERIFY(timer.periodKind() == PeriodKind::Work);
}

void TstIntervalTimer::periodStartedAndChangedEvents()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 5, 3);
    Events first = timer.start();
    QCOMPARE(count(first, IntervalEventType::PeriodChanged), 0);
    clock.advance(5000);
    const Events events = timer.update();
    // At the end of work: the change comes first, then the start of the recovery.
    const Events boundary = ofType(events, IntervalEventType::PeriodChanged);
    QCOMPARE(boundary.size(), 1);
    int changedAt = -1, startedAt = -1;
    for (int i = 0; i < events.size(); ++i) {
        if (events.at(i).type == IntervalEventType::PeriodChanged)
            changedAt = i;
        if (events.at(i).type == IntervalEventType::PeriodStarted)
            startedAt = i;
    }
    QVERIFY(changedAt >= 0 && startedAt > changedAt);
    QVERIFY(events.at(changedAt).kind == PeriodKind::Recovery);
    QVERIFY(events.at(startedAt).kind == PeriodKind::Recovery);
    QCOMPARE(events.at(startedAt).periodIndex, 1);
    clock.advance(3000);
    const Events back = timer.update();
    QCOMPARE(count(back, IntervalEventType::PeriodChanged), 1);
    QVERIFY(ofType(back, IntervalEventType::PeriodChanged).first().kind == PeriodKind::Work);
}

void TstIntervalTimer::eventTimesAreScheduledTimes()
{
    FakeClock clock;
    clock.set(1000);
    IntervalTimer timer(&clock, 5, 3);
    timer.start();
    // One late call reports the events at their scheduled times.
    clock.advance(20000);
    const Events events = timer.update();
    const Events started = ofType(events, IntervalEventType::PeriodStarted);
    QCOMPARE(started.size(), 4);
    QCOMPARE(started.at(0).atMs, qint64(6000));
    QCOMPARE(started.at(1).atMs, qint64(9000));
    QCOMPARE(started.at(2).atMs, qint64(14000));
    QCOMPARE(started.at(3).atMs, qint64(17000));
    qint64 last = 0;
    for (const IntervalEvent &e : events) {
        QVERIFY(e.atMs >= last);
        last = e.atMs;
    }
}

void TstIntervalTimer::ticksAtTheRightRemainingTimes()
{
    FakeClock clock;
    clock.set(500);
    IntervalTimer timer(&clock, 30, 15);
    Events events = timer.start();
    events += run(timer, clock, 30000, 10);
    const Events ticks = ofType(events, IntervalEventType::Tick);
    QCOMPARE(ticks.size(), 5);
    const int seconds[] = { 10, 5, 3, 2, 1 };
    for (int i = 0; i < 5; ++i) {
        QCOMPARE(ticks.at(i).remainingSec, seconds[i]);
        QCOMPARE(ticks.at(i).atMs, qint64(500 + 30000 - seconds[i] * 1000));
        QVERIFY(ticks.at(i).kind == PeriodKind::Work);
        QCOMPARE(ticks.at(i).periodIndex, 0);
    }
    // The recovery ticks at its own end.
    Events recovery = run(timer, clock, 15000, 10);
    const Events recoveryTicks = ofType(recovery, IntervalEventType::Tick);
    QCOMPARE(recoveryTicks.size(), 5);
    QVERIFY(recoveryTicks.first().kind == PeriodKind::Recovery);
    QCOMPARE(recoveryTicks.first().atMs, qint64(500 + 30000 + 15000 - 10000));
}

void TstIntervalTimer::ticksSkipPeriodsThatAreTooShort()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 3, 2);
    Events events = timer.start();
    events += run(timer, clock, 5000, 10);
    // Work of 3 s: ticks at 2 and 1 (3 and more are not below the length). Recovery of 2 s: 1.
    const Events ticks = ofType(events, IntervalEventType::Tick);
    QCOMPARE(ticks.size(), 3);
    QCOMPARE(ticks.at(0).remainingSec, 2);
    QCOMPARE(ticks.at(1).remainingSec, 1);
    QVERIFY(ticks.at(0).kind == PeriodKind::Work);
    QCOMPARE(ticks.at(2).remainingSec, 1);
    QVERIFY(ticks.at(2).kind == PeriodKind::Recovery);

    FakeClock clock2;
    IntervalTimer one(&clock2, 1, 0);
    Events e2 = one.start();
    e2 += run(one, clock2, 3000, 10);
    QCOMPARE(count(e2, IntervalEventType::Tick), 0);
}

void TstIntervalTimer::ticksAreConfigurable()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 60, 0, QList<int>() << 1 << 30 << 15 << 30 << 0 << -4);
    Events events = timer.start();
    events += run(timer, clock, 60000, 50);
    const Events ticks = ofType(events, IntervalEventType::Tick);
    QCOMPARE(ticks.size(), 3);
    QCOMPARE(ticks.at(0).remainingSec, 30);
    QCOMPARE(ticks.at(1).remainingSec, 15);
    QCOMPARE(ticks.at(2).remainingSec, 1);

    FakeClock clock2;
    IntervalTimer none(&clock2, 20, 0, QList<int>());
    Events e2 = none.start();
    e2 += run(none, clock2, 20000, 50);
    QCOMPARE(count(e2, IntervalEventType::Tick), 0);
}

void TstIntervalTimer::ticksRepeatInEveryPeriod()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 12, 6);
    Events events = timer.start();
    events += run(timer, clock, 18000 * 4, 100);
    const Events ticks = ofType(events, IntervalEventType::Tick);
    // Work: 10, 5, 3, 2, 1; recovery: 5, 3, 2, 1. Four cycles.
    QCOMPARE(ticks.size(), 4 * 9);
}

void TstIntervalTimer::tenThousandPeriodsDoNotDrift()
{
    FakeClock clock;
    clock.set(1000000);
    IntervalTimer timer(&clock, 7, 3);
    timer.start();
    Events started;
    const qint64 step = 333;   // does not divide the period lengths
    while (started.size() < 10000) {
        clock.advance(step);
        started += ofType(timer.update(), IntervalEventType::PeriodStarted);
    }
    // Period k starts at start + sum of earlier periods, exactly.
    for (int i = 0; i < started.size(); ++i) {
        const qint64 index = i + 1;
        const qint64 expected = 1000000 + (index / 2) * 10000 + (index % 2) * 7000;
        QCOMPARE(started.at(i).atMs, expected);
        QCOMPARE(started.at(i).periodIndex, i + 1);
    }
    QCOMPARE(timer.periodCount(), started.size() + 1);
    // The drift after 10 000 periods is far below 0.1 s per 60 min.
    const qint64 lastStart = started.last().atMs - 1000000;
    QCOMPARE(lastStart % 1000, qint64(0));
}

void TstIntervalTimer::irregularUpdatesDoNotDrift()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 45, 20);
    timer.start();
    qint64 step = 17;
    qint64 elapsed = 0;
    Events started;
    while (elapsed < 3600000) {
        step = (step * 29 + 13) % 4000 + 1;
        clock.advance(step);
        elapsed += step;
        started += ofType(timer.update(), IntervalEventType::PeriodStarted);
    }
    QVERIFY(!started.isEmpty());
    for (int i = 0; i < started.size(); ++i) {
        const qint64 index = i + 1;
        const qint64 expected = (index / 2) * 65000 + (index % 2) * 45000;
        QCOMPARE(started.at(i).atMs, expected);
    }
}

void TstIntervalTimer::oneLateUpdateCatchesUp()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 10, 5);
    timer.start();
    clock.advance(10 * 3600 * 1000 + 7000);   // a whole night in one step
    const Events events = timer.update();
    const qint64 cycles = 10 * 3600 * 1000 / 15000;   // 2400 cycles
    QCOMPARE(count(events, IntervalEventType::PeriodStarted), int(cycles * 2));
    QCOMPARE(timer.completedWorkPeriods(), int(cycles));
    QVERIFY(timer.periodKind() == PeriodKind::Work);
    QCOMPARE(timer.periodRemainingMs(), qint64(3000));
}

void TstIntervalTimer::remainingAndElapsedTimes()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 20, 10);
    timer.start();
    QCOMPARE(timer.periodElapsedMs(), qint64(0));
    QCOMPARE(timer.periodDurationMs(), qint64(20000));
    clock.advance(7500);
    timer.update();
    QCOMPARE(timer.periodRemainingMs(), qint64(12500));
    QCOMPARE(timer.periodElapsedMs(), qint64(7500));
    clock.advance(12500);
    timer.update();
    QVERIFY(timer.periodKind() == PeriodKind::Recovery);
    QCOMPARE(timer.periodDurationMs(), qint64(10000));
    QCOMPARE(timer.periodRemainingMs(), qint64(10000));
    QCOMPARE(timer.periodElapsedMs(), qint64(0));
    clock.advance(4000);
    QCOMPARE(timer.periodRemainingMs(), qint64(6000));
    QCOMPARE(timer.periodElapsedMs(), qint64(4000));
}

void TstIntervalTimer::completedWorkPeriodsAreCounted()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 10, 5);
    timer.start();
    QCOMPARE(timer.completedWorkPeriods(), 0);
    clock.advance(9999);
    timer.update();
    QCOMPARE(timer.completedWorkPeriods(), 0);
    clock.advance(1);
    timer.update();
    QCOMPARE(timer.completedWorkPeriods(), 1);
    clock.advance(5000);
    timer.update();
    QCOMPARE(timer.completedWorkPeriods(), 1);   // recovery does not count
    clock.advance(10000);
    timer.update();
    QCOMPARE(timer.completedWorkPeriods(), 2);
    // A work period that is stopped halfway is not completed.
    clock.advance(5000);
    timer.update();
    clock.advance(3000);
    timer.stop();
    QCOMPARE(timer.completedWorkPeriods(), 2);
}

void TstIntervalTimer::pauseDuringWork()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 30, 10);
    timer.start();
    clock.advance(12000);
    timer.pause();
    QVERIFY(timer.state() == IntervalState::Paused);
    QCOMPARE(timer.periodRemainingMs(), qint64(18000));
    clock.advance(1000000);
    QVERIFY(timer.update().isEmpty());
    QCOMPARE(timer.periodRemainingMs(), qint64(18000));
    QCOMPARE(timer.periodElapsedMs(), qint64(12000));
    QVERIFY(timer.periodKind() == PeriodKind::Work);
    timer.resume();
    QVERIFY(timer.state() == IntervalState::Running);
    QCOMPARE(timer.periodRemainingMs(), qint64(18000));
    clock.advance(17999);
    QCOMPARE(count(timer.update(), IntervalEventType::PeriodStarted), 0);
    clock.advance(1);
    const Events events = timer.update();
    QCOMPARE(count(events, IntervalEventType::PeriodStarted), 1);
    QVERIFY(timer.periodKind() == PeriodKind::Recovery);
    QCOMPARE(events.last().atMs, clock.nowMs());
}

void TstIntervalTimer::pauseDuringRecovery()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 30, 10);
    timer.start();
    clock.advance(34000);
    timer.update();
    QVERIFY(timer.periodKind() == PeriodKind::Recovery);
    timer.pause();
    QCOMPARE(timer.periodRemainingMs(), qint64(6000));
    clock.advance(60000);
    timer.resume();
    QVERIFY(timer.periodKind() == PeriodKind::Recovery);
    clock.advance(5999);
    QCOMPARE(count(timer.update(), IntervalEventType::PeriodStarted), 0);
    clock.advance(1);
    const Events events = timer.update();
    QCOMPARE(count(events, IntervalEventType::PeriodStarted), 1);
    QVERIFY(timer.periodKind() == PeriodKind::Work);
    QCOMPARE(timer.completedWorkPeriods(), 1);
    QCOMPARE(timer.periodCount(), 3);
}

void TstIntervalTimer::pauseDoesNotRepeatTicks()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 30, 10);
    Events events = timer.start();
    clock.advance(26000);   // 4 s left: ticks 10, 5 fired
    events += timer.update();
    QCOMPARE(count(events, IntervalEventType::Tick), 2);
    timer.pause();
    clock.advance(5000);
    Events after = timer.resume();
    clock.advance(4000);
    after += timer.update();
    // Remaining ticks only: 3, 2, 1.
    const Events ticks = ofType(after, IntervalEventType::Tick);
    QCOMPARE(ticks.size(), 3);
    QCOMPARE(ticks.at(0).remainingSec, 3);
    QCOMPARE(ticks.at(0).atMs, qint64(26000 + 5000 + 1000));
    QCOMPARE(ticks.at(2).remainingSec, 1);
}

void TstIntervalTimer::pauseBetweenTicksKeepsTheRest()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 30, 10);
    timer.start();
    clock.advance(26500);   // 3.5 s left: the ticks for 10 and 5 were reported by update below
    timer.update();
    timer.pause();
    clock.advance(1000);
    timer.resume();
    clock.advance(3500);
    const Events ticks = ofType(timer.update(), IntervalEventType::Tick);
    QCOMPARE(ticks.size(), 3);
    QCOMPARE(ticks.at(0).remainingSec, 3);
}

void TstIntervalTimer::pauseAtTheBoundary()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 10, 5);
    timer.start();
    clock.advance(10000);
    // The period ended exactly now: pausing reports its end and pauses the next period at its start.
    const Events events = timer.pause();
    QCOMPARE(count(events, IntervalEventType::PeriodStarted), 1);
    QVERIFY(timer.state() == IntervalState::Paused);
    QVERIFY(timer.periodKind() == PeriodKind::Recovery);
    QCOMPARE(timer.periodRemainingMs(), qint64(5000));
    QCOMPARE(timer.completedWorkPeriods(), 1);
}

void TstIntervalTimer::pauseWhenNotRunningDoesNothing()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 10, 5);
    QVERIFY(timer.pause().isEmpty());
    QVERIFY(timer.resume().isEmpty());
    QVERIFY(timer.state() == IntervalState::Ready);
    timer.start();
    timer.pause();
    QVERIFY(timer.pause().isEmpty());
    QVERIFY(timer.state() == IntervalState::Paused);
    timer.resume();
    QVERIFY(timer.resume().isEmpty());
    QVERIFY(timer.state() == IntervalState::Running);
}

void TstIntervalTimer::stopFromRunningAndPaused()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 10, 5);
    timer.start();
    clock.advance(3000);
    Events events = timer.stop();
    QVERIFY(timer.state() == IntervalState::Stopped);
    QCOMPARE(events.size(), 1);
    QVERIFY(events.first().type == IntervalEventType::Stopped);
    QCOMPARE(events.first().atMs, qint64(3000));
    QCOMPARE(timer.periodRemainingMs(), qint64(0));

    FakeClock clock2;
    IntervalTimer paused(&clock2, 10, 5);
    paused.start();
    clock2.advance(3000);
    paused.pause();
    events = paused.stop();
    QVERIFY(paused.state() == IntervalState::Stopped);
    QCOMPARE(count(events, IntervalEventType::Stopped), 1);
}

void TstIntervalTimer::stopReportsEarlierEventsFirst()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 10, 5);
    timer.start();
    clock.advance(12000);
    // No update since the work ended: stop reports the end of the work, then the stop.
    const Events events = timer.stop();
    QVERIFY(events.size() >= 2);
    QVERIFY(events.last().type == IntervalEventType::Stopped);
    QCOMPARE(count(events, IntervalEventType::PeriodStarted), 1);
    QCOMPARE(timer.completedWorkPeriods(), 1);
}

void TstIntervalTimer::stoppedTimerStaysStopped()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 10, 5);
    timer.start();
    timer.stop();
    clock.advance(100000);
    QVERIFY(timer.update().isEmpty());
    QVERIFY(timer.start().isEmpty());
    QVERIFY(timer.resume().isEmpty());
    QVERIFY(timer.pause().isEmpty());
    QVERIFY(timer.stop().isEmpty());
    QVERIFY(timer.state() == IntervalState::Stopped);
}

void TstIntervalTimer::startTwiceDoesNothing()
{
    FakeClock clock;
    IntervalTimer timer(&clock, 10, 5);
    QVERIFY(!timer.start().isEmpty());
    clock.advance(4000);
    QVERIFY(timer.start().isEmpty());
    QCOMPARE(timer.periodRemainingMs(), qint64(6000));
}

void TstIntervalTimer::limitCases()
{
    FakeClock clock;
    IntervalTimer longest(&clock, 3600, 600);
    QVERIFY(longest.valid());
    Events events = longest.start();
    clock.advance(3600000);
    events += longest.update();
    QVERIFY(longest.periodKind() == PeriodKind::Recovery);
    QCOMPARE(longest.periodRemainingMs(), qint64(600000));
    clock.advance(600000);
    longest.update();
    QVERIFY(longest.periodKind() == PeriodKind::Work);

    IntervalTimer shortest(&clock, 1, 0);
    QVERIFY(shortest.valid());
    shortest.start();
    clock.advance(10000);
    shortest.update();
    QCOMPARE(shortest.completedWorkPeriods(), 10);
}

QTEST_APPLESS_MAIN(TstIntervalTimer)
#include "tst_intervaltimer.moc"

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "countdown.h"
#include "fakeclock.h"
#include "stopwatch.h"

class TstTimers : public QObject
{
    Q_OBJECT

private slots:
    void countdownStartsReady();
    void countdownDurationLimits();
    void countdownDurationOnlyWhenReady();
    void countdownRunsExactly();
    void countdownExactOverNinetyNineMinutes();
    void countdownFinishesExactlyOnce();
    void countdownFinishedEventIsNotLostByPause();
    void countdownPauseFreezesAndResumeRestores();
    void countdownManyPausesLoseNoTime();
    void countdownResetInEveryState();
    void countdownRestartAfterReset();
    void countdownInvalidTransitions();
    void countdownWithoutDurationDoesNotStart();
    void countdownFinishedStateWithoutUpdate();

    void stopwatchStartsReady();
    void stopwatchCountsUp();
    void stopwatchExactOverNinetyNineMinutes();
    void stopwatchPauseFreezes();
    void stopwatchResumeContinues();
    void stopwatchResetInEveryState();
    void stopwatchStopReturnsElapsed();
    void stopwatchStopWhenPaused();
    void stopwatchStopWhenReady();
    void stopwatchInvalidTransitions();
    void stopwatchRunsPastTheLongestCountdown();
    void countdownTicksComeOnceAtTheListedSeconds();
    void countdownTicksSkipSecondsNotBelowTheDuration();
    void countdownTicksSurviveAPause();
};

void TstTimers::countdownStartsReady()
{
    FakeClock clock;
    clock.set(123456);
    Countdown c(&clock);
    QVERIFY(c.state() == CountdownState::Ready);
    QCOMPARE(c.remainingMs(), qint64(0));
    QVERIFY(c.setDuration(90));
    QCOMPARE(c.durationSec(), 90);
    QVERIFY(c.state() == CountdownState::Ready);
    QCOMPARE(c.remainingMs(), qint64(90000));
    QCOMPARE(c.elapsedMs(), qint64(0));
    clock.advance(10000);
    QCOMPARE(c.remainingMs(), qint64(90000));
    QVERIFY(!c.update());
}

void TstTimers::countdownDurationLimits()
{
    FakeClock clock;
    Countdown c(&clock);
    QVERIFY(!c.setDuration(0));
    QVERIFY(!c.setDuration(-5));
    QVERIFY(!c.setDuration(360000));
    QCOMPARE(c.durationSec(), 0);
    QVERIFY(c.setDuration(1));
    QCOMPARE(c.durationSec(), 1);
    QVERIFY(c.setDuration(359999));
    QCOMPARE(c.durationSec(), 359999);
    QVERIFY(!c.setDuration(360000));
    QCOMPARE(c.durationSec(), 359999);
}

void TstTimers::countdownDurationOnlyWhenReady()
{
    FakeClock clock;
    Countdown c(&clock);
    QVERIFY(c.setDuration(10));
    QVERIFY(c.start());
    QVERIFY(!c.setDuration(20));
    QCOMPARE(c.durationSec(), 10);
    QVERIFY(c.pause());
    QVERIFY(!c.setDuration(20));
    c.reset();
    QVERIFY(c.setDuration(20));
    QCOMPARE(c.remainingMs(), qint64(20000));
}

void TstTimers::countdownRunsExactly()
{
    FakeClock clock;
    clock.set(5000);
    Countdown c(&clock);
    QVERIFY(c.setDuration(60));
    QVERIFY(c.start());
    QVERIFY(c.state() == CountdownState::Running);
    QCOMPARE(c.remainingMs(), qint64(60000));
    clock.advance(1);
    QCOMPARE(c.remainingMs(), qint64(59999));
    QCOMPARE(c.elapsedMs(), qint64(1));
    clock.advance(24999);
    QCOMPARE(c.remainingMs(), qint64(35000));
    QCOMPARE(c.elapsedMs(), qint64(25000));
    QVERIFY(!c.update());
    clock.advance(34999);
    QCOMPARE(c.remainingMs(), qint64(1));
    QVERIFY(!c.update());
    QVERIFY(c.state() == CountdownState::Running);
    clock.advance(1);
    QCOMPARE(c.remainingMs(), qint64(0));
    QVERIFY(c.update());
    QVERIFY(c.state() == CountdownState::Finished);
}

void TstTimers::countdownExactOverNinetyNineMinutes()
{
    FakeClock clock;
    Countdown c(&clock);
    QVERIFY(c.setDuration(5999));
    QVERIFY(c.start());
    qint64 expected = 5999000;
    int finished = 0;
    // Irregular steps: the result depends on the clock only.
    for (qint64 step = 7; expected > 0; step = (step * 31 + 17) % 2000 + 1) {
        const qint64 use = qMin(step, expected);
        clock.advance(use);
        expected -= use;
        QCOMPARE(c.remainingMs(), expected);
        QCOMPARE(c.elapsedMs(), 5999000 - expected);
        if (c.update())
            ++finished;
    }
    QCOMPARE(finished, 1);
    QVERIFY(c.state() == CountdownState::Finished);
    QCOMPARE(c.remainingMs(), qint64(0));
    QCOMPARE(c.elapsedMs(), qint64(5999000));
}

void TstTimers::countdownFinishesExactlyOnce()
{
    FakeClock clock;
    Countdown c(&clock);
    QVERIFY(c.setDuration(3));
    QVERIFY(c.start());
    clock.advance(3000);
    QVERIFY(c.update());
    for (int i = 0; i < 5; ++i) {
        clock.advance(1000);
        QVERIFY(!c.update());
    }
    QVERIFY(c.state() == CountdownState::Finished);
    QCOMPARE(c.remainingMs(), qint64(0));
    // Much later: still one event only.
    clock.advance(1000000);
    QVERIFY(!c.update());
    QCOMPARE(c.elapsedMs(), qint64(3000));
    // After a reset and a new run there is a new event.
    c.reset();
    QVERIFY(c.setDuration(2));
    QVERIFY(c.start());
    clock.advance(2000);
    QVERIFY(c.update());
    QVERIFY(!c.update());
}

void TstTimers::countdownFinishedEventIsNotLostByPause()
{
    FakeClock clock;
    Countdown c(&clock);
    QVERIFY(c.setDuration(5));
    QVERIFY(c.start());
    clock.advance(6000);
    // Pausing a countdown that has run out does not pause it, and the event is still reported.
    QVERIFY(!c.pause());
    QVERIFY(c.state() == CountdownState::Finished);
    QVERIFY(c.update());
    QVERIFY(!c.update());
    QVERIFY(!c.resume());
}

void TstTimers::countdownPauseFreezesAndResumeRestores()
{
    FakeClock clock;
    Countdown c(&clock);
    QVERIFY(c.setDuration(100));
    QVERIFY(c.start());
    clock.advance(30000);
    QVERIFY(c.pause());
    QVERIFY(c.state() == CountdownState::Paused);
    QCOMPARE(c.remainingMs(), qint64(70000));
    clock.advance(500000);
    QCOMPARE(c.remainingMs(), qint64(70000));
    QCOMPARE(c.elapsedMs(), qint64(30000));
    QVERIFY(!c.update());
    QVERIFY(c.state() == CountdownState::Paused);
    QVERIFY(c.resume());
    QVERIFY(c.state() == CountdownState::Running);
    QCOMPARE(c.remainingMs(), qint64(70000));
    clock.advance(69999);
    QCOMPARE(c.remainingMs(), qint64(1));
    QVERIFY(!c.update());
    clock.advance(1);
    QVERIFY(c.update());
}

void TstTimers::countdownManyPausesLoseNoTime()
{
    FakeClock clock;
    Countdown c(&clock);
    QVERIFY(c.setDuration(600));
    QVERIFY(c.start());
    for (int i = 0; i < 600; ++i) {
        clock.advance(300);
        QVERIFY(c.pause());
        clock.advance(777);
        QVERIFY(c.resume());
        clock.advance(700);
    }
    QCOMPARE(c.remainingMs(), qint64(0));
    QVERIFY(c.update());
}

void TstTimers::countdownResetInEveryState()
{
    FakeClock clock;
    Countdown c(&clock);
    QVERIFY(c.setDuration(40));

    c.reset();   // from Ready
    QVERIFY(c.state() == CountdownState::Ready);
    QCOMPARE(c.remainingMs(), qint64(40000));

    QVERIFY(c.start());
    clock.advance(10000);
    c.reset();   // from Running
    QVERIFY(c.state() == CountdownState::Ready);
    QCOMPARE(c.remainingMs(), qint64(40000));
    QCOMPARE(c.elapsedMs(), qint64(0));
    clock.advance(50000);
    QVERIFY(!c.update());

    QVERIFY(c.start());
    clock.advance(10000);
    QVERIFY(c.pause());
    c.reset();   // from Paused
    QVERIFY(c.state() == CountdownState::Ready);
    QCOMPARE(c.remainingMs(), qint64(40000));
    QVERIFY(!c.resume());

    QVERIFY(c.start());
    clock.advance(40000);
    QVERIFY(c.update());
    c.reset();   // from Finished
    QVERIFY(c.state() == CountdownState::Ready);
    QCOMPARE(c.remainingMs(), qint64(40000));
    QVERIFY(!c.update());

    // Reset while the end is reached but not yet reported drops the event.
    QVERIFY(c.start());
    clock.advance(41000);
    c.reset();
    QVERIFY(!c.update());
    QVERIFY(c.state() == CountdownState::Ready);
}

void TstTimers::countdownRestartAfterReset()
{
    FakeClock clock;
    Countdown c(&clock);
    QVERIFY(c.setDuration(10));
    QVERIFY(c.start());
    clock.advance(4000);
    c.reset();
    clock.advance(1000);
    QVERIFY(c.start());
    QCOMPARE(c.remainingMs(), qint64(10000));
    clock.advance(10000);
    QVERIFY(c.update());
}

void TstTimers::countdownInvalidTransitions()
{
    FakeClock clock;
    Countdown c(&clock);
    QVERIFY(c.setDuration(10));
    QVERIFY(!c.pause());
    QVERIFY(!c.resume());
    QVERIFY(c.start());
    QVERIFY(!c.start());
    QVERIFY(!c.resume());
    clock.advance(1000);
    QVERIFY(c.pause());
    QVERIFY(!c.pause());
    QVERIFY(!c.start());
    QCOMPARE(c.remainingMs(), qint64(9000));
}

void TstTimers::countdownWithoutDurationDoesNotStart()
{
    FakeClock clock;
    Countdown c(&clock);
    QVERIFY(!c.start());
    QVERIFY(c.state() == CountdownState::Ready);
}

void TstTimers::countdownFinishedStateWithoutUpdate()
{
    FakeClock clock;
    Countdown c(&clock);
    QVERIFY(c.setDuration(2));
    QVERIFY(c.start());
    clock.advance(5000);
    // The state and the times are right before update() is called.
    QVERIFY(c.state() == CountdownState::Finished);
    QCOMPARE(c.remainingMs(), qint64(0));
    QCOMPARE(c.elapsedMs(), qint64(2000));
    QVERIFY(c.update());
}

void TstTimers::stopwatchStartsReady()
{
    FakeClock clock;
    clock.set(99);
    Stopwatch s(&clock);
    QVERIFY(s.state() == StopwatchState::Ready);
    QCOMPARE(s.elapsedMs(), qint64(0));
    clock.advance(5000);
    QCOMPARE(s.elapsedMs(), qint64(0));
}

void TstTimers::stopwatchCountsUp()
{
    FakeClock clock;
    clock.set(1000);
    Stopwatch s(&clock);
    QVERIFY(s.start());
    QVERIFY(s.state() == StopwatchState::Running);
    QCOMPARE(s.elapsedMs(), qint64(0));
    clock.advance(1);
    QCOMPARE(s.elapsedMs(), qint64(1));
    clock.advance(1233);
    QCOMPARE(s.elapsedMs(), qint64(1234));
}

void TstTimers::stopwatchExactOverNinetyNineMinutes()
{
    FakeClock clock;
    Stopwatch s(&clock);
    QVERIFY(s.start());
    qint64 total = 0;
    for (qint64 step = 13; total < 5999000; step = (step * 37 + 11) % 3000 + 1) {
        clock.advance(step);
        total += step;
        QCOMPARE(s.elapsedMs(), total);
    }
    QVERIFY(s.state() == StopwatchState::Running);
}

void TstTimers::stopwatchPauseFreezes()
{
    FakeClock clock;
    Stopwatch s(&clock);
    QVERIFY(s.start());
    clock.advance(2500);
    QVERIFY(s.pause());
    QVERIFY(s.state() == StopwatchState::Paused);
    QCOMPARE(s.elapsedMs(), qint64(2500));
    clock.advance(3600000);
    QCOMPARE(s.elapsedMs(), qint64(2500));
}

void TstTimers::stopwatchResumeContinues()
{
    FakeClock clock;
    Stopwatch s(&clock);
    QVERIFY(s.start());
    clock.advance(2500);
    QVERIFY(s.pause());
    clock.advance(10000);
    QVERIFY(s.resume());
    QVERIFY(s.state() == StopwatchState::Running);
    QCOMPARE(s.elapsedMs(), qint64(2500));
    clock.advance(1500);
    QCOMPARE(s.elapsedMs(), qint64(4000));
    for (int i = 0; i < 1000; ++i) {
        QVERIFY(s.pause());
        clock.advance(333);
        QVERIFY(s.resume());
        clock.advance(2);
    }
    QCOMPARE(s.elapsedMs(), qint64(4000 + 2000));
}

void TstTimers::stopwatchResetInEveryState()
{
    FakeClock clock;
    Stopwatch s(&clock);
    s.reset();   // from Ready
    QVERIFY(s.state() == StopwatchState::Ready);
    QCOMPARE(s.elapsedMs(), qint64(0));

    QVERIFY(s.start());
    clock.advance(7000);
    s.reset();   // from Running
    QVERIFY(s.state() == StopwatchState::Ready);
    QCOMPARE(s.elapsedMs(), qint64(0));
    clock.advance(5000);
    QCOMPARE(s.elapsedMs(), qint64(0));

    QVERIFY(s.start());
    clock.advance(7000);
    QVERIFY(s.pause());
    s.reset();   // from Paused
    QVERIFY(s.state() == StopwatchState::Ready);
    QCOMPARE(s.elapsedMs(), qint64(0));
    QVERIFY(!s.resume());

    // A new run starts from zero.
    QVERIFY(s.start());
    clock.advance(10);
    QCOMPARE(s.elapsedMs(), qint64(10));
}

void TstTimers::stopwatchStopReturnsElapsed()
{
    FakeClock clock;
    clock.set(777);
    Stopwatch s(&clock);
    QVERIFY(s.start());
    clock.advance(12345);
    QCOMPARE(s.stop(), qint64(12345));
    QVERIFY(s.state() == StopwatchState::Ready);
    QCOMPARE(s.elapsedMs(), qint64(0));
    // The next run is independent.
    QVERIFY(s.start());
    clock.advance(10);
    QCOMPARE(s.stop(), qint64(10));
}

void TstTimers::stopwatchStopWhenPaused()
{
    FakeClock clock;
    Stopwatch s(&clock);
    QVERIFY(s.start());
    clock.advance(4000);
    QVERIFY(s.pause());
    clock.advance(99999);
    QCOMPARE(s.stop(), qint64(4000));
    QVERIFY(s.state() == StopwatchState::Ready);
}

void TstTimers::stopwatchStopWhenReady()
{
    FakeClock clock;
    Stopwatch s(&clock);
    QCOMPARE(s.stop(), qint64(-1));
    QVERIFY(s.state() == StopwatchState::Ready);
}

void TstTimers::stopwatchInvalidTransitions()
{
    FakeClock clock;
    Stopwatch s(&clock);
    QVERIFY(!s.pause());
    QVERIFY(!s.resume());
    QVERIFY(s.start());
    QVERIFY(!s.start());
    QVERIFY(!s.resume());
    clock.advance(100);
    QVERIFY(s.pause());
    QVERIFY(!s.pause());
    QVERIFY(!s.start());
    QCOMPARE(s.elapsedMs(), qint64(100));
}

void TstTimers::stopwatchRunsPastTheLongestCountdown()
{
    FakeClock clock;
    Stopwatch s(&clock);
    QVERIFY(s.start());
    clock.advance(qint64(24) * 3600 * 1000);
    QCOMPARE(s.elapsedMs(), qint64(86400000));
    QCOMPARE(s.stop(), qint64(86400000));
}

void TstTimers::countdownTicksComeOnceAtTheListedSeconds()
{
    FakeClock clock;
    Countdown c(&clock);
    c.setTickSeconds(QList<int>() << 1 << 5 << 3 << 5 << 0);
    QVERIFY(c.setDuration(20));
    QVERIFY(c.start());
    QVERIFY(c.takeTicks().isEmpty());
    clock.advance(14999);
    QVERIFY(c.takeTicks().isEmpty());
    clock.advance(1);
    QCOMPARE(c.takeTicks(), QList<int>() << 5);
    QVERIFY(c.takeTicks().isEmpty());
    // A late poll reports the ticks that were due, largest first, once.
    clock.advance(3000);
    QCOMPARE(c.takeTicks(), QList<int>() << 3);
}

void TstTimers::countdownTicksSkipSecondsNotBelowTheDuration()
{
    FakeClock clock;
    Countdown c(&clock);
    QVERIFY(c.setDuration(5));
    QVERIFY(c.start());
    clock.advance(1);
    // The default list is 10, 5, 3, 2, 1: 10 and 5 are not below a duration of 5 s.
    QVERIFY(c.takeTicks().isEmpty());
    clock.advance(2000);
    QCOMPARE(c.takeTicks(), QList<int>() << 3);
    // The ticks come before the end event.
    clock.advance(10000);
    QCOMPARE(c.takeTicks(), QList<int>() << 2 << 1);
    QVERIFY(c.update());
}

void TstTimers::countdownTicksSurviveAPause()
{
    FakeClock clock;
    Countdown c(&clock);
    QVERIFY(c.setDuration(12));
    QVERIFY(c.start());
    clock.advance(2000);
    QCOMPARE(c.takeTicks(), QList<int>() << 10);
    QVERIFY(c.pause());
    clock.advance(60000);
    QVERIFY(c.takeTicks().isEmpty());
    QVERIFY(c.resume());
    QVERIFY(c.takeTicks().isEmpty());
    clock.advance(5000);
    QCOMPARE(c.takeTicks(), QList<int>() << 5);
    // A reset allows every tick again.
    c.reset();
    QVERIFY(c.setDuration(12));
    QVERIFY(c.start());
    clock.advance(2000);
    QCOMPARE(c.takeTicks(), QList<int>() << 10);
}

QTEST_APPLESS_MAIN(TstTimers)
#include "tst_timers.moc"

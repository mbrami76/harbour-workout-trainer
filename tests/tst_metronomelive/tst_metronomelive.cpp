// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "metronomehelpers.h"

// Live changes of a run (R-MET-6). The sets are 4 s long (4 repetitions of 1 s) and the rests 10 s.
class TstMetronomeLive : public QObject
{
    Q_OBJECT

private slots:
    void addSetInTheLeadIn();
    void addSetWhileRunning();
    void addSetWhileRunningTheLastSet();
    void addSetWhileResting();
    void addSetWhilePaused();
    void addSetWhenReady();
    void addSetStopsAtNinetyNine();
    void addSetUpdatesCountsAndRemainingTime();
    void removeSetInTheLeadIn();
    void removeSetWhileRunning();
    void removeSetWhileResting();
    void removeTheLastSetWhileRestingEndsTheRun();
    void removeTheLastSetWhilePausedInARestEndsTheRun();
    void removeSetWhilePaused();
    void removeFailsWhenNoSetIsLeft();
    void removeNeverRemovesTheRunningSet();
    void removeTheNextSetRemovesTheRestBeforeIt();
    void removeAtTheBoundaries();
    void changesAreRefusedWhenFinishedOrStopped();
    void changesDoNotAffectTheSetThatRuns();
    void eventsDueDuringAChangeAreReportedOnce();
    void addedSetsRunWithTheirRests();
    void addAndRemoveInARow();

private:
    Metronome *make(FakeClock *clock, int lead, int sets, int rest)
    {
        MetronomeConfig c = makeConfig(Tempo::timePerRep(1000, 4), lead, sets, rest);
        return new Metronome(clock, c);
    }
};

void TstMetronomeLive::addSetInTheLeadIn()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 5, 1, 10));
    m->start();
    clock.advance(1000);
    QCOMPARE(m->setCount(), 1);
    QVERIFY(m->addSet());
    QCOMPARE(m->setCount(), 2);
    QCOMPARE(m->setNumber(), 1);
    QCOMPARE(m->setsLeft(), 1);
    // 4 s of the lead-in left, the set, the rest and the second set.
    QCOMPARE(m->remainingMs(), qint64(4000 + 4000 + 10000 + 4000));
    MetEvents events = m->update();
    events += drive(*m, clock, 4000 + 4000 + 10000 + 4000, 100);
    QCOMPARE(countOf(events, MetronomeEventType::SetStarted), 2);
    QCOMPARE(countOf(events, MetronomeEventType::RestStarted), 1);
    QCOMPARE(ofType(events, MetronomeEventType::Finished).first().atMs, qint64(5000 + 4000 + 10000 + 4000));
}

void TstMetronomeLive::addSetWhileRunning()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 2, 10));
    m->start();
    clock.advance(1500);
    QCOMPARE(m->remainingMs(), qint64(2500 + 10000 + 4000));
    QVERIFY(m->addSet());
    QCOMPARE(m->setCount(), 3);
    QCOMPARE(m->setsLeft(), 2);
    QCOMPARE(m->remainingMs(), qint64(2500 + 2 * (10000 + 4000)));
    MetEvents events = m->update();
    events += drive(*m, clock, 2500 + 2 * 14000, 100);
    QCOMPARE(countOf(events, MetronomeEventType::SetStarted), 2);   // the first started before
    QVERIFY(m->state() == MetronomeState::Finished);
    QCOMPARE(m->setNumber(), 3);
}

void TstMetronomeLive::addSetWhileRunningTheLastSet()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 1, 10));
    m->start();
    clock.advance(1000);
    QCOMPARE(m->setsLeft(), 0);
    QVERIFY(m->addSet());
    // The running set is no longer the last: a rest follows it.
    MetEvents events = drive(*m, clock, 3000, 100);
    QCOMPARE(countOf(events, MetronomeEventType::RestStarted), 1);
    QCOMPARE(countOf(events, MetronomeEventType::Finished), 0);
    QVERIFY(m->state() == MetronomeState::Resting);
    events = drive(*m, clock, 10000 + 4000, 100);
    QCOMPARE(countOf(events, MetronomeEventType::Finished), 1);
}

void TstMetronomeLive::addSetWhileResting()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 2, 10));
    m->start();
    clock.advance(4000 + 3000);
    m->update();
    QVERIFY(m->state() == MetronomeState::Resting);
    QCOMPARE(m->restRemainingMs(), qint64(7000));
    QVERIFY(m->addSet());
    QCOMPARE(m->setCount(), 3);
    QCOMPARE(m->restRemainingMs(), qint64(7000));
    QCOMPARE(m->remainingMs(), qint64(7000 + 4000 + 10000 + 4000));
    const MetEvents events = drive(*m, clock, 7000 + 4000 + 10000 + 4000, 100);
    QCOMPARE(countOf(events, MetronomeEventType::SetStarted), 2);
    QVERIFY(m->state() == MetronomeState::Finished);
}

void TstMetronomeLive::addSetWhilePaused()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 1, 10));
    m->start();
    clock.advance(1500);
    m->pause();
    QVERIFY(m->addSet());
    QVERIFY(m->state() == MetronomeState::Paused);
    QCOMPARE(m->setCount(), 2);
    // The paused repetition counts from its start.
    QCOMPARE(m->remainingMs(), qint64(3000 + 10000 + 4000));
    clock.advance(9999);
    m->resume();
    const MetEvents events = drive(*m, clock, 3000 + 14000, 100);
    QCOMPARE(countOf(events, MetronomeEventType::SetStarted), 1);
    QVERIFY(m->state() == MetronomeState::Finished);
}

void TstMetronomeLive::addSetWhenReady()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 3, 2, 10));
    QVERIFY(m->addSet());
    QCOMPARE(m->setCount(), 3);
    QVERIFY(m->state() == MetronomeState::Ready);
    QCOMPARE(m->remainingMs(), qint64(3000 + 3 * 4000 + 2 * 10000));
    QVERIFY(m->removeSet());
    QVERIFY(m->removeSet());
    QCOMPARE(m->setCount(), 1);
    QVERIFY(!m->removeSet());
}

void TstMetronomeLive::addSetStopsAtNinetyNine()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 98, 0));
    m->start();
    QVERIFY(m->addSet());
    QCOMPARE(m->setCount(), 99);
    QVERIFY(!m->addSet());
    QCOMPARE(m->setCount(), 99);
    QVERIFY(m->removeSet());
    QVERIFY(m->addSet());
    QCOMPARE(m->setCount(), 99);
}

void TstMetronomeLive::addSetUpdatesCountsAndRemainingTime()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 2, 5));
    m->start();
    qint64 before = m->remainingMs();
    QVERIFY(m->addSet());
    QCOMPARE(m->remainingMs(), before + 5000 + 4000);
    QCOMPARE(m->setCount(), 3);
    QCOMPARE(m->setsLeft(), 2);
    QCOMPARE(m->setNumber(), 1);
}

void TstMetronomeLive::removeSetInTheLeadIn()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 5, 3, 10));
    m->start();
    clock.advance(2000);
    QVERIFY(m->removeSet());
    QCOMPARE(m->setCount(), 2);
    QCOMPARE(m->setNumber(), 1);
    QCOMPARE(m->remainingMs(), qint64(3000 + 4000 + 10000 + 4000));
    QVERIFY(m->removeSet());
    QCOMPARE(m->setCount(), 1);
    QVERIFY(!m->removeSet());          // the first set is the one that is about to run
    QCOMPARE(m->setCount(), 1);
    QCOMPARE(m->remainingMs(), qint64(3000 + 4000));
}

void TstMetronomeLive::removeSetWhileRunning()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 3, 10));
    m->start();
    clock.advance(4000 + 10000 + 1000);   // in set 2
    m->update();
    QCOMPARE(m->setNumber(), 2);
    QCOMPARE(m->setsLeft(), 1);
    QVERIFY(m->removeSet());
    QCOMPARE(m->setCount(), 2);
    QCOMPARE(m->setsLeft(), 0);
    QCOMPARE(m->remainingMs(), qint64(3000));
    const MetEvents events = drive(*m, clock, 3000, 50);
    QCOMPARE(countOf(events, MetronomeEventType::Finished), 1);
    QCOMPARE(countOf(events, MetronomeEventType::SetStarted), 0);
}

void TstMetronomeLive::removeSetWhileResting()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 4, 10));
    m->start();
    clock.advance(4000 + 3000);
    m->update();
    QVERIFY(m->state() == MetronomeState::Resting);
    QVERIFY(m->removeSet());
    QCOMPARE(m->setCount(), 3);
    // The running rest goes on; two sets with one rest between them follow.
    QCOMPARE(m->restRemainingMs(), qint64(7000));
    QCOMPARE(m->remainingMs(), qint64(7000 + 4000 + 10000 + 4000));
    const MetEvents events = drive(*m, clock, 7000 + 4000 + 10000 + 4000, 100);
    QCOMPARE(countOf(events, MetronomeEventType::SetStarted), 2);
    QCOMPARE(countOf(events, MetronomeEventType::Finished), 1);
}

void TstMetronomeLive::removeTheLastSetWhileRestingEndsTheRun()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 2, 10));
    m->start();
    clock.advance(4000 + 3000);
    m->update();
    QVERIFY(m->state() == MetronomeState::Resting);
    QVERIFY(m->removeSet());
    QCOMPARE(m->setCount(), 1);
    QVERIFY(m->state() == MetronomeState::Finished);
    const MetEvents events = m->update();
    QCOMPARE(countOf(events, MetronomeEventType::Finished), 1);
    QCOMPARE(events.first().atMs, clock.nowMs());
    QCOMPARE(m->remainingMs(), qint64(0));
    QVERIFY(m->update().isEmpty());
}

void TstMetronomeLive::removeTheLastSetWhilePausedInARestEndsTheRun()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 2, 10));
    m->start();
    clock.advance(4000 + 3000);
    m->pause();
    QVERIFY(m->removeSet());
    QVERIFY(m->state() == MetronomeState::Finished);
    QCOMPARE(countOf(m->update(), MetronomeEventType::Finished), 1);
}

void TstMetronomeLive::removeSetWhilePaused()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 3, 10));
    m->start();
    clock.advance(1500);
    m->pause();
    QVERIFY(m->removeSet());
    QVERIFY(m->state() == MetronomeState::Paused);
    QCOMPARE(m->setCount(), 2);
    QCOMPARE(m->remainingMs(), qint64(3000 + 10000 + 4000));
    m->resume();
    const MetEvents events = drive(*m, clock, 3000 + 14000, 100);
    QCOMPARE(countOf(events, MetronomeEventType::Finished), 1);
}

void TstMetronomeLive::removeFailsWhenNoSetIsLeft()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 1, 10));
    m->start();
    QVERIFY(!m->removeSet());
    QCOMPARE(m->setCount(), 1);
    QCOMPARE(m->remainingMs(), qint64(4000));
    clock.advance(1000);
    QVERIFY(!m->removeSet());
    m->pause();
    QVERIFY(!m->removeSet());
    QCOMPARE(m->setCount(), 1);
}

void TstMetronomeLive::removeNeverRemovesTheRunningSet()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 3, 10));
    m->start();
    QVERIFY(m->removeSet());
    QVERIFY(m->removeSet());
    QVERIFY(!m->removeSet());
    QCOMPARE(m->setCount(), 1);
    QCOMPARE(m->setNumber(), 1);
    QVERIFY(m->state() == MetronomeState::Running);
    clock.advance(1000);
    m->update();
    QCOMPARE(m->setNumber(), 1);
}

void TstMetronomeLive::removeTheNextSetRemovesTheRestBeforeIt()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 3, 10));
    m->start();
    QCOMPARE(m->remainingMs(), qint64(3 * 4000 + 2 * 10000));
    QVERIFY(m->removeSet());
    // One set and one rest less.
    QCOMPARE(m->remainingMs(), qint64(2 * 4000 + 1 * 10000));
    MetEvents events = drive(*m, clock, 4000 + 10000 + 4000, 100);
    QCOMPARE(countOf(events, MetronomeEventType::RestStarted), 1);
    QCOMPARE(countOf(events, MetronomeEventType::SetStarted), 1);
    QCOMPARE(ofType(events, MetronomeEventType::Finished).first().atMs, qint64(18000));
}

void TstMetronomeLive::removeAtTheBoundaries()
{
    // Exactly at the end of a set that is not the last one: the run is already resting.
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 3, 10));
    m->start();
    clock.advance(4000);
    QVERIFY(m->removeSet());
    QVERIFY(m->state() == MetronomeState::Resting);
    QCOMPARE(m->setCount(), 2);
    // Exactly at the end of the rest: the next set is running and cannot be removed.
    clock.advance(10000);
    QVERIFY(!m->removeSet());
    QVERIFY(m->state() == MetronomeState::Running);
    QCOMPARE(m->setNumber(), 2);
    // Exactly at the end of the last set: finished, nothing to change.
    clock.advance(4000);
    QVERIFY(!m->addSet());
    QVERIFY(m->state() == MetronomeState::Finished);
}

void TstMetronomeLive::changesAreRefusedWhenFinishedOrStopped()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 2, 0));
    m->start();
    clock.advance(8000);
    m->update();
    QVERIFY(m->state() == MetronomeState::Finished);
    QVERIFY(!m->addSet());
    QVERIFY(!m->removeSet());
    QCOMPARE(m->setCount(), 2);

    FakeClock clock2;
    QScopedPointer<Metronome> s(make(&clock2, 0, 3, 0));
    s->start();
    clock2.advance(1000);
    s->stop();
    QVERIFY(!s->addSet());
    QVERIFY(!s->removeSet());
    QCOMPARE(s->setCount(), 3);
    QVERIFY(s->state() == MetronomeState::Stopped);
}

void TstMetronomeLive::changesDoNotAffectTheSetThatRuns()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 2, 10));
    MetEvents events = m->start();
    clock.advance(1500);
    const double fraction = m->repetitionFraction();
    QVERIFY(m->addSet());
    QVERIFY(m->removeSet());
    QCOMPARE(m->repetitionFraction(), fraction);
    QCOMPARE(m->repetitionNumber(), 2);
    events += m->update();
    clock.advance(2500);
    events += m->update();
    // The running set kept its beats: one per second, four in all.
    QCOMPARE(countOf(events, MetronomeEventType::Beat), 4);
}

void TstMetronomeLive::eventsDueDuringAChangeAreReportedOnce()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 3, 10));
    m->start();
    clock.advance(4000 + 12000);   // set 1 ended, the rest ended, set 2 started and is 2 s in
    // The change finds the run in set 2: set 3 can be removed, set 2 not.
    QVERIFY(m->removeSet());
    QVERIFY(!m->removeSet());
    QCOMPARE(m->setNumber(), 2);
    const MetEvents events = m->update();
    QCOMPARE(countOf(events, MetronomeEventType::SetEnded), 1);
    QCOMPARE(countOf(events, MetronomeEventType::RestStarted), 1);
    QCOMPARE(countOf(events, MetronomeEventType::SetStarted), 1);
    // The beats of both sets, except the first one of set 1 that start() reported.
    QCOMPARE(countOf(events, MetronomeEventType::Beat), 3 + 3);
    // Once only.
    QVERIFY(m->update().isEmpty());
    // The events that fall due during a change are not lost when the next call is another action.
    clock.advance(1000);
    QVERIFY(m->addSet());
    const MetEvents next = m->pause();
    QCOMPARE(countOf(next, MetronomeEventType::Beat), 1);
}

void TstMetronomeLive::addedSetsRunWithTheirRests()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 0, 1, 7));
    MetEvents events = m->start();
    QVERIFY(m->addSet());
    QVERIFY(m->addSet());
    events += drive(*m, clock, 4000 + 2 * (7000 + 4000), 100);
    QCOMPARE(describeAll(ofType(events, MetronomeEventType::SetStarted)),
             QStringList() << "set@0 s1 r0 p0 a0 t0" << "set@11000 s2 r0 p0 a0 t0" << "set@22000 s3 r0 p0 a0 t0");
    QCOMPARE(describeAll(ofType(events, MetronomeEventType::RestStarted)),
             QStringList() << "rest@4000 s1 r0 p0 a0 t0" << "rest@15000 s2 r0 p0 a0 t0");
    QCOMPARE(ofType(events, MetronomeEventType::Finished).first().atMs, qint64(26000));
}

void TstMetronomeLive::addAndRemoveInARow()
{
    FakeClock clock;
    QScopedPointer<Metronome> m(make(&clock, 2, 2, 5));
    m->start();
    for (int i = 0; i < 20; ++i) {
        QVERIFY(m->addSet());
        QVERIFY(m->removeSet());
        QCOMPARE(m->setCount(), 2);
    }
    for (int i = 0; i < 97; ++i)
        QVERIFY(m->addSet());
    QCOMPARE(m->setCount(), 99);
    for (int i = 0; i < 98; ++i)
        QVERIFY(m->removeSet());
    QCOMPARE(m->setCount(), 1);
    QVERIFY(!m->removeSet());
    QCOMPARE(m->remainingMs(), qint64(2000 + 4000));
}

QTEST_APPLESS_MAIN(TstMetronomeLive)
#include "tst_metronomelive.moc"

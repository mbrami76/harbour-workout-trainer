// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "metronomehelpers.h"
#include "testsupport.h"

class TstMetronome : public QObject
{
    Q_OBJECT

private slots:
    void invalidConfigurationsDoNotStart();
    void limitValuesAreAccepted();
    void startWithoutLeadInStartsTheSetAtOnce();
    void leadInTicksAndStart();
    void timePerRepSequence();
    void repsAndTotalEvenlyAndWithRounding();
    void threePhaseSequence();
    void aTryOfARhythmRunsRepetitionsWithoutSetsOrRest();
    void beatsCarryTheirKind();
    void eventsAreTheSameForAnyPollingStep();
    void eventsComeOnceAndInOrder();
    void setsAndRests();
    void restOfZeroStartsTheNextSetAtOnce();
    void lastSetHasNoRest();
    void restTicksUseTheConfiguredList();
    void noDriftOverSixtyMinutes();
    void noDriftWithSetsAndRests();
    void lateUpdateReportsScheduledTimes();
    void pauseInLeadIn();
    void pauseInTheMiddleOfARepetition();
    void pauseInPhasesRestartsAtPhaseOne();
    void pauseAtARepetitionStart();
    void pauseInARest();
    void pauseWhenNotRunningDoesNothing();
    void repeatedPausesAndResumes();
    void stopFromEveryState();
    void stoppedAndFinishedStayDone();
    void startTwiceDoesNothing();
    void queriesInALeadInAndASet();
    void fractionsOfARepetitionAndAPhase();
    void queriesDuringARest();
    void remainingTimeOfTheWholeRun();
    void remainingTimeWhilePaused();
};

void TstMetronome::invalidConfigurationsDoNotStart()
{
    FakeClock clock;
    const Tempo good = Tempo::timePerRep(1000, 3);
    const MetronomeConfig bad[] = {
        makeConfig(Tempo(), 3, 1, 0),
        makeConfig(Tempo::timePerRep(100, 3), 3, 1, 0),
        makeConfig(good, -1, 1, 0),
        makeConfig(good, 11, 1, 0),
        makeConfig(good, 3, 0, 0),
        makeConfig(good, 3, 100, 0),
        makeConfig(good, 3, 1, -1),
        makeConfig(good, 3, 1, 601)
    };
    for (const MetronomeConfig &c : bad) {
        QVERIFY(!Metronome::isValid(c));
        Metronome m(&clock, c);
        QVERIFY(!m.valid());
        QVERIFY(m.start().isEmpty());
        QVERIFY(m.state() == MetronomeState::Ready);
        clock.advance(100000);
        QVERIFY(m.update().isEmpty());
    }
}

void TstMetronome::limitValuesAreAccepted()
{
    FakeClock clock;
    const Tempo good = Tempo::timePerRep(500, 999);
    QVERIFY(Metronome::isValid(makeConfig(good, 0, 1, 0)));
    QVERIFY(Metronome::isValid(makeConfig(good, 10, 99, 600)));
}

void TstMetronome::startWithoutLeadInStartsTheSetAtOnce()
{
    FakeClock clock;
    clock.set(8000);
    Metronome m(&clock, makeConfig(Tempo::timePerRep(1000, 2), 0, 1, 0));
    const MetEvents events = m.start();
    QVERIFY(m.state() == MetronomeState::Running);
    QCOMPARE(describeAll(events), QStringList() << "set@8000 s1 r0 p0 a0 t0" << "rep@8000 s1 r1 p0 a0 t0"
                                                << "beat@8000 s1 r1 p0 a1 t0");
}

void TstMetronome::leadInTicksAndStart()
{
    FakeClock clock;
    clock.set(500);
    Metronome m(&clock, makeConfig(Tempo::timePerRep(1000, 1), 3, 1, 0));
    MetEvents events = m.start();
    QVERIFY(m.state() == MetronomeState::LeadIn);
    QVERIFY(events.isEmpty());
    events += drive(m, clock, 4000, 10);
    // Ticks for 2 s and 1 s (3 s is not below the length of the lead-in), then the set.
    QCOMPARE(describeAll(events), QStringList() << "leadtick@1500 s1 r0 p0 a0 t2" << "leadtick@2500 s1 r0 p0 a0 t1"
                                                << "set@3500 s1 r0 p0 a0 t0" << "rep@3500 s1 r1 p0 a0 t0"
                                                << "beat@3500 s1 r1 p0 a1 t0" << "setend@4500 s1 r0 p0 a0 t0"
                                                << "finished@4500 s1 r0 p0 a0 t0");
}

void TstMetronome::timePerRepSequence()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(Tempo::timePerRep(1500, 3), 0, 1, 0));
    MetEvents events = m.start();
    events += drive(m, clock, 4500, 50);
    QCOMPARE(describeAll(events), QStringList()
        << "set@0 s1 r0 p0 a0 t0"
        << "rep@0 s1 r1 p0 a0 t0" << "beat@0 s1 r1 p0 a1 t0"
        << "rep@1500 s1 r2 p0 a0 t0" << "beat@1500 s1 r2 p0 a1 t0"
        << "rep@3000 s1 r3 p0 a0 t0" << "beat@3000 s1 r3 p0 a1 t0"
        << "setend@4500 s1 r0 p0 a0 t0" << "finished@4500 s1 r0 p0 a0 t0");
    QVERIFY(m.state() == MetronomeState::Finished);
}

void TstMetronome::repsAndTotalEvenlyAndWithRounding()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(Tempo::repsAndTotal(7, 20000), 0, 1, 0));
    MetEvents events = m.start();
    events += drive(m, clock, 20000, 7);
    const MetEvents beats = ofType(events, MetronomeEventType::Beat);
    QCOMPARE(beats.size(), 7);
    const qint64 expected[] = { 0, 2857, 5714, 8571, 11429, 14286, qint64(qRound(6 * 20000.0 / 7)) };
    for (int i = 0; i < 7; ++i) {
        QCOMPARE(beats.at(i).atMs, expected[i]);
        QCOMPARE(beats.at(i).repetition, i + 1);
        QVERIFY(beats.at(i).accent);
    }
    QCOMPARE(beats.at(6).atMs, qint64(17143));
    const MetEvents ends = ofType(events, MetronomeEventType::SetEnded);
    QCOMPARE(ends.size(), 1);
    QCOMPARE(ends.first().atMs, qint64(20000));
    QCOMPARE(ofType(events, MetronomeEventType::Finished).first().atMs, qint64(20000));

    FakeClock clock2;
    Metronome even(&clock2, makeConfig(Tempo::repsAndTotal(4, 6000), 0, 1, 0));
    MetEvents e2 = even.start();
    e2 += drive(even, clock2, 6000, 13);
    const MetEvents b2 = ofType(e2, MetronomeEventType::Beat);
    QCOMPARE(b2.size(), 4);
    QCOMPARE(b2.at(1).atMs, qint64(1500));
    QCOMPARE(b2.at(3).atMs, qint64(4500));
}

void TstMetronome::threePhaseSequence()
{
    FakeClock clock;
    // Phase 1 lasts 1 s, phase 2 lasts 1.5 s, phase 3 lasts 0.5 s.
    Metronome m(&clock, makeConfig(phasesOf({ 2, 3, 1 }, 2), 0, 1, 0));
    MetEvents events = m.start();
    events += drive(m, clock, 6000, 25);
    QCOMPARE(describeAll(events), QStringList()
        << "set@0 s1 r0 p0 a0 t0"
        << "rep@0 s1 r1 p0 a0 t0" << "beat@0 s1 r1 p1 a1 t0" << "beat@1000 s1 r1 p2 a0 t0" << "beat@2500 s1 r1 p3 a0 t0"
        << "rep@3000 s1 r2 p0 a0 t0" << "beat@3000 s1 r2 p1 a1 t0" << "beat@4000 s1 r2 p2 a0 t0" << "beat@5500 s1 r2 p3 a0 t0"
        << "setend@6000 s1 r0 p0 a0 t0" << "finished@6000 s1 r0 p0 a0 t0");

    // Eight phases: one beat each, only the first accented.
    FakeClock clock2;
    Metronome eight(&clock2, makeConfig(phasesOf({ 1, 1, 1, 1, 1, 1, 1, 1 }, 1), 0, 1, 0));
    MetEvents e2 = eight.start();
    e2 += drive(eight, clock2, 4000, 25);
    const MetEvents beats = ofType(e2, MetronomeEventType::Beat);
    QCOMPARE(beats.size(), 8);
    for (int i = 0; i < 8; ++i) {
        QCOMPARE(beats.at(i).phase, i + 1);
        QCOMPARE(beats.at(i).atMs, qint64(i * 500));
        QCOMPARE(beats.at(i).accent, i == 0);
    }
    QCOMPARE(countOf(e2, MetronomeEventType::Finished), 1);
}

void TstMetronome::aTryOfARhythmRunsRepetitionsWithoutSetsOrRest()
{
    // A try applies a saved rhythm (one repetition) to the number of repetitions entered on the
    // spot, with the lead-in, one set and no rest: the same machinery as the quick modes.
    const Tempo rhythm = phasesOf({ 2, 4 }, 1);
    const Tempo tried = rhythm.forReps(10);
    QVERIFY(tried.isValid());
    QCOMPARE(tried.reps(), 10);
    QCOMPARE(tried.phaseCount(), 2);
    FakeClock clock;
    Metronome m(&clock, makeConfig(tried, 2, 1, 0));
    QVERIFY(m.valid());
    MetEvents events = m.start();
    events += drive(m, clock, 40000, 50);
    QCOMPARE(countOf(events, MetronomeEventType::SetStarted), 1);
    QCOMPARE(countOf(events, MetronomeEventType::RepetitionStarted), 10);
    QCOMPARE(countOf(events, MetronomeEventType::Beat), 20);
    QCOMPARE(countOf(events, MetronomeEventType::RestStarted), 0);
    QCOMPARE(countOf(events, MetronomeEventType::Finished), 1);
    // Two seconds of lead-in, then ten repetitions of 3 s.
    QCOMPARE(ofType(events, MetronomeEventType::Finished).first().atMs, qint64(2000 + 30000));
    // The limits of a try: 1 to 999 repetitions of a valid rhythm.
    QVERIFY(!rhythm.forReps(0).isValid());
    QVERIFY(rhythm.forReps(999).isValid());
    QVERIFY(!rhythm.forReps(1000).isValid());
    // The longest rhythm (60 s) is a valid try too; the shortest (0.5 s) as well.
    QVERIFY(phasesOf({ 20, 20, 20, 20, 20, 20 }, 1).forReps(3).isValid());
    QVERIFY(phasesOf({ 1 }, 1).forReps(999).isValid());
}

void TstMetronome::beatsCarryTheirKind()
{
    const int counts[] = { 1, 2, 3, 8 };
    for (int n : counts) {
        FakeClock clock;
        QList<int> units;
        for (int i = 0; i < n; ++i)
            units.append(1);
        Metronome m(&clock, makeConfig(Tempo::phases(units, 2), 0, 1, 0));
        MetEvents events = m.start();
        events += drive(m, clock, qint64(n) * 1000, 25);
        const MetEvents beats = ofType(events, MetronomeEventType::Beat);
        QCOMPARE(beats.size(), 2 * n);
        for (int i = 0; i < beats.size(); ++i) {
            const int inRep = i % n;
            const BeatKind expected = inRep == 0 ? BeatKind::First
                                    : (inRep == n - 1 ? BeatKind::Last : BeatKind::Middle);
            QVERIFY(beats.at(i).beatKind == expected);
        }
    }

    // Forms (a) and (b): every beat is a first beat.
    FakeClock clock;
    Metronome m(&clock, makeConfig(Tempo::timePerRep(1000, 3), 0, 1, 0));
    MetEvents events = m.start();
    events += drive(m, clock, 3000, 25);
    for (const MetronomeEvent &e : ofType(events, MetronomeEventType::Beat))
        QVERIFY(e.beatKind == BeatKind::First);
}

static MetEvents fullRun(qint64 step)
{
    FakeClock clock;
    clock.set(123);
    Metronome m(&clock, makeConfig(Tempo::repsAndTotal(7, 20000), 3, 3, 12));
    MetEvents events = m.start();
    events += drive(m, clock, 3000 + 3 * 20000 + 2 * 12000 + 5000, step);
    return events;
}

void TstMetronome::eventsAreTheSameForAnyPollingStep()
{
    const QStringList reference = describeAll(fullRun(1));
    QVERIFY(reference.size() > 50);
    QCOMPARE(describeAll(fullRun(7)), reference);
    QCOMPARE(describeAll(fullRun(333)), reference);
    QCOMPARE(describeAll(fullRun(5000)), reference);
    QCOMPARE(describeAll(fullRun(1000000)), reference);
}

void TstMetronome::eventsComeOnceAndInOrder()
{
    const MetEvents events = fullRun(97);
    qint64 last = 0;
    for (const MetronomeEvent &e : events) {
        QVERIFY(e.atMs >= last);
        last = e.atMs;
    }
    QCOMPARE(countOf(events, MetronomeEventType::SetStarted), 3);
    QCOMPARE(countOf(events, MetronomeEventType::SetEnded), 3);
    QCOMPARE(countOf(events, MetronomeEventType::RestStarted), 2);
    QCOMPARE(countOf(events, MetronomeEventType::Finished), 1);
    QCOMPARE(countOf(events, MetronomeEventType::Beat), 21);
    QCOMPARE(countOf(events, MetronomeEventType::RepetitionStarted), 21);
    QCOMPARE(countOf(events, MetronomeEventType::Stopped), 0);
    // The last event is the finish.
    QVERIFY(events.last().type == MetronomeEventType::Finished);
    // At one instant: set ended, rest started; a repetition starts before its beat.
    for (int i = 0; i + 1 < events.size(); ++i) {
        if (events.at(i).type == MetronomeEventType::SetEnded && events.at(i + 1).atMs == events.at(i).atMs
            && events.at(i + 1).type != MetronomeEventType::Finished)
            QVERIFY(events.at(i + 1).type == MetronomeEventType::RestStarted);
        if (events.at(i).type == MetronomeEventType::RepetitionStarted)
            QVERIFY(events.at(i + 1).type == MetronomeEventType::Beat);
    }
}

void TstMetronome::setsAndRests()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(Tempo::timePerRep(1000, 2), 0, 3, 5));
    MetEvents events = m.start();
    events += drive(m, clock, 2000 + 5000 + 2000 + 5000 + 2000, 100);
    QCOMPARE(describeAll(ofType(events, MetronomeEventType::SetStarted)),
             QStringList() << "set@0 s1 r0 p0 a0 t0" << "set@7000 s2 r0 p0 a0 t0" << "set@14000 s3 r0 p0 a0 t0");
    QCOMPARE(describeAll(ofType(events, MetronomeEventType::SetEnded)),
             QStringList() << "setend@2000 s1 r0 p0 a0 t0" << "setend@9000 s2 r0 p0 a0 t0" << "setend@16000 s3 r0 p0 a0 t0");
    QCOMPARE(describeAll(ofType(events, MetronomeEventType::RestStarted)),
             QStringList() << "rest@2000 s1 r0 p0 a0 t0" << "rest@9000 s2 r0 p0 a0 t0");
    // A rest of 5 s ticks at 3, 2 and 1 s (10 s and 5 s are not below its length).
    QCOMPARE(describeAll(ofType(events, MetronomeEventType::RestTick)),
             QStringList() << "resttick@4000 s1 r0 p0 a0 t3" << "resttick@5000 s1 r0 p0 a0 t2" << "resttick@6000 s1 r0 p0 a0 t1"
                           << "resttick@11000 s2 r0 p0 a0 t3" << "resttick@12000 s2 r0 p0 a0 t2" << "resttick@13000 s2 r0 p0 a0 t1");
    QCOMPARE(describeAll(ofType(events, MetronomeEventType::Finished)), QStringList() << "finished@16000 s3 r0 p0 a0 t0");
}

void TstMetronome::restOfZeroStartsTheNextSetAtOnce()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(Tempo::timePerRep(1000, 2), 0, 3, 0));
    MetEvents events = m.start();
    events += drive(m, clock, 6000, 100);
    QCOMPARE(countOf(events, MetronomeEventType::RestStarted), 0);
    QCOMPARE(countOf(events, MetronomeEventType::RestTick), 0);
    QCOMPARE(describeAll(ofType(events, MetronomeEventType::SetStarted)),
             QStringList() << "set@0 s1 r0 p0 a0 t0" << "set@2000 s2 r0 p0 a0 t0" << "set@4000 s3 r0 p0 a0 t0");
    // The end of a set and the start of the next one are at the same instant, in that order.
    for (int i = 0; i + 1 < events.size(); ++i) {
        if (events.at(i).type == MetronomeEventType::SetEnded && events.at(i).set < 3)
            QVERIFY(events.at(i + 1).type == MetronomeEventType::SetStarted);
    }
    QCOMPARE(ofType(events, MetronomeEventType::Finished).first().atMs, qint64(6000));
}

void TstMetronome::lastSetHasNoRest()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(Tempo::timePerRep(1000, 2), 0, 1, 600));
    MetEvents events = m.start();
    events += drive(m, clock, 2000, 100);
    QCOMPARE(countOf(events, MetronomeEventType::RestStarted), 0);
    QVERIFY(m.state() == MetronomeState::Finished);
    QCOMPARE(m.restRemainingMs(), qint64(0));
    QCOMPARE(m.remainingMs(), qint64(0));
}

void TstMetronome::restTicksUseTheConfiguredList()
{
    FakeClock clock;
    MetronomeConfig c = makeConfig(Tempo::timePerRep(1000, 1), 0, 2, 40);
    c.tickSeconds = QList<int>() << 30 << 10 << 30 << 0 << -3 << 1;
    Metronome m(&clock, c);
    MetEvents events = m.start();
    events += drive(m, clock, 1000 + 40000, 100);
    const MetEvents ticks = ofType(events, MetronomeEventType::RestTick);
    QCOMPARE(ticks.size(), 3);
    QCOMPARE(ticks.at(0).remainingSec, 30);
    QCOMPARE(ticks.at(0).atMs, qint64(1000 + 10000));
    QCOMPARE(ticks.at(1).remainingSec, 10);
    QCOMPARE(ticks.at(2).remainingSec, 1);

    FakeClock clock2;
    MetronomeConfig none = makeConfig(Tempo::timePerRep(1000, 1), 4, 2, 10);
    none.tickSeconds.clear();
    Metronome quiet(&clock2, none);
    MetEvents e2 = quiet.start();
    e2 += drive(quiet, clock2, 4000 + 1000 + 10000 + 1000, 100);
    QCOMPARE(countOf(e2, MetronomeEventType::RestTick), 0);
    QCOMPARE(countOf(e2, MetronomeEventType::LeadInTick), 0);
}

void TstMetronome::noDriftOverSixtyMinutes()
{
    FakeClock clock;
    clock.set(777);
    const Tempo t = Tempo::repsAndTotal(999, 3600000);
    Metronome m(&clock, makeConfig(t, 0, 1, 0));
    MetEvents events = m.start();
    events += drive(m, clock, 3600000, 37);
    const MetEvents beats = ofType(events, MetronomeEventType::Beat);
    QCOMPARE(beats.size(), 999);
    for (int i = 0; i < beats.size(); ++i) {
        const double exact = 777.0 + double(i) * 3600000.0 / 999.0;
        QVERIFY2(qAbs(double(beats.at(i).atMs) - exact) <= 0.5 + 1e-6, qPrintable(QString::number(i)));
    }
    QCOMPARE(ofType(events, MetronomeEventType::SetEnded).first().atMs, qint64(777 + 3600000));
    QVERIFY(m.state() == MetronomeState::Finished);

    // Phases: 50 minutes of repetitions of 5 s.
    FakeClock clock2;
    Metronome p(&clock2, makeConfig(phasesOf({ 4, 2, 3, 1 }, 600), 0, 1, 0));
    MetEvents e2 = p.start();
    e2 += drive(p, clock2, 3000000, 41);
    const MetEvents b2 = ofType(e2, MetronomeEventType::Beat);
    QCOMPARE(b2.size(), 2400);
    QCOMPARE(b2.last().atMs, qint64(599 * 5000 + 4500));
    QCOMPARE(ofType(e2, MetronomeEventType::Finished).first().atMs, qint64(3000000));
}

void TstMetronome::noDriftWithSetsAndRests()
{
    FakeClock clock;
    const int sets = 20;
    Metronome m(&clock, makeConfig(Tempo::repsAndTotal(97, 180000), 10, sets, 7));
    MetEvents events = m.start();
    const qint64 total = 10000 + sets * 180000 + (sets - 1) * 7000;
    events += drive(m, clock, total, 251);
    const MetEvents started = ofType(events, MetronomeEventType::SetStarted);
    QCOMPARE(started.size(), sets);
    for (int i = 0; i < sets; ++i)
        QCOMPARE(started.at(i).atMs, qint64(10000 + i * 187000));
    QCOMPARE(ofType(events, MetronomeEventType::Finished).first().atMs, total);
    const MetEvents beats = ofType(events, MetronomeEventType::Beat);
    QCOMPARE(beats.size(), sets * 97);
    const Tempo t = Tempo::repsAndTotal(97, 180000);
    for (int s = 0; s < sets; ++s) {
        for (int r = 0; r < 97; r += 13)
            QCOMPARE(beats.at(s * 97 + r).atMs, qint64(10000 + s * 187000) + t.repStartMs(r));
    }
}

void TstMetronome::lateUpdateReportsScheduledTimes()
{
    FakeClock clock;
    clock.set(1000);
    Metronome m(&clock, makeConfig(Tempo::timePerRep(1000, 3), 0, 1, 0));
    MetEvents events = m.start();
    clock.advance(10000);
    events += m.update();
    const MetEvents beats = ofType(events, MetronomeEventType::Beat);
    QCOMPARE(beats.size(), 3);
    QCOMPARE(beats.at(1).atMs, qint64(2000));
    QCOMPARE(beats.at(2).atMs, qint64(3000));
    QCOMPARE(ofType(events, MetronomeEventType::Finished).first().atMs, qint64(4000));
    QVERIFY(m.update().isEmpty());
}

void TstMetronome::pauseInLeadIn()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(Tempo::timePerRep(1000, 1), 6, 1, 0));
    m.start();
    clock.advance(2500);
    MetEvents events = m.update();   // the tick for 5 s (at 1 s) is reported
    QCOMPARE(countOf(events, MetronomeEventType::LeadInTick), 1);
    clock.advance(1500);              // 4 s in, 2 s left: the ticks for 3 s and 2 s are due
    events = m.pause();
    QCOMPARE(countOf(events, MetronomeEventType::LeadInTick), 2);
    QVERIFY(m.state() == MetronomeState::Paused);
    QCOMPARE(m.leadInRemainingMs(), qint64(2000));
    clock.advance(100000);
    QVERIFY(m.update().isEmpty());
    QCOMPARE(m.leadInRemainingMs(), qint64(2000));
    QVERIFY(m.resume().isEmpty());
    QVERIFY(m.state() == MetronomeState::LeadIn);
    clock.advance(2000);
    events = m.update();
    // Only the tick for 1 s is new; the earlier ones are not repeated.
    const MetEvents ticks = ofType(events, MetronomeEventType::LeadInTick);
    QCOMPARE(ticks.size(), 1);
    QCOMPARE(ticks.first().remainingSec, 1);
    QCOMPARE(countOf(events, MetronomeEventType::SetStarted), 1);
    QCOMPARE(ofType(events, MetronomeEventType::SetStarted).first().atMs, qint64(106000));
}

void TstMetronome::pauseInTheMiddleOfARepetition()
{
    FakeClock clock;
    clock.set(1000);
    Metronome m(&clock, makeConfig(Tempo::timePerRep(4000, 3), 0, 1, 0));
    m.start();
    clock.advance(4000 + 2500);       // in repetition 2, 2.5 s in
    MetEvents events = m.pause();
    QCOMPARE(countOf(events, MetronomeEventType::Beat), 1);   // the beat of repetition 2
    QVERIFY(m.state() == MetronomeState::Paused);
    QCOMPARE(m.repetitionNumber(), 2);
    QCOMPARE(m.repetitionFraction(), 0.0);
    // The lost 2.5 s of the repetition do not count as elapsed: it is counted from its start.
    QCOMPARE(m.remainingMs(), qint64(8000));
    clock.advance(50000);
    QVERIFY(m.update().isEmpty());
    events = m.resume();
    QCOMPARE(describeAll(events), QStringList() << "rep@57500 s1 r2 p0 a0 t0" << "beat@57500 s1 r2 p0 a1 t0");
    QVERIFY(m.state() == MetronomeState::Running);
    QCOMPARE(m.repetitionNumber(), 2);
    QCOMPARE(m.remainingMs(), qint64(8000));
    clock.advance(4000);
    events = m.update();
    QCOMPARE(describeAll(events), QStringList() << "rep@61500 s1 r3 p0 a0 t0" << "beat@61500 s1 r3 p0 a1 t0");
    clock.advance(4000);
    events = m.update();
    QCOMPARE(describeAll(events), QStringList() << "setend@65500 s1 r0 p0 a0 t0" << "finished@65500 s1 r0 p0 a0 t0");
}

void TstMetronome::pauseInPhasesRestartsAtPhaseOne()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(phasesOf({ 2, 2, 2, 2 }, 2), 0, 1, 0));   // repetitions of 4 s
    m.start();
    clock.advance(2500);   // in the third phase of repetition 1
    m.pause();
    QCOMPARE(m.repetitionNumber(), 1);
    QCOMPARE(m.phaseNumber(), 1);
    clock.advance(9000);
    const MetEvents events = m.resume();
    QCOMPARE(describeAll(events), QStringList() << "rep@11500 s1 r1 p0 a0 t0" << "beat@11500 s1 r1 p1 a1 t0");
    QCOMPARE(m.phaseNumber(), 1);
    clock.advance(500);
    QCOMPARE(m.phaseNumber(), 1);
    clock.advance(500);
    QCOMPARE(m.phaseNumber(), 2);
}

void TstMetronome::pauseAtARepetitionStart()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(Tempo::timePerRep(2000, 3), 0, 1, 0));
    m.start();
    clock.advance(2000);
    // The beat of repetition 2 is reported by the pause; the resume starts the repetition again.
    MetEvents events = m.pause();
    QCOMPARE(countOf(events, MetronomeEventType::Beat), 1);
    QCOMPARE(m.repetitionNumber(), 2);
    clock.advance(500);
    events = m.resume();
    QCOMPARE(describeAll(events), QStringList() << "rep@2500 s1 r2 p0 a0 t0" << "beat@2500 s1 r2 p0 a1 t0");
}

void TstMetronome::pauseInARest()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(Tempo::timePerRep(1000, 1), 0, 2, 20));
    m.start();
    clock.advance(1000 + 12000);   // rest of 20 s, 12 s in: the tick for 10 s is due, 8 s left
    MetEvents events = m.pause();
    QCOMPARE(countOf(events, MetronomeEventType::RestTick), 1);
    QVERIFY(m.state() == MetronomeState::Paused);
    QCOMPARE(m.restRemainingMs(), qint64(8000));
    QCOMPARE(m.remainingMs(), qint64(8000 + 1000));
    clock.advance(70000);
    QVERIFY(m.update().isEmpty());
    QCOMPARE(m.restRemainingMs(), qint64(8000));
    QVERIFY(m.resume().isEmpty());
    QVERIFY(m.state() == MetronomeState::Resting);
    clock.advance(8000);
    events = m.update();
    // 5, 3, 2 and 1 s were not reported before; 10 s is not repeated.
    const MetEvents ticks = ofType(events, MetronomeEventType::RestTick);
    QCOMPARE(ticks.size(), 4);
    QCOMPARE(ticks.at(0).remainingSec, 5);
    QCOMPARE(countOf(events, MetronomeEventType::SetStarted), 1);
    QCOMPARE(ofType(events, MetronomeEventType::SetStarted).first().atMs, qint64(1000 + 12000 + 70000 + 8000));
}

void TstMetronome::pauseWhenNotRunningDoesNothing()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(Tempo::timePerRep(1000, 1), 0, 1, 0));
    QVERIFY(m.pause().isEmpty());
    QVERIFY(m.resume().isEmpty());
    QVERIFY(m.state() == MetronomeState::Ready);
    m.start();
    m.pause();
    QVERIFY(m.pause().isEmpty());
    QVERIFY(m.state() == MetronomeState::Paused);
    m.resume();
    QVERIFY(m.resume().isEmpty());
    QVERIFY(m.state() == MetronomeState::Running);
    clock.advance(5000);
    m.update();
    QVERIFY(m.state() == MetronomeState::Finished);
    QVERIFY(m.pause().isEmpty());
    QVERIFY(m.resume().isEmpty());
    QVERIFY(m.state() == MetronomeState::Finished);
}

void TstMetronome::repeatedPausesAndResumes()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(Tempo::timePerRep(1000, 20), 2, 2, 3));
    m.start();
    int resumedBeats = 0;
    for (int i = 0; i < 200 && m.state() != MetronomeState::Finished; ++i) {
        clock.advance(400);
        m.update();
        m.pause();
        clock.advance(1000);
        resumedBeats += countOf(m.resume(), MetronomeEventType::Beat);
    }
    // Each cycle runs 0.4 s of a 1 s repetition, so a repetition never completes while pausing
    // that often: the run is stuck at its repetition and loses no time.
    QVERIFY(m.state() == MetronomeState::Running || m.state() == MetronomeState::LeadIn);
    QCOMPARE(m.setNumber(), 1);
    QVERIFY(resumedBeats > 0);
}

void TstMetronome::stopFromEveryState()
{
    const MetronomeConfig c = makeConfig(Tempo::timePerRep(1000, 2), 3, 2, 5);
    struct Case { qint64 at; bool pauseFirst; };
    const Case cases[] = { { 1000, false }, { 1000, true }, { 3500, false }, { 3500, true },
                           { 3000 + 2000 + 2000, false }, { 3000 + 2000 + 2000, true } };
    for (const Case &k : cases) {
        FakeClock clock;
        Metronome m(&clock, c);
        QVERIFY(m.stop().isEmpty());       // ready: nothing to stop
        QVERIFY(m.state() == MetronomeState::Ready);
        m.start();
        clock.advance(k.at);
        MetEvents before = m.update();
        if (k.pauseFirst)
            before += m.pause();
        const MetEvents events = m.stop();
        QVERIFY(m.state() == MetronomeState::Stopped);
        QCOMPARE(countOf(events, MetronomeEventType::Stopped), 1);
        QVERIFY(events.last().type == MetronomeEventType::Stopped);
        QCOMPARE(events.last().atMs, clock.nowMs());
        clock.advance(100000);
        QVERIFY(m.update().isEmpty());
        QCOMPARE(m.remainingMs(), qint64(0));
    }
}

void TstMetronome::stoppedAndFinishedStayDone()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(Tempo::timePerRep(1000, 1), 0, 1, 0));
    m.start();
    m.stop();
    QVERIFY(m.start().isEmpty());
    QVERIFY(m.resume().isEmpty());
    QVERIFY(m.pause().isEmpty());
    QVERIFY(m.stop().isEmpty());
    QVERIFY(m.state() == MetronomeState::Stopped);

    Metronome f(&clock, makeConfig(Tempo::timePerRep(1000, 1), 0, 1, 0));
    f.start();
    clock.advance(1000);
    // A stop that finds the run over reports the end and does not stop it.
    const MetEvents events = f.stop();
    QCOMPARE(countOf(events, MetronomeEventType::Finished), 1);
    QCOMPARE(countOf(events, MetronomeEventType::Stopped), 0);
    QVERIFY(f.state() == MetronomeState::Finished);
}

void TstMetronome::startTwiceDoesNothing()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(Tempo::timePerRep(1000, 3), 0, 1, 0));
    QVERIFY(!m.start().isEmpty());
    clock.advance(500);
    QVERIFY(m.start().isEmpty());
    QCOMPARE(m.remainingMs(), qint64(2500));
}

void TstMetronome::queriesInALeadInAndASet()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(phasesOf({ 2, 2, 2 }, 3), 4, 2, 6));
    QCOMPARE(m.setNumber(), 0);
    QCOMPARE(m.setCount(), 2);
    QCOMPARE(m.setsLeft(), 1);
    QCOMPARE(m.repetitionCount(), 3);
    QCOMPARE(m.repetitionNumber(), 0);
    m.start();
    QCOMPARE(m.setNumber(), 1);
    QCOMPARE(m.repetitionNumber(), 0);
    QCOMPARE(m.phaseNumber(), 0);
    QCOMPARE(m.leadInRemainingMs(), qint64(4000));
    clock.advance(1500);
    QCOMPARE(m.leadInRemainingMs(), qint64(2500));
    clock.advance(2500);
    m.update();
    QVERIFY(m.state() == MetronomeState::Running);
    QCOMPARE(m.leadInRemainingMs(), qint64(0));
    QCOMPARE(m.repetitionNumber(), 1);
    QCOMPARE(m.phaseNumber(), 1);
    clock.advance(1000);
    QCOMPARE(m.phaseNumber(), 2);
    clock.advance(1000);
    QCOMPARE(m.phaseNumber(), 3);
    clock.advance(1000);
    QCOMPARE(m.repetitionNumber(), 2);
    QCOMPARE(m.phaseNumber(), 1);
    QCOMPARE(m.setsLeft(), 1);
}

void TstMetronome::fractionsOfARepetitionAndAPhase()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(phasesOf({ 2, 4, 2 }, 2), 0, 1, 0));   // 1 s, 2 s, 1 s: 4 s
    QCOMPARE(m.repetitionFraction(), 0.0);
    m.start();
    QCOMPARE(m.repetitionFraction(), 0.0);
    QCOMPARE(m.phaseFraction(), 0.0);
    clock.advance(500);
    QCOMPARE(m.repetitionFraction(), 0.125);
    QCOMPARE(m.phaseFraction(), 0.5);
    clock.advance(1500);   // 2 s: 1 s into phase 2
    QCOMPARE(m.repetitionFraction(), 0.5);
    QCOMPARE(m.phaseFraction(), 0.5);
    clock.advance(2000);   // the second repetition starts
    m.update();
    QCOMPARE(m.repetitionFraction(), 0.0);
    QCOMPARE(m.phaseFraction(), 0.0);

    FakeClock clock2;
    Metronome plain(&clock2, makeConfig(Tempo::timePerRep(2000, 2), 0, 1, 0));
    plain.start();
    clock2.advance(500);
    QCOMPARE(plain.repetitionFraction(), 0.25);
    QCOMPARE(plain.phaseFraction(), 0.25);
    QCOMPARE(plain.phaseNumber(), 0);
}

void TstMetronome::queriesDuringARest()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(Tempo::timePerRep(1000, 2), 0, 2, 9));
    m.start();
    clock.advance(2000 + 3000);
    m.update();
    QVERIFY(m.state() == MetronomeState::Resting);
    QCOMPARE(m.setNumber(), 1);
    QCOMPARE(m.setsLeft(), 1);
    QCOMPARE(m.restRemainingMs(), qint64(6000));
    QCOMPARE(m.repetitionNumber(), 0);
    QCOMPARE(m.phaseNumber(), 0);
    QCOMPARE(m.repetitionFraction(), 0.0);
    clock.advance(6000);
    m.update();
    QCOMPARE(m.setNumber(), 2);
    QCOMPARE(m.setsLeft(), 0);
    QCOMPARE(m.restRemainingMs(), qint64(0));
}

void TstMetronome::remainingTimeOfTheWholeRun()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(Tempo::timePerRep(1000, 4), 3, 3, 10));
    const qint64 total = 3000 + 3 * 4000 + 2 * 10000;
    QCOMPARE(m.remainingMs(), total);
    m.start();
    QCOMPARE(m.remainingMs(), total);
    qint64 elapsed = 0;
    while (m.state() != MetronomeState::Finished) {
        clock.advance(777);
        elapsed += 777;
        m.update();
        if (m.state() == MetronomeState::Finished)
            break;
        QCOMPARE(m.remainingMs(), total - elapsed);
    }
    QCOMPARE(m.remainingMs(), qint64(0));
}

void TstMetronome::remainingTimeWhilePaused()
{
    FakeClock clock;
    Metronome m(&clock, makeConfig(Tempo::timePerRep(4000, 3), 2, 2, 10));
    m.start();
    m.pause();   // in the lead-in
    QCOMPARE(m.remainingMs(), qint64(2000 + 12000 + 10000 + 12000));
    m.resume();
    clock.advance(2000 + 4000 + 1500);   // 1.5 s into repetition 2
    m.update();
    m.pause();
    // The current repetition counts from its start.
    QCOMPARE(m.remainingMs(), qint64(8000 + 10000 + 12000));
    m.resume();
    QCOMPARE(m.remainingMs(), qint64(8000 + 10000 + 12000));
}

QTEST_APPLESS_MAIN(TstMetronome)
#include "tst_metronome.moc"

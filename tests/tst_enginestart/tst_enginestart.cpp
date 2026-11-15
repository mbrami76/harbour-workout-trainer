// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "enginehelpers.h"

class TstEngineStart : public QObject
{
    Q_OBJECT

private slots:
    void firstStepNeverWaits();
    void secondStepFlaggedWaits();
    void firstStepOfALaterBlockWaits();
    void middleStepWaits();
    void waitWithoutLeadIn();
    void waitingTimeIsNotStepTime();
    void startIsIgnoredOutsideTheWait();
    void pauseWhileWaiting();
    void nextWhileWaiting();
    void nextIntoAStepThatWaits();
    void stopWhileWaiting();
    void skipToAWaitingStepWhilePaused();
    void pauseInTheLeadInAfterTheTapDoesNotWaitAgain();
    void restAndRepetitionStepsCanWait();
    void everyStepWaiting();
    void roundsWaitAgainInEveryRound();
    void insertedRestsNeverWait();
    void readOnlyStateWhileWaiting();
    void leadInTicksStartAtTheTap();
    void plainStepsKeepTheirBehaviour();
};

static Workout single(const QList<Step> &steps, int rounds = 1, int restBetweenRounds = 0)
{
    return workout(QList<Block>() << block(steps, rounds, restBetweenRounds));
}

void TstEngineStart::firstStepNeverWaits()
{
    // The first step is flagged, but the Start that opened the session is its trigger.
    EngineRig r;
    r.make(single(QList<Step>() << onTap(work("a", 5)) << work("b", 5)), 3, QList<int>());
    r.take(r.engine->start());
    QVERIFY(r.is(SessionState::LeadIn));
    QCOMPARE(r.log.size(), 0);
    r.at(3000);
    QVERIFY(r.is(SessionState::Running));
    r.at(8000);
    r.at(11000);
    r.at(16000);
    QCOMPARE(describeAll(r.log), QStringList()
        << "step@3000 i0" << "step@11000 i1" << "finished@16000 i1");
    QCOMPARE(countOf(r.log, EventType::WaitingForStart), 0);
}

void TstEngineStart::secondStepFlaggedWaits()
{
    EngineRig r;
    r.make(single(QList<Step>() << onTap(work("a", 5)) << onTap(work("b", 5))), 0, QList<int>());
    r.take(r.engine->start());
    QVERIFY(r.is(SessionState::Running));
    r.at(5000);
    QVERIFY(r.is(SessionState::WaitingToStart));
    QCOMPARE(r.engine->stepIndex(), 1);
    QCOMPARE(describeAll(r.log), QStringList() << "step@0 i0" << "wait@5000 i1");
}

void TstEngineStart::firstStepOfALaterBlockWaits()
{
    EngineRig r;
    r.make(workout(QList<Block>()
        << block(QList<Step>() << work("a", 3))
        << block(QList<Step>() << onTap(work("c", 3)))), 0, QList<int>());
    r.take(r.engine->start());
    r.at(3000);
    QVERIFY(r.is(SessionState::WaitingToStart));
    QCOMPARE(r.engine->currentStep().blockIndex, 1);
    QCOMPARE(r.engine->stepIndex(), 1);
}

void TstEngineStart::middleStepWaits()
{
    EngineRig r;
    r.make(single(QList<Step>() << work("a", 5) << onTap(work("b", 5)) << work("c", 5)), 0, QList<int>());
    r.take(r.engine->start());
    r.at(5000);
    QVERIFY(r.is(SessionState::WaitingToStart));
    QCOMPARE(r.engine->stepIndex(), 1);
    QCOMPARE(describeAll(r.log), QStringList() << "step@0 i0" << "wait@5000 i1");
    r.at(50000);
    QCOMPARE(r.log.size(), 2);
    r.take(r.engine->start());
    QVERIFY(r.is(SessionState::Running));
    r.at(55000);
    r.at(60000);
    QCOMPARE(describeAll(r.log), QStringList()
        << "step@0 i0" << "wait@5000 i1" << "step@50000 i1" << "step@55000 i2" << "finished@60000 i2");
}

void TstEngineStart::waitWithoutLeadIn()
{
    EngineRig r;
    r.make(single(QList<Step>() << work("a", 2) << onTap(work("b", 5))), 0, QList<int>());
    r.take(r.engine->start());
    r.at(2000);
    QVERIFY(r.is(SessionState::WaitingToStart));
    r.clock.set(7000);
    r.take(r.engine->start());
    QVERIFY(r.is(SessionState::Running));
    QCOMPARE(r.engine->stepRemainingMs(), qint64(5000));
    r.at(12000);
    QCOMPARE(describeAll(r.log), QStringList() << "step@0 i0" << "wait@2000 i1" << "step@7000 i1" << "finished@12000 i1");
}

void TstEngineStart::waitingTimeIsNotStepTime()
{
    EngineRig r;
    r.make(single(QList<Step>() << work("a", 5) << onTap(work("b", 5)) << work("c", 5)), 3, QList<int>());
    r.take(r.engine->start());
    r.at(3000);
    r.at(8000);                       // a is over, b waits
    QVERIFY(r.is(SessionState::WaitingToStart));
    r.at(608000);                     // ten minutes of waiting
    r.take(r.engine->start());
    r.at(611000);
    r.at(616000);                     // b over
    r.at(619000);
    r.at(624000);
    QVERIFY(r.is(SessionState::Finished));
    QList<qint64> active;
    for (const StepRecord &record : r.engine->records())
        active.append(record.activeMs);
    QCOMPARE(active, QList<qint64>() << 5000 << 5000 << 5000);
    // The log carries the step seconds only.
    const LogEntry entry = buildLogEntry("W", 0, r.engine->plan(), r.engine->records(), EndState::Completed,
                                         [](const QString &key, QString *name, MuscleSet *muscles) { *name = key; *muscles = 1; return true; });
    QCOMPARE(entry.totalSec, 15);
    QCOMPARE(entry.steps.size(), 3);
    QCOMPARE(entry.steps.at(1).actualSec, 5);
}

void TstEngineStart::startIsIgnoredOutsideTheWait()
{
    EngineRig r;
    r.make(single(QList<Step>() << work("a", 5) << onTap(work("b", 5))), 0, QList<int>());
    r.take(r.engine->start());
    r.clock.set(2000);
    QVERIFY(r.engine->start().isEmpty());   // running
    QCOMPARE(r.engine->stepIndex(), 0);
    r.at(5000);
    r.take(r.engine->start());
    r.at(10000);
    QVERIFY(r.is(SessionState::Finished));
    QVERIFY(r.engine->start().isEmpty());   // finished
}

void TstEngineStart::pauseWhileWaiting()
{
    EngineRig r;
    r.make(single(QList<Step>() << work("a", 5) << onTap(work("b", 5))), 0, QList<int>());
    r.take(r.engine->start());
    r.at(5000);
    r.clock.set(6000);
    r.take(r.engine->pause());
    QVERIFY(r.is(SessionState::Paused));
    r.clock.set(60000);
    r.take(r.engine->resume());
    // Back at the wait, without a second event for the same wait.
    QVERIFY(r.is(SessionState::WaitingToStart));
    QCOMPARE(countOf(r.log, EventType::WaitingForStart), 1);
    r.take(r.engine->start());
    QVERIFY(r.is(SessionState::Running));
    QCOMPARE(r.engine->stepIndex(), 1);
    r.at(65000);
    QVERIFY(r.is(SessionState::Finished));
}

void TstEngineStart::nextWhileWaiting()
{
    EngineRig r;
    r.make(single(QList<Step>() << work("a", 5) << onTap(work("b", 5)) << work("c", 5)), 0, QList<int>());
    r.take(r.engine->start());
    r.at(5000);
    r.clock.set(9000);
    r.take(r.engine->next());
    QVERIFY(r.is(SessionState::Running));
    QCOMPARE(r.engine->stepIndex(), 2);
    r.at(14000);
    QVERIFY(r.is(SessionState::Finished));
}

void TstEngineStart::nextIntoAStepThatWaits()
{
    EngineRig r;
    r.make(single(QList<Step>() << work("a", 5) << onTap(work("b", 5)) << onTap(work("c", 5))), 0, QList<int>());
    r.take(r.engine->start());
    r.clock.set(1000);
    r.take(r.engine->next());
    QVERIFY(r.is(SessionState::WaitingToStart));
    QCOMPARE(r.engine->stepIndex(), 1);
    r.clock.set(2000);
    r.take(r.engine->next());
    QVERIFY(r.is(SessionState::WaitingToStart));
    QCOMPARE(r.engine->stepIndex(), 2);
    QCOMPARE(countOf(r.log, EventType::WaitingForStart), 2);
    r.clock.set(3000);
    r.take(r.engine->next());
    QVERIFY(r.is(SessionState::Finished));
}

void TstEngineStart::stopWhileWaiting()
{
    EngineRig r;
    r.make(single(QList<Step>() << work("a", 5) << onTap(work("b", 5))), 0, QList<int>());
    r.take(r.engine->start());
    r.at(5000);
    r.clock.set(30000);
    r.take(r.engine->stop());
    QVERIFY(r.is(SessionState::Stopped));
    QCOMPARE(r.log.last().type, EventType::Stopped);
    QCOMPARE(r.log.last().atMs, qint64(30000));
    QCOMPARE(r.engine->records().last().activeMs, qint64(0));
    // A stopped session ignores the tap.
    QVERIFY(r.engine->start().isEmpty());
}

void TstEngineStart::skipToAWaitingStepWhilePaused()
{
    EngineRig r;
    r.make(single(QList<Step>() << work("a", 10) << onTap(work("b", 5))), 0, QList<int>());
    r.take(r.engine->start());
    r.clock.set(2000);
    r.take(r.engine->pause());
    r.clock.set(3000);
    r.take(r.engine->next());
    QVERIFY(r.is(SessionState::Paused));
    QCOMPARE(r.engine->stepIndex(), 1);
    r.clock.set(4000);
    r.take(r.engine->resume());
    QVERIFY(r.is(SessionState::WaitingToStart));
    QCOMPARE(countOf(r.log, EventType::WaitingForStart), 1);
    QCOMPARE(r.log.last().atMs, qint64(4000));
}

void TstEngineStart::pauseInTheLeadInAfterTheTapDoesNotWaitAgain()
{
    EngineRig r;
    r.make(single(QList<Step>() << work("x", 1) << onTap(work("a", 5))), 3, QList<int>());
    r.take(r.engine->start());
    r.at(3000);
    r.at(4000);
    QVERIFY(r.is(SessionState::WaitingToStart));
    r.clock.set(5000);
    r.take(r.engine->start());
    QVERIFY(r.is(SessionState::LeadIn));
    r.clock.set(6000);
    r.take(r.engine->pause());
    r.clock.set(13000);
    r.take(r.engine->resume());
    QVERIFY(r.is(SessionState::LeadIn));
    QCOMPARE(countOf(r.log, EventType::WaitingForStart), 1);
    r.at(16000);
    QVERIFY(r.is(SessionState::Running));
    // Pausing a running step that started on a tap does not make it wait again either.
    r.clock.set(17000);
    r.take(r.engine->pause());
    r.clock.set(18000);
    r.take(r.engine->resume());
    QVERIFY(r.is(SessionState::LeadIn));
}

void TstEngineStart::restAndRepetitionStepsCanWait()
{
    EngineRig r;
    r.make(single(QList<Step>() << work("a", 5) << onTap(rest(10)) << onTap(repStep("b", 8))), 3, QList<int>());
    r.take(r.engine->start());
    r.at(3000);
    r.at(8000);
    QVERIFY(r.is(SessionState::WaitingToStart));
    QCOMPARE(r.engine->currentStep().type, StepType::Rest);
    r.clock.set(20000);
    r.take(r.engine->start());
    QVERIFY(r.is(SessionState::Running));       // a rest has no lead-in
    QCOMPARE(r.engine->stepRemainingMs(), qint64(10000));
    r.at(30000);
    QVERIFY(r.is(SessionState::WaitingToStart));
    QCOMPARE(r.engine->stepIndex(), 2);
    r.clock.set(40000);
    r.take(r.engine->start());
    QVERIFY(r.is(SessionState::LeadIn));
    r.at(43000);
    QVERIFY(r.is(SessionState::WaitingForReps));
    r.take(r.engine->done());
    QVERIFY(r.is(SessionState::Finished));
}

void TstEngineStart::everyStepWaiting()
{
    // Every step but the first waits.
    EngineRig r;
    r.make(single(QList<Step>() << onTap(work("a", 2)) << onTap(work("b", 2)) << onTap(work("c", 2))), 0, QList<int>());
    r.take(r.engine->start());
    QVERIFY(r.is(SessionState::Running));
    r.at(2000);
    for (int i = 1; i < 3; ++i) {
        QVERIFY(r.is(SessionState::WaitingToStart));
        QCOMPARE(r.engine->stepIndex(), i);
        r.clock.set(r.clock.nowMs() + 5000);
        r.take(r.engine->start());
        r.at(r.clock.nowMs() + 2000);
    }
    QVERIFY(r.is(SessionState::Finished));
    QCOMPARE(countOf(r.log, EventType::WaitingForStart), 2);
}

void TstEngineStart::roundsWaitAgainInEveryRound()
{
    // The first step of the first round starts with the session; the same step in later rounds waits.
    EngineRig r;
    r.make(single(QList<Step>() << onTap(work("a", 2)), 3, 4), 0, QList<int>());
    r.take(r.engine->start());
    QVERIFY(r.is(SessionState::Running));
    int taps = 0;
    while (!r.is(SessionState::Finished) && taps < 10) {
        if (r.is(SessionState::WaitingToStart)) {
            ++taps;
            r.clock.set(r.clock.nowMs() + 1000);
            r.take(r.engine->start());
        } else {
            r.at(r.clock.nowMs() + 500);
        }
    }
    QCOMPARE(taps, 2);
    QCOMPARE(countOf(r.log, EventType::RoundChanged), 2);
    // The rest between rounds ran by itself.
    QCOMPARE(countOf(r.log, EventType::WaitingForStart), 2);
}

void TstEngineStart::insertedRestsNeverWait()
{
    EngineRig r;
    r.make(single(QList<Step>() << work("a", 2), 2, 4), 0, QList<int>());
    r.take(r.engine->start());
    r.at(2000);
    QVERIFY(r.is(SessionState::Running));
    QCOMPARE(r.engine->currentStep().insertedRest, true);
    QVERIFY(!r.engine->currentStep().startOnTap);
}

void TstEngineStart::readOnlyStateWhileWaiting()
{
    EngineRig r;
    r.make(single(QList<Step>() << work("a", 5) << onTap(work("b", 30))), 3, QList<int>());
    r.take(r.engine->start());
    r.at(3000);
    r.at(8000);
    QVERIFY(r.is(SessionState::WaitingToStart));
    QCOMPARE(r.engine->stepRemainingMs(), qint64(30000));
    QCOMPARE(r.engine->leadInRemainingMs(), qint64(0));
    QCOMPARE(r.engine->currentStepActiveMs(), qint64(0));
    bool open = true;
    QCOMPARE(r.engine->plannedRemainingMs(&open), qint64(30000));
    QVERIFY(!open);
}

void TstEngineStart::leadInTicksStartAtTheTap()
{
    EngineRig r;
    r.make(single(QList<Step>() << work("x", 1) << onTap(work("a", 20))), 5, QList<int>() << 3 << 1);
    r.take(r.engine->start());
    r.at(5000);
    r.at(6000);
    QVERIFY(r.is(SessionState::WaitingToStart));
    r.at(60000);
    const int before = countOf(r.log, EventType::Tick);
    r.take(r.engine->start());
    r.at(65000);
    const EngEvents ticks = ofType(r.log, EventType::Tick);
    QCOMPARE(ticks.size() - before, 2);
    QCOMPARE(ticks.at(before).atMs, qint64(62000));
    QCOMPARE(ticks.at(before + 1).atMs, qint64(64000));
    QVERIFY(ticks.at(before).inLeadIn);
}

void TstEngineStart::plainStepsKeepTheirBehaviour()
{
    EngineRig r;
    r.make(single(QList<Step>() << work("a", 5) << work("b", 5)), 3, QList<int>());
    r.take(r.engine->start());
    QVERIFY(r.is(SessionState::LeadIn));
    r.at(3000);
    r.at(8000);
    r.at(11000);
    r.at(16000);
    QCOMPARE(countOf(r.log, EventType::WaitingForStart), 0);
    QCOMPARE(describeAll(r.log), QStringList() << "step@3000 i0" << "step@11000 i1" << "finished@16000 i1");
}

QTEST_APPLESS_MAIN(TstEngineStart)
#include "tst_enginestart.moc"

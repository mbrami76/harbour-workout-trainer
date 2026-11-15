// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "enginehelpers.h"

class TstEngineTempo : public QObject
{
    Q_OBJECT

private slots:
    void timePerRepBeats();
    void repsAndTotalBeatsDoNotDrift();
    void threePhaseBeats();
    void tempoStepRunsAsATimedStep();
    void noTicksInATempoStep();
    void sixtyMinutesWithoutDrift();
    void beatsDoNotDependOnThePollingStep();
    void doneEndsTheStepEarly();
    void doneIsIgnoredInTheLeadIn();
    void pauseRestartsTheRepetition();
    void pauseWithALeadIn();
    void pauseInPhasesRestartsAtThePhaseOne();
    void pauseAtTheStartOfARepetition();
    void secondsInTheLogAreTheActiveTime();
    void stepEndsExactlyAndTheRestFollows();
    void startOnTapAndTempo();
    void nextEndsATempoStep();
    void remainingTimeOfTheStep();
    void roundsRepeatTheTempo();
    void repetitionStepWithoutTempoStillWaitsForDone();
    void plannedRemainingCountsTempoSteps();
    void stepRunsItsOwnRepetitionsAtTheTempoOfItsExercise();
    void aTimedStepOfTheSameExerciseIgnoresTheTempo();
    void exerciseWithoutAMetronomeWaitsForDone();
    void tempoElapsedFollowsTheSetAndThePause();
    void aTotalTimePaceRunsSingleBeatRepetitions();
    void aTimePerRepetitionPaceEndsByItself();
    void aRhythmRunsInsteadOfAStoredPace();
    void aRepetitionStepWithoutAPaceWaitsForDone();
    void aPacedStepPausesAndResumesLikeARhythm();
};

static Workout single(const QList<Step> &steps, int rounds = 1, int restBetweenRounds = 0)
{
    return workout(QList<Block>() << block(steps, rounds, restBetweenRounds));
}

static QStringList beatLines(const EngEvents &events)
{
    return describeAll(ofType(events, EventType::Beat));
}

void TstEngineTempo::timePerRepBeats()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", Tempo::timePerRep(1500, 3))), 3, QList<int>());
    r.take(r.engine->start());
    r.runTo(8000, 100);
    QCOMPARE(describeAll(r.log), QStringList()
        << "step@3000 i0" << "beat@3000 i0 r1 p0 a1" << "beat@4500 i0 r2 p0 a1" << "beat@6000 i0 r3 p0 a1"
        << "finished@7500 i0");
}

void TstEngineTempo::repsAndTotalBeatsDoNotDrift()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", Tempo::repsAndTotal(7, 20000)) << rest(5)), 0, QList<int>());
    r.take(r.engine->start());
    r.runTo(30000, 7);
    const EngEvents beats = ofType(r.log, EventType::Beat);
    QCOMPARE(beats.size(), 7);
    const qint64 expected[] = { 0, 2857, 5714, 8571, 11429, 14286, 17143 };
    for (int i = 0; i < 7; ++i) {
        QCOMPARE(beats.at(i).atMs, expected[i]);
        QCOMPARE(beats.at(i).repetition, i + 1);
    }
    // The step ends exactly at 20 s and the rest starts then.
    QCOMPARE(timesOf(r.log, EventType::StepStarted), QList<qint64>() << 0 << 20000);
    QCOMPARE(timesOf(r.log, EventType::Finished), QList<qint64>() << 25000);
}

void TstEngineTempo::threePhaseBeats()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", phasesOf({ 2, 3, 1 }, 2))), 0, QList<int>());
    r.take(r.engine->start());
    r.runTo(7000, 50);
    QCOMPARE(beatLines(r.log), QStringList()
        << "beat@0 i0 r1 p1 a1" << "beat@1000 i0 r1 p2 a0" << "beat@2500 i0 r1 p3 a0"
        << "beat@3000 i0 r2 p1 a1" << "beat@4000 i0 r2 p2 a0" << "beat@5500 i0 r2 p3 a0");
    QCOMPARE(timesOf(r.log, EventType::Finished), QList<qint64>() << 6000);
}

void TstEngineTempo::tempoStepRunsAsATimedStep()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", Tempo::timePerRep(2000, 3))), 0, QList<int>());
    r.take(r.engine->start());
    // It is running, not waiting for a repetition count.
    QVERIFY(r.is(SessionState::Running));
    r.at(3000);
    QVERIFY(r.is(SessionState::Running));
    r.at(6000);
    QVERIFY(r.is(SessionState::Finished));
}

void TstEngineTempo::noTicksInATempoStep()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", Tempo::timePerRep(4000, 3))), 0);
    r.take(r.engine->start());
    r.runTo(13000, 100);
    QCOMPARE(countOf(r.log, EventType::Tick), 0);
    QCOMPARE(countOf(r.log, EventType::Beat), 3);
}

void TstEngineTempo::sixtyMinutesWithoutDrift()
{
    EngineRig r;
    r.clock.set(777);
    r.make(single(QList<Step>() << tempoStep("a", Tempo::repsAndTotal(999, 3600000))), 0, QList<int>());
    r.clock.set(777);
    r.take(r.engine->start());
    r.runTo(777 + 3600000, 37);
    const EngEvents beats = ofType(r.log, EventType::Beat);
    QCOMPARE(beats.size(), 999);
    for (int i = 0; i < beats.size(); ++i) {
        const double exact = 777.0 + double(i) * 3600000.0 / 999.0;
        QVERIFY2(qAbs(double(beats.at(i).atMs) - exact) <= 0.5 + 1e-6, qPrintable(QString::number(i)));
    }
    QCOMPARE(timesOf(r.log, EventType::Finished), QList<qint64>() << qint64(777 + 3600000));
}

static QStringList fullRun(qint64 step)
{
    EngineRig r;
    r.make(single(QList<Step>() << work("a", 3) << tempoStep("b", Tempo::repsAndTotal(7, 20000))
                                << tempoStep("c", phasesOf({ 2, 3, 1 }, 3)) << rest(5), 2, 4), 2);
    r.take(r.engine->start());
    r.runTo(130000, step);
    return describeAll(r.log);
}

void TstEngineTempo::beatsDoNotDependOnThePollingStep()
{
    const QStringList reference = fullRun(1);
    QVERIFY(reference.size() > 60);
    QCOMPARE(fullRun(7), reference);
    QCOMPARE(fullRun(333), reference);
    QCOMPARE(fullRun(10000), reference);
    QCOMPARE(fullRun(1000000), reference);
}

void TstEngineTempo::doneEndsTheStepEarly()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", Tempo::timePerRep(1000, 10)) << work("b", 5)), 0, QList<int>());
    r.take(r.engine->start());
    r.clock.set(3200);
    r.take(r.engine->update());
    r.take(r.engine->done());
    QCOMPARE(r.engine->stepIndex(), 1);
    QVERIFY(r.is(SessionState::Running));
    QCOMPARE(r.log.last().type, EventType::StepStarted);
    QCOMPARE(r.log.last().atMs, qint64(3200));
    QCOMPARE(countOf(r.log, EventType::Beat), 4);   // 0, 1, 2 and 3 s
    r.at(20000);
    QCOMPARE(countOf(r.log, EventType::Beat), 4);   // no more beats of the ended step
    QCOMPARE(r.engine->records().first().activeMs, qint64(3200));
    const LogEntry entry = buildLogEntry("W", 0, r.engine->plan(), r.engine->records(), EndState::Completed,
                                         [](const QString &key, QString *name, MuscleSet *muscles) { *name = key; *muscles = 1; return true; });
    QCOMPARE(entry.steps.first().actualSec, 3);
    QCOMPARE(entry.steps.first().plannedReps, 10);
    QCOMPARE(entry.steps.first().plannedSec, 0);
}

void TstEngineTempo::doneIsIgnoredInTheLeadIn()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", Tempo::timePerRep(1000, 4))), 3, QList<int>());
    r.take(r.engine->start());
    r.clock.set(1000);
    QVERIFY(r.engine->done().isEmpty());
    QVERIFY(r.is(SessionState::LeadIn));
    r.at(3000);
    QVERIFY(r.is(SessionState::Running));
}

void TstEngineTempo::pauseRestartsTheRepetition()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", Tempo::timePerRep(4000, 3))), 0, QList<int>());
    r.take(r.engine->start());
    r.at(6500);                              // 2.5 s into repetition 2
    r.take(r.engine->pause());
    QVERIFY(r.is(SessionState::Paused));
    QCOMPARE(r.engine->stepRemainingMs(), qint64(8000));   // the repetition counts from its start
    r.clock.set(20000);
    QVERIFY(r.engine->update().isEmpty());
    r.take(r.engine->resume());
    QVERIFY(r.is(SessionState::Running));
    QCOMPARE(describeAll(r.log), QStringList()
        << "step@0 i0" << "beat@0 i0 r1 p0 a1" << "beat@4000 i0 r2 p0 a1" << "beat@20000 i0 r2 p0 a1");
    r.at(24000);
    r.at(28000);
    QCOMPARE(beatLines(r.log).last(), QStringLiteral("beat@24000 i0 r3 p0 a1"));
    QCOMPARE(timesOf(r.log, EventType::Finished), QList<qint64>() << 28000);
}

void TstEngineTempo::pauseWithALeadIn()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", Tempo::timePerRep(4000, 3))), 3, QList<int>());
    r.take(r.engine->start());
    r.at(3000);
    r.at(9500);                              // repetition 2, 2.5 s in
    r.take(r.engine->pause());
    r.clock.set(20000);
    r.take(r.engine->resume());
    QVERIFY(r.is(SessionState::LeadIn));
    r.at(23000);
    QVERIFY(r.is(SessionState::Running));
    QCOMPARE(beatLines(r.log).last(), QStringLiteral("beat@23000 i0 r2 p0 a1"));
    r.at(31000);
    QCOMPARE(timesOf(r.log, EventType::Finished), QList<qint64>() << 31000);
}

void TstEngineTempo::pauseInPhasesRestartsAtThePhaseOne()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", phasesOf({ 2, 2, 2, 2 }, 2))), 0, QList<int>());   // 4 s
    r.take(r.engine->start());
    r.at(2500);                              // third phase of repetition 1
    r.take(r.engine->pause());
    r.clock.set(10000);
    r.take(r.engine->resume());
    const QStringList beats = beatLines(r.log);
    QCOMPARE(beats.last(), QStringLiteral("beat@10000 i0 r1 p1 a1"));
    r.runTo(13000, 100);
    QCOMPARE(QStringList(beatLines(r.log).mid(beats.size())), QStringList()
        << "beat@11000 i0 r1 p2 a0" << "beat@12000 i0 r1 p3 a0" << "beat@13000 i0 r1 p4 a0");
}

void TstEngineTempo::pauseAtTheStartOfARepetition()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", Tempo::timePerRep(2000, 3))), 0, QList<int>());
    r.take(r.engine->start());
    r.clock.set(2000);
    r.take(r.engine->pause());               // the beat of repetition 2 is reported by the pause
    QCOMPARE(countOf(r.log, EventType::Beat), 2);
    r.clock.set(5000);
    r.take(r.engine->resume());
    QCOMPARE(beatLines(r.log).last(), QStringLiteral("beat@5000 i0 r2 p0 a1"));
    r.at(9000);
    QCOMPARE(timesOf(r.log, EventType::Finished), QList<qint64>() << 9000);
}

void TstEngineTempo::secondsInTheLogAreTheActiveTime()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", Tempo::timePerRep(1000, 4))), 0, QList<int>());
    r.take(r.engine->start());
    r.at(2500);
    r.take(r.engine->pause());
    r.clock.set(12500);
    r.take(r.engine->resume());
    r.at(14500);
    QVERIFY(r.is(SessionState::Finished));
    // 2.5 s before the pause and the 2 s that were left of the planned 4 s after repetition 3 restarted.
    QCOMPARE(r.engine->records().first().activeMs, qint64(4500));
    const LogEntry entry = buildLogEntry("W", 0, r.engine->plan(), r.engine->records(), EndState::Completed,
                                         [](const QString &key, QString *name, MuscleSet *muscles) { *name = key; *muscles = 1; return true; });
    QCOMPARE(entry.steps.first().actualSec, 5);
    QCOMPARE(entry.totalSec, 5);

    // A step that ran its whole time without a pause: the planned seconds.
    EngineRig q;
    q.make(single(QList<Step>() << tempoStep("a", Tempo::timePerRep(1500, 4))), 3, QList<int>());
    q.take(q.engine->start());
    q.at(3000);
    q.at(9000);
    QCOMPARE(q.engine->records().first().activeMs, qint64(6000));
}

void TstEngineTempo::stepEndsExactlyAndTheRestFollows()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", Tempo::timePerRep(1500, 3)) << rest(5) << work("b", 10)), 0, QList<int>());
    r.take(r.engine->start());
    r.runTo(20000, 13);
    QCOMPARE(timesOf(r.log, EventType::StepStarted), QList<qint64>() << 0 << 4500 << 9500);
    QCOMPARE(timesOf(r.log, EventType::Finished), QList<qint64>() << 19500);
}

void TstEngineTempo::startOnTapAndTempo()
{
    EngineRig r;
    r.make(single(QList<Step>() << work("x", 1) << onTap(tempoStep("a", Tempo::timePerRep(1000, 2)))), 2, QList<int>());
    r.take(r.engine->start());
    r.at(2000);
    r.at(3000);
    QVERIFY(r.is(SessionState::WaitingToStart));
    r.at(10000);
    r.take(r.engine->start());
    r.at(12000);
    r.at(14000);
    QCOMPARE(describeAll(r.log), QStringList()
        << "step@2000 i0" << "wait@3000 i1" << "step@12000 i1" << "beat@12000 i1 r1 p0 a1"
        << "beat@13000 i1 r2 p0 a1" << "finished@14000 i1");
}

void TstEngineTempo::nextEndsATempoStep()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", Tempo::timePerRep(1000, 10)) << work("b", 5)), 0, QList<int>());
    r.take(r.engine->start());
    r.clock.set(2200);
    r.take(r.engine->next());
    QCOMPARE(r.engine->stepIndex(), 1);
    r.at(30000);
    QCOMPARE(countOf(r.log, EventType::Beat), 3);
    QCOMPARE(r.engine->records().first().activeMs, qint64(2200));
}

void TstEngineTempo::remainingTimeOfTheStep()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", Tempo::timePerRep(1500, 4))), 3, QList<int>());
    r.take(r.engine->start());
    QCOMPARE(r.engine->stepRemainingMs(), qint64(6000));   // in the lead-in the whole step is ahead
    r.at(3000);
    r.clock.set(4000);
    QCOMPARE(r.engine->stepRemainingMs(), qint64(5000));
}

void TstEngineTempo::roundsRepeatTheTempo()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", Tempo::timePerRep(1000, 2)), 2, 3), 0, QList<int>());
    r.take(r.engine->start());
    r.runTo(8000, 50);
    QCOMPARE(beatLines(r.log), QStringList()
        << "beat@0 i0 r1 p0 a1" << "beat@1000 i0 r2 p0 a1" << "beat@5000 i2 r1 p0 a1" << "beat@6000 i2 r2 p0 a1");
    QCOMPARE(timesOf(r.log, EventType::Finished), QList<qint64>() << 7000);
}

void TstEngineTempo::repetitionStepWithoutTempoStillWaitsForDone()
{
    EngineRig r;
    r.make(single(QList<Step>() << repStep("a", 10) << work("b", 5)), 0, QList<int>());
    r.take(r.engine->start());
    QVERIFY(r.is(SessionState::WaitingForReps));
    r.at(60000);
    QVERIFY(r.is(SessionState::WaitingForReps));
    QCOMPARE(countOf(r.log, EventType::Beat), 0);
    r.take(r.engine->done());
    QCOMPARE(r.engine->stepIndex(), 1);
}

void TstEngineTempo::plannedRemainingCountsTempoSteps()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", Tempo::timePerRep(1500, 4)) << repStep("b", 9) << work("c", 10)), 0, QList<int>());
    bool open = false;
    QCOMPARE(r.engine->plannedRemainingMs(&open), qint64(16000));
    QVERIFY(open);
    r.take(r.engine->start());
    r.at(2000);
    QCOMPARE(r.engine->plannedRemainingMs(&open), qint64(4000 + 10000));
    QVERIFY(open);
    r.take(r.engine->pause());
    QCOMPARE(r.engine->plannedRemainingMs(&open), qint64(4500 + 10000));   // repetition 2 restarts
}

void TstEngineTempo::stepRunsItsOwnRepetitionsAtTheTempoOfItsExercise()
{
    // The exercise's metronome has 2 repetitions, 5 sets and a rest; none of them is used. The step
    // says 3 repetitions: the three-phase pace of 4 s is run 3 times and the step ends by itself.
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", phasesOf({ 4, 2, 2 }, 2), 3) << rest(5)), 0, QList<int>());
    r.take(r.engine->start());
    QVERIFY(r.is(SessionState::Running));
    r.runTo(20000, 50);
    QCOMPARE(beatLines(r.log), QStringList()
        << "beat@0 i0 r1 p1 a1" << "beat@2000 i0 r1 p2 a0" << "beat@3000 i0 r1 p3 a0"
        << "beat@4000 i0 r2 p1 a1" << "beat@6000 i0 r2 p2 a0" << "beat@7000 i0 r2 p3 a0"
        << "beat@8000 i0 r3 p1 a1" << "beat@10000 i0 r3 p2 a0" << "beat@11000 i0 r3 p3 a0");
    // The rest follows at 12 s without a tap on "done", and the session ends at 17 s.
    QCOMPARE(timesOf(r.log, EventType::StepStarted), QList<qint64>() << 0 << 12000);
    QCOMPARE(timesOf(r.log, EventType::Finished), QList<qint64>() << 17000);
}

void TstEngineTempo::aTimedStepOfTheSameExerciseIgnoresTheTempo()
{
    EngineRig r;
    tempoSnapshot().insert("a", Tempo::timePerRep(1000, 4));
    r.make(single(QList<Step>() << work("a", 5)), 0, QList<int>());
    r.take(r.engine->start());
    r.runTo(10000, 100);
    QCOMPARE(countOf(r.log, EventType::Beat), 0);
    QCOMPARE(timesOf(r.log, EventType::Finished), QList<qint64>() << 5000);
}

void TstEngineTempo::exerciseWithoutAMetronomeWaitsForDone()
{
    EngineRig r;
    r.make(single(QList<Step>() << repStep("lonely", 12) << rest(5)), 0, QList<int>());
    r.take(r.engine->start());
    // A plain repetition step with its own repetitions: no beats, it waits for the user.
    QVERIFY(r.is(SessionState::WaitingForReps));
    r.at(60000);
    QVERIFY(r.is(SessionState::WaitingForReps));
    QCOMPARE(countOf(r.log, EventType::Beat), 0);
    QCOMPARE(r.engine->currentStep().reps, 12);
    r.take(r.engine->done());
    QVERIFY(r.is(SessionState::Running));
}

void TstEngineTempo::tempoElapsedFollowsTheSetAndThePause()
{
    EngineRig r;
    r.make(single(QList<Step>() << tempoStep("a", Tempo::timePerRep(1500, 4))), 0, QList<int>());
    QCOMPARE(r.engine->tempoElapsedMs(), qint64(0));
    r.take(r.engine->start());
    r.at(2000);
    QCOMPARE(r.engine->tempoElapsedMs(), qint64(2000));
    // A pause keeps the start of the repetition that is run again.
    r.take(r.engine->pause());
    QCOMPARE(r.engine->tempoElapsedMs(), qint64(1500));
    r.at(9000);
    QCOMPARE(r.engine->tempoElapsedMs(), qint64(1500));
    r.take(r.engine->resume());
    QCOMPARE(r.engine->tempoElapsedMs(), qint64(1500));
    r.at(9500);
    QCOMPARE(r.engine->tempoElapsedMs(), qint64(2000));
    // Never beyond the set.
    r.at(30000);
    QVERIFY(r.engine->tempoElapsedMs() <= 6000);
}

void TstEngineTempo::aTotalTimePaceRunsSingleBeatRepetitions()
{
    EngineRig r;
    // 5 repetitions in 10 s: one beat every 2 s, only the first kind of beat, and the step ends at 10 s.
    r.make(single(QList<Step>() << pacedStep("a", 5, PaceKind::Total, 10000) << rest(5)), 0, QList<int>());
    r.take(r.engine->start());
    QVERIFY(r.engine->currentStep().hasTempo);
    r.runTo(16000, 50);
    QCOMPARE(describeAll(ofType(r.log, EventType::Beat)), QStringList()
        << "beat@0 i0 r1 p0 a1" << "beat@2000 i0 r2 p0 a1" << "beat@4000 i0 r3 p0 a1"
        << "beat@6000 i0 r4 p0 a1" << "beat@8000 i0 r5 p0 a1");
    QCOMPARE(timesOf(r.log, EventType::Finished), QList<qint64>() << 15000);
}

void TstEngineTempo::aTimePerRepetitionPaceEndsByItself()
{
    EngineRig r;
    r.make(single(QList<Step>() << pacedStep("a", 3, PaceKind::PerRep, 1500)), 3, QList<int>());
    r.take(r.engine->start());
    r.runTo(9000, 100);
    QCOMPARE(describeAll(r.log), QStringList()
        << "step@3000 i0" << "beat@3000 i0 r1 p0 a1" << "beat@4500 i0 r2 p0 a1" << "beat@6000 i0 r3 p0 a1"
        << "finished@7500 i0");
}

void TstEngineTempo::aRhythmRunsInsteadOfAStoredPace()
{
    EngineRig r;
    tempoSnapshot().insert("a", phasesOf({ 2, 4 }, 1));
    // 2 repetitions of 3 s with 2 phases each; the stored pace of 1 s per repetition is ignored.
    r.make(single(QList<Step>() << pacedStep("a", 2, PaceKind::PerRep, 1000)), 0, QList<int>());
    r.take(r.engine->start());
    r.runTo(7000, 50);
    QCOMPARE(countOf(r.log, EventType::Beat), 4);
    QCOMPARE(describeAll(ofType(r.log, EventType::Beat)).first(), QStringLiteral("beat@0 i0 r1 p1 a1"));
    QCOMPARE(timesOf(r.log, EventType::Finished), QList<qint64>() << 6000);
}

void TstEngineTempo::aRepetitionStepWithoutAPaceWaitsForDone()
{
    EngineRig r;
    r.make(single(QList<Step>() << repStep("a", 12)), 0, QList<int>());
    r.take(r.engine->start());
    QVERIFY(r.is(SessionState::WaitingForReps));
    r.at(60000);
    QVERIFY(r.is(SessionState::WaitingForReps));
    QCOMPARE(countOf(r.log, EventType::Beat), 0);
}

void TstEngineTempo::aPacedStepPausesAndResumesLikeARhythm()
{
    EngineRig r;
    r.make(single(QList<Step>() << pacedStep("a", 4, PaceKind::PerRep, 2000)), 0, QList<int>());
    r.take(r.engine->start());
    r.at(3000);
    // The repetition 2 is running (2 s to 4 s); the pause drops its elapsed part.
    r.take(r.engine->pause());
    QVERIFY(r.is(SessionState::Paused));
    r.at(10000);
    r.take(r.engine->resume());
    QVERIFY(r.is(SessionState::Running));
    r.runTo(20000, 50);
    // Repetitions 1 and 2 began before the pause at 0 and 2 s; after the resume the repetition 2
    // starts again, then 3 and 4, and the step ends after the last one.
    QCOMPARE(countOf(r.log, EventType::Beat), 5);
    QCOMPARE(countOf(r.log, EventType::Finished), 1);
}

QTEST_APPLESS_MAIN(TstEngineTempo)
#include "tst_enginetempo.moc"

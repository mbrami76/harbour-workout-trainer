// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "plan.h"
#include "testsupport.h"

class TstPlan : public QObject
{
    Q_OBJECT

private slots:
    void init() { tempoSnapshot().clear(); }
    void emptyWorkout();
    void singleBlockRoundsWithoutRest();
    void restBetweenRoundsOnly();
    void noTrailingRest();
    void restBetweenBlocks();
    void zeroRestInsertsNothing();
    void positionsAndCounts();
    void insertedRestFields();
    void stepFieldsAreCopied();
    void startModeAndTempoAreCopiedToEveryRound();
    void plannedLengthOfEachKind();
    void tempoStepKeepsItsRepsAtThePaceOfOneRepetition();
    void everyFormGivesThePaceOfOneRepetition();
    void onlyRepetitionStepsOfAnExerciseWithATempoArePaced();
    void warmupAndCooldownRepetitionStepsArePaced();
    void anInvalidEntryLeavesAPlainRepetitionStep();
    void insertedRestsStartAutomatically();
    void aTimedPaceGivesAPacedStep();
    void aRhythmWinsOverAStoredPaceInThePlan();
    void plannedTotalCountsPacedStepsAndLeavesOutOpenOnes();
};

static QString layout(const QList<PlanStep> &plan)
{
    QString text;
    for (const PlanStep &s : plan) {
        if (s.insertedRest)
            text += QStringLiteral("R%1 ").arg(s.durationSec);
        else if (s.type == StepType::Rest)
            text += QStringLiteral("r%1 ").arg(s.durationSec);
        else
            text += s.exerciseKey + QLatin1Char(' ');
    }
    return text.trimmed();
}

void TstPlan::emptyWorkout()
{
    QVERIFY(expandPlan(Workout()).isEmpty());
}

void TstPlan::singleBlockRoundsWithoutRest()
{
    const Workout w = workout(QList<Block>() << block(QList<Step>() << work("a", 5) << work("b", 5), 3));
    QCOMPARE(layout(expandPlan(w)), QStringLiteral("a b a b a b"));
}

void TstPlan::restBetweenRoundsOnly()
{
    const Workout w = workout(QList<Block>() << block(QList<Step>() << work("a", 5) << work("b", 5), 3, 8));
    QCOMPARE(layout(expandPlan(w)), QStringLiteral("a b R8 a b R8 a b"));
}

void TstPlan::noTrailingRest()
{
    const Workout w = workout(QList<Block>() << block(QList<Step>() << work("a", 5), 2, 8) << block(QList<Step>() << work("b", 5)), 6);
    const QList<PlanStep> plan = expandPlan(w);
    QVERIFY(!plan.last().insertedRest);
    QCOMPARE(layout(plan), QStringLiteral("a R8 a R6 b"));

    const Workout single = workout(QList<Block>() << block(QList<Step>() << work("a", 5), 1, 8), 6);
    QCOMPARE(layout(expandPlan(single)), QStringLiteral("a"));
}

void TstPlan::restBetweenBlocks()
{
    const Workout w = workout(QList<Block>()
        << block(QList<Step>() << work("a", 5))
        << block(QList<Step>() << work("b", 5))
        << block(QList<Step>() << work("c", 5)), 9);
    QCOMPARE(layout(expandPlan(w)), QStringLiteral("a R9 b R9 c"));
}

void TstPlan::zeroRestInsertsNothing()
{
    const Workout w = workout(QList<Block>()
        << block(QList<Step>() << work("a", 5), 2, 0)
        << block(QList<Step>() << work("b", 5)), 0);
    QCOMPARE(layout(expandPlan(w)), QStringLiteral("a a b"));
}

void TstPlan::positionsAndCounts()
{
    const Workout w = workout(QList<Block>()
        << block(QList<Step>() << work("a", 5) << rest(3), 2, 4)
        << block(QList<Step>() << work("b", 5)), 7);
    const QList<PlanStep> plan = expandPlan(w);
    // a r3 R4 a r3 R7 b
    QCOMPARE(plan.size(), 7);

    QCOMPARE(plan.at(0).blockIndex, 0);
    QCOMPARE(plan.at(0).blockCount, 2);
    QCOMPARE(plan.at(0).roundIndex, 0);
    QCOMPARE(plan.at(0).roundCount, 2);
    QCOMPARE(plan.at(0).stepIndexInBlock, 0);
    QCOMPARE(plan.at(0).stepCountInBlock, 2);

    QCOMPARE(plan.at(1).stepIndexInBlock, 1);
    QVERIFY(!plan.at(1).insertedRest);

    QCOMPARE(plan.at(3).roundIndex, 1);
    QCOMPARE(plan.at(3).stepIndexInBlock, 0);
    QCOMPARE(plan.at(4).stepIndexInBlock, 1);

    QCOMPARE(plan.at(6).blockIndex, 1);
    QCOMPARE(plan.at(6).roundIndex, 0);
    QCOMPARE(plan.at(6).roundCount, 1);
    QCOMPARE(plan.at(6).stepCountInBlock, 1);
}

void TstPlan::insertedRestFields()
{
    const Workout w = workout(QList<Block>()
        << block(QList<Step>() << work("a", 5), 2, 4)
        << block(QList<Step>() << work("b", 5)), 7);
    const QList<PlanStep> plan = expandPlan(w);
    // a R4 a R7 b
    const PlanStep &betweenRounds = plan.at(1);
    QVERIFY(betweenRounds.insertedRest);
    QVERIFY(betweenRounds.type == StepType::Rest);
    QVERIFY(betweenRounds.exerciseKey.isEmpty());
    QCOMPARE(betweenRounds.durationSec, 4);
    QCOMPARE(betweenRounds.reps, 0);
    QCOMPARE(betweenRounds.blockIndex, 0);
    QCOMPARE(betweenRounds.roundIndex, 0);
    QCOMPARE(betweenRounds.stepIndexInBlock, -1);

    const PlanStep &betweenBlocks = plan.at(3);
    QVERIFY(betweenBlocks.insertedRest);
    QCOMPARE(betweenBlocks.durationSec, 7);
    QCOMPARE(betweenBlocks.blockIndex, 0);
    QCOMPARE(betweenBlocks.roundIndex, 1);
}

void TstPlan::stepFieldsAreCopied()
{
    const Workout w = workout(QList<Block>() << block(QList<Step>()
        << timedStep(StepType::Warmup, "w", 30) << repStep("p", 12) << rest(10)
        << timedStep(StepType::Cooldown, "c", 60)));
    const QList<PlanStep> plan = expandPlan(w);
    QCOMPARE(plan.size(), 4);
    QVERIFY(plan.at(0).type == StepType::Warmup);
    QCOMPARE(plan.at(0).exerciseKey, QStringLiteral("w"));
    QCOMPARE(plan.at(0).durationSec, 30);
    QVERIFY(plan.at(0).isTimed());
    QCOMPARE(plan.at(1).reps, 12);
    QVERIFY(!plan.at(1).isTimed());
    QVERIFY(plan.at(2).type == StepType::Rest);
    QVERIFY(!plan.at(2).insertedRest);
    QVERIFY(plan.at(3).type == StepType::Cooldown);
}

void TstPlan::startModeAndTempoAreCopiedToEveryRound()
{
    const Tempo t = Tempo::phases(QList<int>() << 2 << 3 << 1, 5);
    const Workout w = workout(QList<Block>() << block(QList<Step>() << onTap(work("a", 30)) << tempoStep("b", t) << repStep("c", 9), 3, 7));
    const QList<PlanStep> plan = expandPlan(w, tempoSnapshot());
    int checked = 0;
    for (const PlanStep &s : plan) {
        if (s.insertedRest)
            continue;
        if (s.exerciseKey == "a") {
            QVERIFY(s.startOnTap);
            QVERIFY(!s.hasTempo);
        } else if (s.exerciseKey == "b") {
            QVERIFY(!s.startOnTap);
            QVERIFY(s.hasTempo);
            QVERIFY(s.tempo == t);
            QCOMPARE(s.reps, 5);
            QVERIFY(!s.isTimed());
        } else {
            QVERIFY(!s.startOnTap && !s.hasTempo);
        }
        ++checked;
    }
    QCOMPARE(checked, 9);
}

void TstPlan::plannedLengthOfEachKind()
{
    const QList<PlanStep> plan = expandPlan(workout(QList<Block>() << block(QList<Step>()
        << work("a", 30) << tempoStep("b", Tempo::repsAndTotal(7, 20000)) << repStep("c", 9))), tempoSnapshot());
    QCOMPARE(plan.at(0).plannedMs(), qint64(30000));
    QCOMPARE(plan.at(1).plannedMs(), qint64(20000));
    QCOMPARE(plan.at(2).plannedMs(), qint64(0));
}

void TstPlan::tempoStepKeepsItsRepsAtThePaceOfOneRepetition()
{
    // The step has its own 9 repetitions; the metronome has 4 repetitions of 1.5 s.
    const QList<PlanStep> plan = expandPlan(workout(QList<Block>() << block(QList<Step>()
        << tempoStep("a", Tempo::timePerRep(1500, 4), 9), 2)), tempoSnapshot());
    QCOMPARE(plan.size(), 2);
    for (const PlanStep &s : plan) {
        QVERIFY(s.hasTempo);
        QCOMPARE(s.reps, 9);
        QCOMPARE(s.tempo.reps(), 9);
        QCOMPARE(s.tempo.repDurationMs(0), qint64(1500));
        QCOMPARE(s.plannedMs(), qint64(13500));
    }
}

void TstPlan::everyFormGivesThePaceOfOneRepetition()
{
    // Form a: 7 repetitions in 20 s give 3 repetitions in 8571 ms; form c keeps phases and names.
    ExerciseTempos tempos;
    tempos.insert("a", Tempo::repsAndTotal(7, 20000));
    tempos.insert("b", Tempo::phases(QList<int>() << 2 << 3 << 1, 8, QStringList() << "up" << "" << "down"));
    const QList<PlanStep> plan = expandPlan(workout(QList<Block>() << block(QList<Step>()
        << repStep("a", 3) << repStep("b", 5))), tempos);
    QCOMPARE(plan.at(0).tempo.form(), TempoForm::RepsAndTotal);
    QCOMPARE(plan.at(0).tempo.reps(), 3);
    QCOMPARE(plan.at(0).tempo.valueMs(), qint64(8571));
    QCOMPARE(plan.at(0).plannedMs(), qint64(8571));
    QCOMPARE(plan.at(1).tempo.form(), TempoForm::Phases);
    QCOMPARE(plan.at(1).tempo.reps(), 5);
    QCOMPARE(plan.at(1).tempo.phaseName(1), QStringLiteral("up"));
    QCOMPARE(plan.at(1).tempo.phaseName(3), QStringLiteral("down"));
    QCOMPARE(plan.at(1).plannedMs(), qint64(5 * 3000));
}

void TstPlan::onlyRepetitionStepsOfAnExerciseWithATempoArePaced()
{
    ExerciseTempos tempos;
    tempos.insert("a", Tempo::timePerRep(1000, 4));
    // A duration step, a rest and a repetition step of an exercise without a tempo stay plain.
    const QList<PlanStep> plan = expandPlan(workout(QList<Block>() << block(QList<Step>()
        << work("a", 30) << rest(10) << repStep("b", 6) << timedStep(StepType::Warmup, "a", 20)
        << timedStep(StepType::Cooldown, "a", 20) << repStep("a", 6))), tempos);
    QCOMPARE(plan.size(), 6);
    for (int i = 0; i < 5; ++i) {
        QVERIFY(!plan.at(i).hasTempo);
        QCOMPARE(plan.at(i).plannedMs(), plan.at(i).isTimed() ? qint64(plan.at(i).durationSec) * 1000 : qint64(0));
    }
    QVERIFY(plan.at(5).hasTempo);
    QCOMPARE(plan.at(5).plannedMs(), qint64(6000));
}

void TstPlan::warmupAndCooldownRepetitionStepsArePaced()
{
    ExerciseTempos tempos;
    tempos.insert("a", Tempo::timePerRep(1000, 4));
    Step warm = repStep("a", 3);
    warm.type = StepType::Warmup;
    Step cool = repStep("a", 2);
    cool.type = StepType::Cooldown;
    const QList<PlanStep> plan = expandPlan(workout(QList<Block>() << block(QList<Step>() << warm << cool)), tempos);
    QVERIFY(plan.at(0).hasTempo && plan.at(1).hasTempo);
    QCOMPARE(plan.at(0).tempo.reps(), 3);
    QCOMPARE(plan.at(1).tempo.reps(), 2);
}

void TstPlan::anInvalidEntryLeavesAPlainRepetitionStep()
{
    ExerciseTempos tempos;
    tempos.insert("a", Tempo());   // an entry that is not a valid pace counts as missing
    const QList<PlanStep> plan = expandPlan(workout(QList<Block>() << block(QList<Step>() << repStep("a", 12) << repStep("b", 8), 3)), tempos);
    QCOMPARE(plan.size(), 6);
    for (const PlanStep &s : plan) {
        QVERIFY(!s.hasTempo);
        QVERIFY(!s.isTimed());
        QCOMPARE(s.plannedMs(), qint64(0));
    }
    QCOMPARE(plan.at(0).reps, 12);
    QCOMPARE(plan.at(1).reps, 8);
}

void TstPlan::insertedRestsStartAutomatically()
{
    const QList<PlanStep> plan = expandPlan(workout(QList<Block>() << block(QList<Step>() << onTap(work("a", 30)), 2, 7)));
    QCOMPARE(plan.size(), 3);
    QVERIFY(plan.at(1).insertedRest);
    QVERIFY(!plan.at(1).startOnTap);
    QVERIFY(!plan.at(1).hasTempo);
}

void TstPlan::aTimedPaceGivesAPacedStep()
{
    const QList<PlanStep> plan = expandPlan(workout(QList<Block>() << block(QList<Step>()
        << pacedStep("a", 10, PaceKind::Total, 30000) << pacedStep("a", 6, PaceKind::PerRep, 2500) << repStep("a", 4), 2)));
    QCOMPARE(plan.size(), 6);
    // Repetitions in a total time: one beat per repetition, 3 s each.
    QVERIFY(plan.at(0).hasTempo);
    QVERIFY(plan.at(0).pace == PaceKind::Total);
    QCOMPARE(plan.at(0).tempo.reps(), 10);
    QCOMPARE(plan.at(0).tempo.beatsPerRep(), 1);
    QCOMPARE(plan.at(0).tempo.repDurationMs(0), qint64(3000));
    QCOMPARE(plan.at(0).plannedMs(), qint64(30000));
    // A time per repetition.
    QVERIFY(plan.at(1).hasTempo);
    QVERIFY(plan.at(1).pace == PaceKind::PerRep);
    QCOMPARE(plan.at(1).plannedMs(), qint64(15000));
    // A repetition step with no pace ends on "done".
    QVERIFY(!plan.at(2).hasTempo);
    QVERIFY(plan.at(2).pace == PaceKind::None);
    QCOMPARE(plan.at(2).plannedMs(), qint64(0));
    // The second round has the same paces.
    QVERIFY(plan.at(3).hasTempo);
    QCOMPARE(plan.at(3).plannedMs(), qint64(30000));
}

void TstPlan::aRhythmWinsOverAStoredPaceInThePlan()
{
    ExerciseTempos rhythms;
    rhythms.insert("a", phasesOf({ 2, 4 }, 1));
    const QList<PlanStep> plan = expandPlan(workout(QList<Block>() << block(QList<Step>()
        << pacedStep("a", 5, PaceKind::PerRep, 1000) << pacedStep("b", 5, PaceKind::PerRep, 1000))), rhythms);
    QCOMPARE(plan.size(), 2);
    // The exercise with a rhythm runs its phases: the stored pace is ignored.
    QVERIFY(plan.at(0).hasTempo);
    QVERIFY(plan.at(0).pace == PaceKind::None);
    QCOMPARE(plan.at(0).tempo.phaseCount(), 2);
    QCOMPARE(plan.at(0).plannedMs(), qint64(15000));
    // The other exercise has none: its own pace is used.
    QVERIFY(plan.at(1).hasTempo);
    QVERIFY(plan.at(1).pace == PaceKind::PerRep);
    QCOMPARE(plan.at(1).tempo.phaseCount(), 0);
    QCOMPARE(plan.at(1).plannedMs(), qint64(5000));
}

void TstPlan::plannedTotalCountsPacedStepsAndLeavesOutOpenOnes()
{
    ExerciseTempos rhythms;
    rhythms.insert("r", phasesOf({ 2, 4 }, 1));
    // 20 s timed, 10 repetitions in 30 s, 4 repetitions of the rhythm (3 s each), and one step that ends on a tap.
    const Workout w = workout(QList<Block>() << block(QList<Step>()
        << work("a", 20) << pacedStep("a", 10, PaceKind::Total, 30000) << repStep("r", 4) << repStep("a", 12)));
    const PlannedTotal total = totalPlannedSeconds(w, rhythms);
    QCOMPARE(total.seconds, 20 + 30 + 12);
    QCOMPARE(total.secondsTimedOnly, 20);
    QVERIFY(total.hasRepSteps);
    QVERIFY(total.hasTempoSteps);
    // Without the open step the total has nothing left out.
    const Workout closed = workout(QList<Block>() << block(QList<Step>() << pacedStep("a", 10, PaceKind::PerRep, 1500)));
    const PlannedTotal t2 = totalPlannedSeconds(closed);
    QCOMPARE(t2.seconds, 15);
    QVERIFY(!t2.hasRepSteps);
    QVERIFY(t2.hasTempoSteps);
}

QTEST_APPLESS_MAIN(TstPlan)

#include "tst_plan.moc"

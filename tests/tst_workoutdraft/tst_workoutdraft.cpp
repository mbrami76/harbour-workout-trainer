// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "plan.h"
#include "workoutdraft.h"
#include "workoutlimits.h"

namespace {

Step step(const QString &key, int sec = 30, bool onTap = false)
{
    Step s;
    s.exerciseKey = key;
    s.durationSec = sec;
    s.startOnTap = onTap;
    return s;
}

QStringList keys(const Block &b)
{
    QStringList list;
    for (const Step &s : b.steps)
        list << s.exerciseKey;
    return list;
}

// Block 0: a b c; block 1: d.
Workout sample()
{
    Workout w;
    Block first;
    first.steps << step("a") << step("b") << step("c");
    Block second;
    second.steps << step("d");
    w.blocks << first << second;
    return w;
}

}

class TstWorkoutDraft : public QObject
{
    Q_OBJECT

private slots:
    void addStepAppendsAWorkStepWithTheDefaultDuration();
    void addBlockAppendsAnEmptyBlock();
    void unknownPlacesChangeNothing();
    void removeStepAndBlock();
    void duplicateStepSitsRightAfterTheOriginal();
    void duplicateBlockCopiesTheSteps();
    void moveStepStopsAtTheEnds();
    void moveBlockKeepsTheSteps();
    void moveStepToBlockKeepsTheStepWhole();
    void movingToTheSameBlockChangesNothing();
    void theFirstStepNeverWaitsForATap();
    void theFlagReturnsNotWhenAStepLeavesTheFirstPlace();
    void restTypeClearsTheExerciseAndIsTimed();
    void otherTypesKeepTheirValues();
    void switchingTheEndingSetsTheDefaultOfTheNewOne();
    void aRestStaysTimed();
    void aStepMovesAcrossTheEndsOfItsBlock();
    void aStepStopsAtTheFirstAndTheLastPlaceOfTheWorkout();
    void movingNeverChangesThePlannedTotal();
    void aDuplicatedBlockIsIndependentOfTheOriginal();
    void duplicatingAndMovingKeepTheWorkoutValid();
    void theFirstStepStaysAutomaticAfterEveryMove();
    void aPaceStartsAtItsDefaultAndStaysInsideTheLimits();
    void changingTheRepetitionsKeepsATotalTimeValid();
    void leavingTheRepetitionsDropsThePace();
};

void TstWorkoutDraft::addStepAppendsAWorkStepWithTheDefaultDuration()
{
    Workout w = sample();
    const WorkoutDraft::Position p = WorkoutDraft::addStep(w, 1);
    QCOMPARE(p.block, 1);
    QCOMPARE(p.step, 1);
    const Step &s = w.blocks.at(1).steps.at(1);
    QCOMPARE(s.type, StepType::Work);
    QVERIFY(s.exerciseKey.isEmpty());
    QCOMPARE(s.durationSec, WorkoutDraft::defaultDurationSec);
    QCOMPARE(s.reps, 0);
}

void TstWorkoutDraft::addBlockAppendsAnEmptyBlock()
{
    Workout w = sample();
    const WorkoutDraft::Position p = WorkoutDraft::addBlock(w);
    QCOMPARE(p.block, 2);
    QCOMPARE(w.blocks.at(2).rounds, 1);
    QVERIFY(w.blocks.at(2).steps.isEmpty());
}

void TstWorkoutDraft::unknownPlacesChangeNothing()
{
    Workout w = sample();
    QVERIFY(!WorkoutDraft::addStep(w, 5).isValid());
    QVERIFY(!WorkoutDraft::removeStep(w, 0, 3));
    QVERIFY(!WorkoutDraft::removeBlock(w, -1));
    QVERIFY(!WorkoutDraft::duplicateStep(w, 2, 0).isValid());
    QVERIFY(!WorkoutDraft::duplicateBlock(w, 9).isValid());
    QVERIFY(!WorkoutDraft::moveStep(w, 0, 7, 1).isValid());
    QVERIFY(!WorkoutDraft::moveBlock(w, 4, 1).isValid());
    QVERIFY(!WorkoutDraft::moveStepToBlock(w, 0, 0, 2).isValid());
    QVERIFY(!WorkoutDraft::moveStepToBlock(w, 0, 5, 1).isValid());
    QCOMPARE(keys(w.blocks.at(0)), QStringList() << "a" << "b" << "c");
    QCOMPARE(w.blocks.size(), 2);
}

void TstWorkoutDraft::removeStepAndBlock()
{
    Workout w = sample();
    QVERIFY(WorkoutDraft::removeStep(w, 0, 1));
    QCOMPARE(keys(w.blocks.at(0)), QStringList() << "a" << "c");
    QVERIFY(WorkoutDraft::removeBlock(w, 0));
    QCOMPARE(w.blocks.size(), 1);
    QCOMPARE(keys(w.blocks.at(0)), QStringList() << "d");
}

void TstWorkoutDraft::duplicateStepSitsRightAfterTheOriginal()
{
    Workout w = sample();
    w.blocks[0].steps[1].reps = 0;
    const WorkoutDraft::Position p = WorkoutDraft::duplicateStep(w, 0, 1);
    QCOMPARE(p.step, 2);
    QCOMPARE(keys(w.blocks.at(0)), QStringList() << "a" << "b" << "b" << "c");
}

void TstWorkoutDraft::duplicateBlockCopiesTheSteps()
{
    Workout w = sample();
    w.blocks[0].rounds = 4;
    w.blocks[0].restBetweenRoundsSec = 15;
    const WorkoutDraft::Position p = WorkoutDraft::duplicateBlock(w, 0);
    QCOMPARE(p.block, 1);
    QCOMPARE(w.blocks.size(), 3);
    QCOMPARE(w.blocks.at(1).rounds, 4);
    QCOMPARE(w.blocks.at(1).restBetweenRoundsSec, 15);
    QCOMPARE(keys(w.blocks.at(1)), QStringList() << "a" << "b" << "c");
    QCOMPARE(keys(w.blocks.at(2)), QStringList() << "d");
}

void TstWorkoutDraft::moveStepStopsAtTheEnds()
{
    Workout w = sample();
    WorkoutDraft::Position p = WorkoutDraft::moveStep(w, 0, 1, 1);
    QCOMPARE(p.step, 2);
    QCOMPARE(keys(w.blocks.at(0)), QStringList() << "a" << "c" << "b");
    p = WorkoutDraft::moveStep(w, 0, 2, 1);
    QCOMPARE(p.step, 2);
    QCOMPARE(keys(w.blocks.at(0)), QStringList() << "a" << "c" << "b");
    p = WorkoutDraft::moveStep(w, 0, 2, -5);
    QCOMPARE(p.step, 0);
    QCOMPARE(keys(w.blocks.at(0)), QStringList() << "b" << "a" << "c");
}

void TstWorkoutDraft::moveBlockKeepsTheSteps()
{
    Workout w = sample();
    const WorkoutDraft::Position p = WorkoutDraft::moveBlock(w, 1, -1);
    QCOMPARE(p.block, 0);
    QCOMPARE(keys(w.blocks.at(0)), QStringList() << "d");
    QCOMPARE(keys(w.blocks.at(1)), QStringList() << "a" << "b" << "c");
    QCOMPARE(WorkoutDraft::moveBlock(w, 0, -1).block, 0);
}

void TstWorkoutDraft::moveStepToBlockKeepsTheStepWhole()
{
    Workout w = sample();
    w.blocks[0].steps[1].type = StepType::Cooldown;
    w.blocks[0].steps[1].startOnTap = true;
    const WorkoutDraft::Position p = WorkoutDraft::moveStepToBlock(w, 0, 1, 1);
    QCOMPARE(p.block, 1);
    QCOMPARE(p.step, 1);
    QCOMPARE(keys(w.blocks.at(0)), QStringList() << "a" << "c");
    QCOMPARE(keys(w.blocks.at(1)), QStringList() << "d" << "b");
    QCOMPARE(w.blocks.at(1).steps.at(1).type, StepType::Cooldown);
    QVERIFY(w.blocks.at(1).steps.at(1).startOnTap);
}

void TstWorkoutDraft::movingToTheSameBlockChangesNothing()
{
    Workout w = sample();
    const WorkoutDraft::Position p = WorkoutDraft::moveStepToBlock(w, 0, 1, 0);
    QCOMPARE(p.block, 0);
    QCOMPARE(p.step, 1);
    QCOMPARE(keys(w.blocks.at(0)), QStringList() << "a" << "b" << "c");
}

void TstWorkoutDraft::theFirstStepNeverWaitsForATap()
{
    Workout w = sample();
    w.blocks[0].steps[1].startOnTap = true;
    const WorkoutDraft::Position p = WorkoutDraft::moveStep(w, 0, 1, -1);
    QCOMPARE(p.step, 0);
    QVERIFY(!w.blocks.at(0).steps.at(0).startOnTap);
    w.blocks[1].steps[0].startOnTap = true;
    WorkoutDraft::removeBlock(w, 0);
    QVERIFY(!w.blocks.at(0).steps.at(0).startOnTap);
}

void TstWorkoutDraft::theFlagReturnsNotWhenAStepLeavesTheFirstPlace()
{
    Workout w = sample();
    w.blocks[0].steps[1].startOnTap = true;
    WorkoutDraft::moveStep(w, 0, 1, -1);
    WorkoutDraft::moveStep(w, 0, 0, 1);
    QVERIFY(!w.blocks.at(0).steps.at(1).startOnTap);
}

void TstWorkoutDraft::restTypeClearsTheExerciseAndIsTimed()
{
    Step s = step("a", 0);
    s.reps = 12;
    WorkoutDraft::setStepType(s, StepType::Rest);
    QCOMPARE(s.type, StepType::Rest);
    QVERIFY(s.exerciseKey.isEmpty());
    QCOMPARE(s.reps, 0);
    QCOMPARE(s.durationSec, WorkoutDraft::defaultDurationSec);

    Step timed = step("a", 45);
    WorkoutDraft::setStepType(timed, StepType::Rest);
    QCOMPARE(timed.durationSec, 45);
}

void TstWorkoutDraft::otherTypesKeepTheirValues()
{
    Step s = step("a", 45, true);
    WorkoutDraft::setStepType(s, StepType::Warmup);
    QCOMPARE(s.type, StepType::Warmup);
    QCOMPARE(s.exerciseKey, QStringLiteral("a"));
    QCOMPARE(s.durationSec, 45);
    QVERIFY(s.startOnTap);
}

void TstWorkoutDraft::switchingTheEndingSetsTheDefaultOfTheNewOne()
{
    Step s = step("a", 45);
    WorkoutDraft::setStepTimed(s, false);
    QCOMPARE(s.durationSec, 0);
    QCOMPARE(s.reps, WorkoutDraft::defaultReps);
    s.reps = 20;
    WorkoutDraft::setStepTimed(s, false);
    QCOMPARE(s.reps, 20);
    WorkoutDraft::setStepTimed(s, true);
    QCOMPARE(s.reps, 0);
    QCOMPARE(s.durationSec, WorkoutDraft::defaultDurationSec);
}

void TstWorkoutDraft::aRestStaysTimed()
{
    Step s;
    s.type = StepType::Rest;
    s.durationSec = 10;
    WorkoutDraft::setStepTimed(s, false);
    QCOMPARE(s.durationSec, WorkoutDraft::defaultDurationSec);
    QCOMPARE(s.reps, 0);
}

void TstWorkoutDraft::aPaceStartsAtItsDefaultAndStaysInsideTheLimits()
{
    Step step;
    step.type = StepType::Work;
    step.exerciseKey = "a";
    WorkoutDraft::setStepReps(step, 10);
    // The time of one repetition starts at 2 s, a total time at 2 s for each repetition.
    WorkoutDraft::setStepPace(step, PaceKind::PerRep);
    QVERIFY(step.pace == PaceKind::PerRep);
    QCOMPARE(step.paceMs, 2000);
    WorkoutDraft::setStepPace(step, PaceKind::Total);
    QVERIFY(step.pace == PaceKind::Total);
    QCOMPARE(step.paceMs, 20000);
    QVERIFY(validateStep(step).isEmpty());
    // Values outside the limits are clamped: 0.5 s and 60 s for a repetition.
    WorkoutDraft::setStepPace(step, PaceKind::Total, 1);
    QCOMPARE(step.paceMs, 5000);
    WorkoutDraft::setStepPace(step, PaceKind::Total, 10000000);
    QCOMPARE(step.paceMs, 600000);
    WorkoutDraft::setStepPace(step, PaceKind::PerRep, 100);
    QCOMPARE(step.paceMs, 500);
    WorkoutDraft::setStepPace(step, PaceKind::PerRep, 99999);
    QCOMPARE(step.paceMs, 60000);
    QVERIFY(validateStep(step).isEmpty());
    // None clears it.
    WorkoutDraft::setStepPace(step, PaceKind::None);
    QVERIFY(step.pace == PaceKind::None);
    QCOMPARE(step.paceMs, 0);
}

void TstWorkoutDraft::changingTheRepetitionsKeepsATotalTimeValid()
{
    Step step;
    step.type = StepType::Work;
    step.exerciseKey = "a";
    WorkoutDraft::setStepReps(step, 10);
    WorkoutDraft::setStepPace(step, PaceKind::Total, 6000);
    // More repetitions in the same time would make each shorter than 0.5 s: the total grows.
    WorkoutDraft::setStepReps(step, 20);
    QCOMPARE(step.paceMs, 10000);
    QVERIFY(validateStep(step).isEmpty());
    WorkoutDraft::setStepPace(step, PaceKind::Total, 1200000);
    WorkoutDraft::setStepReps(step, 2);
    QCOMPARE(step.paceMs, 120000);
    QVERIFY(validateStep(step).isEmpty());
    // The time per repetition does not depend on the repetitions.
    WorkoutDraft::setStepPace(step, PaceKind::PerRep, 3000);
    WorkoutDraft::setStepReps(step, 400);
    QCOMPARE(step.paceMs, 3000);
    QCOMPARE(step.reps, 400);
}

void TstWorkoutDraft::leavingTheRepetitionsDropsThePace()
{
    Step step;
    step.type = StepType::Work;
    step.exerciseKey = "a";
    WorkoutDraft::setStepReps(step, 10);
    WorkoutDraft::setStepPace(step, PaceKind::PerRep, 2500);
    WorkoutDraft::setStepTimed(step, true);
    QVERIFY(step.pace == PaceKind::None);
    QCOMPARE(step.paceMs, 0);
    QCOMPARE(step.durationSec, 30);
    // A pace cannot be set on a step that ends by duration.
    WorkoutDraft::setStepPace(step, PaceKind::PerRep, 2500);
    QVERIFY(step.pace == PaceKind::None);
    WorkoutDraft::setStepReps(step, 8);
    WorkoutDraft::setStepPace(step, PaceKind::Total);
    WorkoutDraft::setStepDuration(step, 45);
    QVERIFY(step.pace == PaceKind::None);
    QCOMPARE(step.reps, 0);
    QCOMPARE(step.durationSec, 45);
    // A rest never has one.
    WorkoutDraft::setStepReps(step, 8);
    WorkoutDraft::setStepPace(step, PaceKind::Total);
    WorkoutDraft::setStepType(step, StepType::Rest);
    QVERIFY(step.pace == PaceKind::None);
    QVERIFY(step.isTimed());
}

void TstWorkoutDraft::aStepMovesAcrossTheEndsOfItsBlock()
{
    Workout w = sample();
    // Down from the last place of block 0 goes to the start of block 1.
    WorkoutDraft::Position p = WorkoutDraft::moveStepAcross(w, 0, 2, 1);
    QCOMPARE(p.block, 1);
    QCOMPARE(p.step, 0);
    QCOMPARE(keys(w.blocks.at(0)), QStringList() << "a" << "b");
    QCOMPARE(keys(w.blocks.at(1)), QStringList() << "c" << "d");
    // Up from the first place of block 1 goes back to the end of block 0.
    p = WorkoutDraft::moveStepAcross(w, 1, 0, -1);
    QCOMPARE(p.block, 0);
    QCOMPARE(p.step, 2);
    QCOMPARE(keys(w.blocks.at(0)), QStringList() << "a" << "b" << "c");
    QCOMPARE(keys(w.blocks.at(1)), QStringList() << "d");
    // Inside a block it is an ordinary move, one place or more.
    p = WorkoutDraft::moveStepAcross(w, 0, 0, 2);
    QCOMPARE(p.step, 2);
    QCOMPARE(keys(w.blocks.at(0)), QStringList() << "b" << "c" << "a");
    // Three places down from the middle of block 0 passes the end into block 1.
    p = WorkoutDraft::moveStepAcross(w, 0, 1, 3);
    QCOMPARE(p.block, 1);
    QCOMPARE(p.step, 1);
    QCOMPARE(keys(w.blocks.at(0)), QStringList() << "b" << "a");
    QCOMPARE(keys(w.blocks.at(1)), QStringList() << "d" << "c");
    // A step that does not exist changes nothing.
    QVERIFY(!WorkoutDraft::moveStepAcross(w, 5, 0, 1).isValid());
    QVERIFY(!WorkoutDraft::moveStepAcross(w, 0, 9, 1).isValid());
}

void TstWorkoutDraft::aStepStopsAtTheFirstAndTheLastPlaceOfTheWorkout()
{
    Workout w = sample();
    WorkoutDraft::Position p = WorkoutDraft::moveStepAcross(w, 0, 0, -1);
    QCOMPARE(p.block, 0);
    QCOMPARE(p.step, 0);
    QCOMPARE(keys(w.blocks.at(0)), QStringList() << "a" << "b" << "c");
    p = WorkoutDraft::moveStepAcross(w, 1, 0, 4);
    QCOMPARE(p.block, 1);
    QCOMPARE(p.step, 0);
    QCOMPARE(keys(w.blocks.at(1)), QStringList() << "d");
    // Many places up from the end reach the very first place and stop there.
    p = WorkoutDraft::moveStepAcross(w, 1, 0, -50);
    QCOMPARE(p.block, 0);
    QCOMPARE(p.step, 0);
    QCOMPARE(keys(w.blocks.at(0)), QStringList() << "d" << "a" << "b" << "c");
    QVERIFY(w.blocks.at(1).steps.isEmpty());
}

void TstWorkoutDraft::movingNeverChangesThePlannedTotal()
{
    Workout w = sample();
    w.blocks[0].rounds = 3;
    w.blocks[0].restBetweenRoundsSec = 10;
    w.restBetweenBlocksSec = 5;
    const int before = totalPlannedSeconds(w).seconds;
    // 3 rounds of 90 s with two rests of 10 s, 5 s between blocks, one block of 30 s.
    QCOMPARE(before, 3 * 90 + 2 * 10 + 5 + 30);
    // Reordering steps inside a block and the blocks themselves keeps the total.
    WorkoutDraft::moveStep(w, 0, 0, 2);
    QCOMPARE(totalPlannedSeconds(w).seconds, before);
    WorkoutDraft::moveBlock(w, 0, 1);
    QCOMPARE(totalPlannedSeconds(w).seconds, before);
    // A step that crosses into a block with other rounds changes the total by the difference.
    WorkoutDraft::moveStepAcross(w, 1, 0, -1);
    // The first step of the block of 3 rounds goes to the block of 1 round: 3 x 30 s fewer, 30 s more.
    QCOMPARE(totalPlannedSeconds(w).seconds, before - 3 * 30 + 30);
}

void TstWorkoutDraft::aDuplicatedBlockIsIndependentOfTheOriginal()
{
    Workout w = sample();
    w.blocks[0].rounds = 2;
    w.blocks[0].steps[1].durationSec = 45;
    WorkoutDraft::duplicateBlock(w, 0);
    QCOMPARE(w.blocks.size(), 3);
    // Editing the copy leaves the original, and the other way round.
    w.blocks[1].rounds = 7;
    w.blocks[1].steps[1].durationSec = 12;
    w.blocks[1].steps[0].exerciseKey = QStringLiteral("changed");
    QCOMPARE(w.blocks.at(0).rounds, 2);
    QCOMPARE(w.blocks.at(0).steps.at(1).durationSec, 45);
    QCOMPARE(w.blocks.at(0).steps.at(0).exerciseKey, QStringLiteral("a"));
    w.blocks[0].steps[2].durationSec = 99;
    QCOMPARE(w.blocks.at(1).steps.at(2).durationSec, 30);
    // The copy has every field of the original.
    Workout fresh = sample();
    fresh.blocks[0].restBetweenRoundsSec = 20;
    fresh.blocks[0].steps[0].type = StepType::Warmup;
    fresh.blocks[0].steps[2].startOnTap = true;
    WorkoutDraft::duplicateBlock(fresh, 0);
    QCOMPARE(fresh.blocks.at(1).restBetweenRoundsSec, 20);
    QCOMPARE(fresh.blocks.at(1).steps.at(0).type, StepType::Warmup);
    QVERIFY(fresh.blocks.at(1).steps.at(2).startOnTap);
}

void TstWorkoutDraft::duplicatingAndMovingKeepTheWorkoutValid()
{
    Workout w = sample();
    for (Block &b : w.blocks) {
        for (Step &s : b.steps)
            s.exerciseKey = QStringLiteral("x-") + s.exerciseKey;
    }
    w.name = QStringLiteral("W");
    QVERIFY(validate(w).isEmpty());
    WorkoutDraft::duplicateBlock(w, 0);
    QVERIFY(validate(w).isEmpty());
    WorkoutDraft::moveBlock(w, 2, -2);
    QVERIFY(validate(w).isEmpty());
    WorkoutDraft::moveStepAcross(w, 1, 2, 1);
    QVERIFY(validate(w).isEmpty());
    // A block that loses its only step stays and is reported as empty.
    WorkoutDraft::moveStepAcross(w, 0, 0, 1);
    bool empty = false;
    for (const Problem &p : validate(w))
        empty = empty || (p.code == Problem::EmptyBlock && p.blockIndex == 0);
    QVERIFY(empty);
    WorkoutDraft::moveStepAcross(w, 1, 0, -1);
    QVERIFY(validate(w).isEmpty());
    // The limits are reported by the validation, not hidden: duplicating at the limit of blocks.
    while (w.blocks.size() < Limits::maxBlocksPerWorkout)
        WorkoutDraft::duplicateBlock(w, 0);
    QVERIFY(validate(w).isEmpty());
    WorkoutDraft::duplicateBlock(w, 0);
    bool tooMany = false;
    for (const Problem &p : validate(w))
        tooMany = tooMany || p.code == Problem::TooManyBlocks;
    QVERIFY(tooMany);
}

void TstWorkoutDraft::theFirstStepStaysAutomaticAfterEveryMove()
{
    Workout w = sample();
    w.blocks[1].steps[0].startOnTap = true;
    // The step d waits for a tap; moved to the very first place it starts by itself.
    WorkoutDraft::moveStepAcross(w, 1, 0, -50);
    QCOMPARE(w.blocks.at(0).steps.first().exerciseKey, QStringLiteral("d"));
    QVERIFY(!w.blocks.at(0).steps.first().startOnTap);
    w = sample();
    w.blocks[1].steps[0].startOnTap = true;
    WorkoutDraft::moveBlock(w, 1, -1);
    QVERIFY(!w.blocks.at(0).steps.first().startOnTap);
    w = sample();
    w.blocks[1].steps[0].startOnTap = true;
    WorkoutDraft::duplicateBlock(w, 1);
    QVERIFY(w.blocks.at(2).steps.first().startOnTap);
    QVERIFY(!w.blocks.at(0).steps.first().startOnTap);
}

QTEST_APPLESS_MAIN(TstWorkoutDraft)
#include "tst_workoutdraft.moc"

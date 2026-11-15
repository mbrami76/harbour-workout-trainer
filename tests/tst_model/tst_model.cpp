// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "workoutlimits.h"
#include "model.h"
#include "musclegroups.h"
#include "plan.h"
#include "testsupport.h"

class TstModel : public QObject
{
    Q_OBJECT

private slots:
    void init() { tempoSnapshot().clear(); }
    void validWorkoutHasNoProblems();
    void durationLimits_data();
    void durationLimits();
    void repsLimits_data();
    void repsLimits();
    void roundsLimits_data();
    void roundsLimits();
    void restLimits_data();
    void restLimits();
    void stepCountLimit();
    void blockCountLimit();
    void exerciseRules();
    void durationAndRepsAreExclusive();
    void emptyWorkoutAndBlock();
    void problemsCarryPosition();
    void totalWithoutRepSteps();
    void totalWithRepSteps();
    void quickBuilder();
    void tempoStepIsValid();
    void tempoNeverMakesAStepInvalid();
    void startOnTapNeedsNoValidation();
    void totalCountsTempoSteps();
    void totalBothWays();
    void normalizeClearsTheFirstStepOnly();
    void normalizeSkipsEmptyBlocksAndHandlesEmptyWorkouts();
    void quickBuilderWithoutRest();
    void muscleBits();
    void muscleBitPositionsArePinned();
    void muscleSetIsSixtyFourBitsWide();
    void muscleKeysArePinned();
    void muscleKeyLookups();
    void retiredGroupsAreConverted();
    void keysReadFromAFile();
    void stepPaceIsCheckedByTheCore();
    void aRhythmWinsOverAStoredPace();
    void stepPaceNamesRoundTrip();
};

static Workout single(const Step &step)
{
    return workout(QList<Block>() << block(QList<Step>() << step));
}

void TstModel::validWorkoutHasNoProblems()
{
    const Workout w = workout(QList<Block>() << block(QList<Step>() << work("a", 30) << rest(10), 3, 5), 20);
    QVERIFY(validate(w).isEmpty());
}

void TstModel::durationLimits_data()
{
    QTest::addColumn<int>("seconds");
    QTest::addColumn<int>("code");

    const int none = -1;
    QTest::newRow("minimum") << Limits::minDurationSec << none;
    QTest::newRow("maximum") << Limits::maxDurationSec << none;
    QTest::newRow("one above maximum") << Limits::maxDurationSec + 1 << int(Problem::DurationOutOfRange);
    QTest::newRow("negative") << -1 << int(Problem::DurationOutOfRange);
    QTest::newRow("zero without repetitions") << 0 << int(Problem::MissingDurationOrReps);
}

void TstModel::durationLimits()
{
    QFETCH(int, seconds);
    QFETCH(int, code);

    const QList<Problem> found = validate(single(work("a", seconds)));
    if (code < 0)
        QVERIFY(found.isEmpty());
    else
        QVERIFY(hasProblem(found, Problem::Code(code)));
}

void TstModel::repsLimits_data()
{
    QTest::addColumn<int>("reps");
    QTest::addColumn<int>("code");

    const int none = -1;
    QTest::newRow("minimum") << Limits::minReps << none;
    QTest::newRow("maximum") << Limits::maxReps << none;
    QTest::newRow("one above maximum") << Limits::maxReps + 1 << int(Problem::RepsOutOfRange);
    QTest::newRow("negative") << -1 << int(Problem::RepsOutOfRange);
}

void TstModel::repsLimits()
{
    QFETCH(int, reps);
    QFETCH(int, code);

    const QList<Problem> found = validate(single(repStep("a", reps)));
    if (code < 0)
        QVERIFY(found.isEmpty());
    else
        QVERIFY(hasProblem(found, Problem::Code(code)));
}

void TstModel::roundsLimits_data()
{
    QTest::addColumn<int>("rounds");
    QTest::addColumn<bool>("valid");

    QTest::newRow("minimum") << Limits::minRounds << true;
    QTest::newRow("maximum") << Limits::maxRounds << true;
    QTest::newRow("zero") << Limits::minRounds - 1 << false;
    QTest::newRow("one above maximum") << Limits::maxRounds + 1 << false;
}

void TstModel::roundsLimits()
{
    QFETCH(int, rounds);
    QFETCH(bool, valid);

    const Workout w = workout(QList<Block>() << block(QList<Step>() << work("a", 5), rounds));
    QCOMPARE(validate(w).isEmpty(), valid);
    if (!valid)
        QVERIFY(hasProblem(validate(w), Problem::RoundsOutOfRange));
}

void TstModel::restLimits_data()
{
    QTest::addColumn<int>("seconds");
    QTest::addColumn<bool>("valid");

    QTest::newRow("minimum") << Limits::minRestSec << true;
    QTest::newRow("maximum") << Limits::maxRestSec << true;
    QTest::newRow("negative") << Limits::minRestSec - 1 << false;
    QTest::newRow("one above maximum") << Limits::maxRestSec + 1 << false;
}

void TstModel::restLimits()
{
    QFETCH(int, seconds);
    QFETCH(bool, valid);

    const Workout betweenRounds = workout(QList<Block>() << block(QList<Step>() << work("a", 5), 2, seconds));
    QCOMPARE(validate(betweenRounds).isEmpty(), valid);
    if (!valid)
        QVERIFY(hasProblem(validate(betweenRounds), Problem::RestBetweenRoundsOutOfRange));

    const Workout betweenBlocks = workout(QList<Block>() << block(QList<Step>() << work("a", 5)), seconds);
    QCOMPARE(validate(betweenBlocks).isEmpty(), valid);
    if (!valid)
        QVERIFY(hasProblem(validate(betweenBlocks), Problem::RestBetweenBlocksOutOfRange));
}

void TstModel::stepCountLimit()
{
    QList<Step> steps;
    for (int i = 0; i < Limits::maxStepsPerBlock; ++i)
        steps.append(work("a", 5));
    QVERIFY(validate(workout(QList<Block>() << block(steps))).isEmpty());

    steps.append(work("a", 5));
    QVERIFY(hasProblem(validate(workout(QList<Block>() << block(steps))), Problem::TooManySteps));
}

void TstModel::blockCountLimit()
{
    QList<Block> blocks;
    for (int i = 0; i < Limits::maxBlocksPerWorkout; ++i)
        blocks.append(block(QList<Step>() << work("a", 5)));
    QVERIFY(validate(workout(blocks)).isEmpty());

    blocks.append(block(QList<Step>() << work("a", 5)));
    QVERIFY(hasProblem(validate(workout(blocks)), Problem::TooManyBlocks));
}

void TstModel::exerciseRules()
{
    QVERIFY(hasProblem(validate(single(work(QString(), 5))), Problem::MissingExercise));
    QVERIFY(hasProblem(validate(single(timedStep(StepType::Warmup, QString(), 5))), Problem::MissingExercise));
    QVERIFY(hasProblem(validate(single(timedStep(StepType::Cooldown, QString(), 5))), Problem::MissingExercise));
    QVERIFY(validate(single(timedStep(StepType::Warmup, "a", 5))).isEmpty());
    QVERIFY(validate(single(timedStep(StepType::Cooldown, "a", 5))).isEmpty());

    QVERIFY(validate(single(rest(5))).isEmpty());
    QVERIFY(hasProblem(validate(single(timedStep(StepType::Rest, "a", 5))), Problem::RestHasExercise));

    Step repRest;
    repRest.type = StepType::Rest;
    repRest.reps = 5;
    QVERIFY(hasProblem(validate(single(repRest)), Problem::RestNotTimed));
}

void TstModel::durationAndRepsAreExclusive()
{
    Step both = work("a", 5);
    both.reps = 5;
    QVERIFY(hasProblem(validate(single(both)), Problem::BothDurationAndReps));
    QVERIFY(hasProblem(validate(single(work("a", 0))), Problem::MissingDurationOrReps));
}

void TstModel::emptyWorkoutAndBlock()
{
    QVERIFY(hasProblem(validate(Workout()), Problem::NoBlocks));
    QVERIFY(hasProblem(validate(workout(QList<Block>() << Block())), Problem::EmptyBlock));
}

void TstModel::problemsCarryPosition()
{
    const Workout w = workout(QList<Block>()
        << block(QList<Step>() << work("a", 5))
        << block(QList<Step>() << work("a", 5) << work("a", Limits::maxDurationSec + 1)));
    const QList<Problem> found = validate(w);
    QCOMPARE(found.size(), 1);
    QCOMPARE(found.first().blockIndex, 1);
    QCOMPARE(found.first().stepIndex, 1);
}

void TstModel::totalWithoutRepSteps()
{
    // Block 1: 3 rounds of (30 + 10 + 30 + 10) s with 5 s between rounds = 240 + 10 s.
    // Block 2: one 45 s step; 20 s between blocks.
    const Workout w = workout(QList<Block>()
        << block(QList<Step>() << work("a", 30) << rest(10) << work("b", 30) << rest(10), 3, 5)
        << block(QList<Step>() << work("c", 45)), 20);
    const PlannedTotal total = totalPlannedSeconds(w);
    QCOMPARE(total.seconds, 240 + 10 + 20 + 45);
    QVERIFY(!total.hasRepSteps);
}

void TstModel::totalWithRepSteps()
{
    // Repetition steps add no time; the timed rest after them does.
    const Workout w = workout(QList<Block>()
        << block(QList<Step>() << repStep("a", 12) << rest(15), 2, 0)
        << block(QList<Step>() << work("b", 60)), 20);
    const PlannedTotal total = totalPlannedSeconds(w);
    QCOMPARE(total.seconds, 15 * 2 + 20 + 60);
    QVERIFY(total.hasRepSteps);
}

void TstModel::tempoStepIsValid()
{
    const Tempo forms[] = { Tempo::repsAndTotal(7, 20000), Tempo::timePerRep(1500, 4),
                            Tempo::phases(QList<int>() << 2 << 3 << 1, 8) };
    for (const Tempo &t : forms) {
        QVERIFY(validateStep(tempoStep("a", t)).isEmpty());
        QVERIFY(validateStep(onTap(tempoStep("a", t))).isEmpty());
    }
}

void TstModel::tempoNeverMakesAStepInvalid()
{
    // The tempo belongs to the exercise: a timed step, a rest and a repetition step validate by
    // their own rules whatever the snapshot holds.
    tempoSnapshot().insert("a", Tempo::timePerRep(1000, 5));
    QVERIFY(validateStep(work("a", 30)).isEmpty());
    QVERIFY(validateStep(rest(10)).isEmpty());
    Step warm = repStep("a", 6);
    warm.type = StepType::Warmup;
    QVERIFY(validateStep(warm).isEmpty());
    Step step = repStep("a", 1000);
    const QList<Problem> found = validateStep(step);
    QCOMPARE(found.size(), 1);
    QVERIFY(found.first().code == Problem::RepsOutOfRange);
}

void TstModel::startOnTapNeedsNoValidation()
{
    QVERIFY(validateStep(onTap(work("a", 30))).isEmpty());
    QVERIFY(validateStep(onTap(rest(30))).isEmpty());
    QVERIFY(validateStep(onTap(repStep("a", 10))).isEmpty());
    QVERIFY(validate(workout(QList<Block>() << block(QList<Step>() << onTap(work("a", 30))))).isEmpty());
}

void TstModel::totalCountsTempoSteps()
{
    // A tempo step of 10 repetitions at 2 s adds 20 s; a plain repetition step adds nothing.
    const Workout w = workout(QList<Block>()
        << block(QList<Step>() << tempoStep("a", Tempo::timePerRep(2000, 7), 10) << repStep("b", 12) << rest(15), 2, 5)
        << block(QList<Step>() << work("c", 60)), 10);
    const PlannedTotal total = totalPlannedSeconds(w, tempoSnapshot());
    QCOMPARE(total.seconds, 2 * (20 + 15) + 5 + 10 + 60);
    QCOMPARE(total.secondsTimedOnly, 2 * 15 + 5 + 10 + 60);
    QVERIFY(total.hasRepSteps);
    QVERIFY(total.hasTempoSteps);
    // Only tempo steps among the repetition steps: nothing is left out.
    const Workout only = workout(QList<Block>() << block(QList<Step>() << tempoStep("a", Tempo::timePerRep(1500, 4)) << rest(10)));
    const PlannedTotal t2 = totalPlannedSeconds(only, tempoSnapshot());
    QCOMPARE(t2.seconds, 6 + 10);
    QVERIFY(!t2.hasRepSteps);
    QVERIFY(t2.hasTempoSteps);
}

void TstModel::totalBothWays()
{
    // The milliseconds of tempo steps are summed before the total is rounded to a second.
    const Workout w = workout(QList<Block>()
        << block(QList<Step>() << tempoStep("a", Tempo::timePerRep(1500, 3)), 3));
    const PlannedTotal total = totalPlannedSeconds(w, tempoSnapshot());
    QCOMPARE(total.seconds, 14);   // 13.5 s
    QCOMPARE(total.secondsTimedOnly, 0);
    QCOMPARE(Workout().blocks.size(), 0);
    const PlannedTotal none = totalPlannedSeconds(Workout());
    QCOMPARE(none.seconds, 0);
    QVERIFY(!none.hasTempoSteps);
}

void TstModel::normalizeClearsTheFirstStepOnly()
{
    Workout w = workout(QList<Block>()
        << block(QList<Step>() << onTap(work("a", 5)) << onTap(work("b", 5)), 2)
        << block(QList<Step>() << onTap(work("c", 5))));
    normalizeStartModes(w);
    QVERIFY(!w.blocks.at(0).steps.at(0).startOnTap);
    QVERIFY(w.blocks.at(0).steps.at(1).startOnTap);
    QVERIFY(w.blocks.at(1).steps.at(0).startOnTap);
    // Idempotent, and it changes nothing else.
    const Workout once = w;
    normalizeStartModes(w);
    QVERIFY(w.blocks.at(0).steps.at(1).startOnTap == once.blocks.at(0).steps.at(1).startOnTap);
    QCOMPARE(w.blocks.size(), once.blocks.size());
    Step tempo = onTap(tempoStep("a", Tempo::timePerRep(1000, 4)));
    Workout t = workout(QList<Block>() << block(QList<Step>() << tempo));
    normalizeStartModes(t);
    QVERIFY(!t.blocks.first().steps.first().startOnTap);
    QCOMPARE(t.blocks.first().steps.first().reps, 4);
}

void TstModel::normalizeSkipsEmptyBlocksAndHandlesEmptyWorkouts()
{
    Workout w = workout(QList<Block>() << Block() << block(QList<Step>() << onTap(work("a", 5)) << onTap(work("b", 5))));
    normalizeStartModes(w);
    QVERIFY(!w.blocks.at(1).steps.at(0).startOnTap);
    QVERIFY(w.blocks.at(1).steps.at(1).startOnTap);
    Workout none;
    normalizeStartModes(none);
    QVERIFY(none.blocks.isEmpty());
}

void TstModel::quickBuilder()
{
    const Workout w = quickWorkout("Quick", "burpee", 40, 20, 4);
    QVERIFY(validate(w).isEmpty());
    QCOMPARE(w.blocks.size(), 1);
    QCOMPARE(w.blocks.first().rounds, 4);
    QCOMPARE(w.blocks.first().steps.size(), 1);
    QCOMPARE(w.blocks.first().restBetweenRoundsSec, 20);

    const QList<PlanStep> plan = expandPlan(w);
    QCOMPARE(plan.size(), 7);
    QVERIFY(plan.first().type == StepType::Work);
    QVERIFY(plan.last().type == StepType::Work);
    QCOMPARE(totalPlannedSeconds(w).seconds, 4 * 40 + 3 * 20);
}

void TstModel::quickBuilderWithoutRest()
{
    const Workout w = quickWorkout("Quick", "burpee", 40, 0, 4);
    QCOMPARE(expandPlan(w).size(), 4);
    QCOMPARE(totalPlannedSeconds(w).seconds, 160);
}

void TstModel::muscleBits()
{
    QCOMPARE(muscleGroupCount(), 21);
    QCOMPARE(allMuscles(), MuscleSet(0x1fffff));
    QCOMPARE(muscleBit(MuscleGroup::Chest), MuscleSet(1));
    QCOMPARE(muscleBit(MuscleGroup::Calves), MuscleSet(1) << 8);
    QCOMPARE(muscleBit(MuscleGroup::Shins), MuscleSet(1) << 20);
    MuscleSet all = 0;
    MuscleSet active = 0;
    for (int i = 0; i < muscleGroupCount(); ++i) {
        all |= muscleBit(MuscleGroup(i));
        if (!MuscleKeys::isRetired(MuscleGroup(i)))
            active |= muscleBit(MuscleGroup(i));
    }
    QCOMPARE(all, allMuscles());
    QCOMPARE(active, activeMuscles());
    QCOMPARE(activeMuscles(), allMuscles() & ~(muscleBit(MuscleGroup::Chest) | muscleBit(MuscleGroup::Back) | muscleBit(MuscleGroup::Arms)));
}

void TstModel::muscleBitPositionsArePinned()
{
    // Stored sets depend on these positions: they must never change or be reused.
    const MuscleGroup order[] = {
        MuscleGroup::Chest, MuscleGroup::Back, MuscleGroup::Shoulders, MuscleGroup::Arms, MuscleGroup::Core,
        MuscleGroup::Glutes, MuscleGroup::Quadriceps, MuscleGroup::Hamstrings, MuscleGroup::Calves,
        MuscleGroup::LowerAbdomen, MuscleGroup::LowerBack, MuscleGroup::UpperChest, MuscleGroup::LowerChest,
        MuscleGroup::Obliques, MuscleGroup::Trapezius, MuscleGroup::Lats, MuscleGroup::MidBack,
        MuscleGroup::Biceps, MuscleGroup::Triceps, MuscleGroup::Forearms, MuscleGroup::Shins
    };
    for (int i = 0; i < 21; ++i) {
        QCOMPARE(int(order[i]), i);
        QCOMPARE(muscleBit(order[i]), MuscleSet(1) << i);
    }
}

void TstModel::muscleSetIsSixtyFourBitsWide()
{
    QCOMPARE(int(sizeof(MuscleSet)), 8);
    // Positions above 31 work in shifts and masks.
    const MuscleSet high = MuscleSet(1) << 62;
    QVERIFY(high > MuscleSet(0xffffffffu));
    QCOMPARE((high | MuscleSet(1)) & ~MuscleSet(1), high);
    // The table leaves room: at most 63 positions can be used.
    QVERIFY(muscleGroupCount() <= 63);
    QCOMPARE(allMuscles() >> muscleGroupCount(), MuscleSet(0));
    QCOMPARE(MuscleKeys::convertRetired(high | muscleBit(MuscleGroup::Chest)),
             high | muscleBit(MuscleGroup::UpperChest) | muscleBit(MuscleGroup::LowerChest));
    QCOMPARE(MuscleKeys::keysOf(high), QStringList());
}

void TstModel::muscleKeysArePinned()
{
    // Position, key and state of every group. Exports name groups by these keys (R-MUS-4, R-MUS-5).
    struct Row { const char *key; bool retired; };
    const Row rows[] = {
        { "chest", true }, { "back", true }, { "shoulders", false }, { "arms", true }, { "core", false },
        { "glutes", false }, { "quadriceps", false }, { "hamstrings", false }, { "calves", false },
        { "lower-abdomen", false }, { "lower-back", false }, { "upper-chest", false }, { "lower-chest", false },
        { "obliques", false }, { "trapezius", false }, { "lats", false }, { "mid-back", false },
        { "biceps", false }, { "triceps", false }, { "forearms", false }, { "shins", false }
    };
    QStringList active;
    for (int i = 0; i < 21; ++i) {
        const QString key = QString::fromLatin1(rows[i].key);
        QCOMPARE(MuscleKeys::keyOf(MuscleGroup(i)), key);
        QCOMPARE(MuscleKeys::isRetired(MuscleGroup(i)), rows[i].retired);
        MuscleGroup group;
        QVERIFY(MuscleKeys::fromKey(key, &group));
        QCOMPARE(int(group), i);
        if (!rows[i].retired)
            active.append(key);
    }
    QCOMPARE(active.size(), 18);
    QCOMPARE(MuscleKeys::keys(), active);
}

void TstModel::muscleKeyLookups()
{
    MuscleGroup group = MuscleGroup::Chest;
    QVERIFY(!MuscleKeys::fromKey(QString(), &group));
    QVERIFY(!MuscleKeys::fromKey(QStringLiteral("Chest"), &group));
    QVERIFY(!MuscleKeys::fromKey(QStringLiteral("wings"), &group));
    QCOMPARE(MuscleKeys::keyOf(MuscleGroup(40)), QString());
    QVERIFY(!MuscleKeys::isRetired(MuscleGroup(40)));
    QCOMPARE(MuscleKeys::keysOf(0), QStringList());
    // Active keys in position order; retired bits are never listed.
    QCOMPARE(MuscleKeys::keysOf(muscleBit(MuscleGroup::Calves) | muscleBit(MuscleGroup::Shoulders) | muscleBit(MuscleGroup::Biceps)),
             QStringList() << "shoulders" << "calves" << "biceps");
    QCOMPARE(MuscleKeys::keysOf(muscleBit(MuscleGroup::Chest) | muscleBit(MuscleGroup::Core)), QStringList() << "core");
    QCOMPARE(MuscleKeys::keysOf(allMuscles()), MuscleKeys::keys());
    QCOMPARE(MuscleKeys::keysOf(MuscleSet(1) << 25), QStringList());
}

void TstModel::retiredGroupsAreConverted()
{
    const MuscleSet chest = muscleBit(MuscleGroup::Chest);
    const MuscleSet back = muscleBit(MuscleGroup::Back);
    const MuscleSet arms = muscleBit(MuscleGroup::Arms);
    const MuscleSet upperAndLowerChest = muscleBit(MuscleGroup::UpperChest) | muscleBit(MuscleGroup::LowerChest);
    const MuscleSet backGroups = muscleBit(MuscleGroup::Trapezius) | muscleBit(MuscleGroup::Lats) | muscleBit(MuscleGroup::MidBack);
    const MuscleSet armGroups = muscleBit(MuscleGroup::Biceps) | muscleBit(MuscleGroup::Triceps) | muscleBit(MuscleGroup::Forearms);
    QCOMPARE(MuscleKeys::convertRetired(chest), upperAndLowerChest);
    QCOMPARE(MuscleKeys::convertRetired(back), backGroups);
    QCOMPARE(MuscleKeys::convertRetired(arms), armGroups);
    // Mixed sets keep their other bits; a set that already holds a replacement stays a set.
    const MuscleSet core = muscleBit(MuscleGroup::Core);
    QCOMPARE(MuscleKeys::convertRetired(chest | core | arms), upperAndLowerChest | core | armGroups);
    QCOMPARE(MuscleKeys::convertRetired(chest | muscleBit(MuscleGroup::UpperChest)), upperAndLowerChest);
    QCOMPARE(MuscleKeys::convertRetired(chest | back | arms), upperAndLowerChest | backGroups | armGroups);
    // Nothing retired: unchanged. Empty stays empty.
    const MuscleSet plain = core | muscleBit(MuscleGroup::Shins) | muscleBit(MuscleGroup::Shoulders);
    QCOMPARE(MuscleKeys::convertRetired(plain), plain);
    QCOMPARE(MuscleKeys::convertRetired(0), MuscleSet(0));
    // The result never holds a retired bit.
    QCOMPARE(MuscleKeys::convertRetired(allMuscles()), activeMuscles());
}

void TstModel::keysReadFromAFile()
{
    QCOMPARE(MuscleKeys::replacementKeys("chest"), QStringList() << "upper-chest" << "lower-chest");
    QCOMPARE(MuscleKeys::replacementKeys("back"), QStringList() << "trapezius" << "lats" << "mid-back");
    QCOMPARE(MuscleKeys::replacementKeys("arms"), QStringList() << "biceps" << "triceps" << "forearms");
    QCOMPARE(MuscleKeys::replacementKeys("core"), QStringList() << "core");
    QCOMPARE(MuscleKeys::replacementKeys("shins"), QStringList() << "shins");
    QCOMPARE(MuscleKeys::replacementKeys("wings"), QStringList());
    QCOMPARE(MuscleKeys::replacementKeys(QString()), QStringList());
}

void TstModel::stepPaceIsCheckedByTheCore()
{
    // No pace: always fine. Repetitions in a total time: 0.5 s to 60 s for each.
    QVERIFY(validateStep(repStep("a", 10)).isEmpty());
    QVERIFY(validateStep(pacedStep("a", 10, PaceKind::Total, 5000)).isEmpty());
    QVERIFY(validateStep(pacedStep("a", 10, PaceKind::Total, 600000)).isEmpty());
    QVERIFY(hasProblem(validateStep(pacedStep("a", 10, PaceKind::Total, 4999)), Problem::PaceRepTooShort));
    QVERIFY(hasProblem(validateStep(pacedStep("a", 10, PaceKind::Total, 600001)), Problem::PaceRepTooLong));
    QVERIFY(hasProblem(validateStep(pacedStep("a", 10, PaceKind::Total, 0)), Problem::PaceRepTooShort));
    // A time per repetition.
    QVERIFY(validateStep(pacedStep("a", 999, PaceKind::PerRep, 500)).isEmpty());
    QVERIFY(validateStep(pacedStep("a", 1, PaceKind::PerRep, 60000)).isEmpty());
    QVERIFY(hasProblem(validateStep(pacedStep("a", 5, PaceKind::PerRep, 499)), Problem::PaceRepTooShort));
    QVERIFY(hasProblem(validateStep(pacedStep("a", 5, PaceKind::PerRep, 60001)), Problem::PaceRepTooLong));
    // The repetitions keep their own limits (1 to 999), reported once.
    const QList<Problem> many = validateStep(pacedStep("a", 1000, PaceKind::PerRep, 1000));
    QVERIFY(hasProblem(many, Problem::RepsOutOfRange));
    QCOMPARE(many.size(), 1);
    // A pace belongs to repetitions: a step that ends by duration cannot have one.
    Step timed = work("a", 30);
    timed.pace = PaceKind::PerRep;
    timed.paceMs = 1000;
    QVERIFY(hasProblem(validateStep(timed), Problem::PaceWithDuration));
    // The limits apply to a whole workout too, with the position of the step.
    Workout w = workout(QList<Block>() << block(QList<Step>() << repStep("a", 5) << pacedStep("a", 5, PaceKind::PerRep, 100)));
    const QList<Problem> found = validate(w);
    QCOMPARE(found.size(), 1);
    QVERIFY(found.first().code == Problem::PaceRepTooShort);
    QCOMPARE(found.first().stepIndex, 1);
}

void TstModel::aRhythmWinsOverAStoredPace()
{
    ExerciseTempos rhythms;
    rhythms.insert("a", phasesOf({ 2, 4 }, 1));
    // With a rhythm only repetitions are allowed.
    QVERIFY(validateStepFor(repStep("a", 8), true).isEmpty());
    QVERIFY(hasProblem(validateStepFor(work("a", 30), true), Problem::DurationWithRhythm));
    QVERIFY(validateStepFor(work("a", 30), false).isEmpty());
    // A stored pace is kept but not checked while the rhythm exists, and checked again without it.
    const Step stale = pacedStep("a", 8, PaceKind::PerRep, 100);
    QVERIFY(validateStepFor(stale, true).isEmpty());
    QVERIFY(hasProblem(validateStepFor(stale, false), Problem::PaceRepTooShort));
    Workout w = workout(QList<Block>() << block(QList<Step>() << stale));
    QVERIFY(!validate(w).isEmpty());
    QVERIFY(validate(w, rhythms).isEmpty());
    // A rest is never a step of an exercise.
    QVERIFY(validateStepFor(rest(10), true).isEmpty());
}

void TstModel::stepPaceNamesRoundTrip()
{
    const PaceKind kinds[] = { PaceKind::None, PaceKind::Total, PaceKind::PerRep };
    for (PaceKind kind : kinds) {
        PaceKind back = PaceKind::Total;
        QVERIFY(paceKindFromName(paceKindName(kind), &back));
        QVERIFY(back == kind);
    }
    PaceKind unknown = PaceKind::Total;
    QVERIFY(!paceKindFromName("waltz", &unknown));
    QVERIFY(paceKindName(PaceKind::None).isEmpty());
}

QTEST_APPLESS_MAIN(TstModel)

#include "tst_model.moc"

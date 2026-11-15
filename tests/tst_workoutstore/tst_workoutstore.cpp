// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QElapsedTimer>
#include <QtTest>

#include "testdb.h"
#include "exercisestore.h"
#include "metronomestore.h"
#include "workoutlimits.h"
#include "workoutstore.h"

class TstWorkoutStore : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void treeRoundTripKeepsOrder();
    void stepTypesAndKindsRoundTrip();
    void updateReplacesTheTree();
    void forcedFailureRollsBackUpdate();
    void forcedFailureRollsBackCreate();
    void duplicateIsIndependent();
    void duplicateNamesAreUnique();
    void listPutsCopyNumbersAfterThePlainCopy();
    void startModeRoundTrips();
    void stepPaceRoundTrips();
    void aPaceBelowTheLimitIsRefusedAndNothingIsWritten();
    void aStoredPaceIsIgnoredWhileTheExerciseHasARhythm();
    void planUsesTheCurrentTempoOfTheExercise();
    void changingAMetronomeChangesThePlannedTime();
    void exerciseThatLostItsMetronomeRunsAsPlainRepetitionStep();
    void newFieldsDefaultToOff();
    void duplicateCopiesStartMode();
    void aStepStoresNoMetronomeLink();
    void updateChangesStartMode();
    void summaryCountsTempoSteps();
    void createClearsTheFlagOnTheFirstStep();
    void updateClearsTheFlagOnTheFirstStep();
    void aStepMovedToTheFirstPlaceLosesTheFlag();
    void removingTheFirstStepLeavesNoFlagOnTheNewFirst();
    void duplicateOfAFlaggedWorkoutHasNoFlagOnTheFirstStep();
    void readingNormalizesWhatIsStored();
    void flagsOnLaterStepsAndBlocksStay();
    void favoritesComeFirst();
    void summariesCarryTotalsAndFlag();
    void invalidWorkoutsAreRejected();
    void invalidUpdateKeepsOriginal();
    void unknownExerciseIsRejected();
    void removeDeletesTheTreeOnly();
    void unknownIdIsNotFound();
    void volume();


    void iconAndColourRoundTrip();
    void defaultsAreEmptyKeys();
    void unknownStoredColourAndIconAreKept();
    void invalidIconAndColourAreRefusedOnWrite();
    void listCarriesIconAndColour();
    void equalIdentityIsRefused();
    void sameNameWithAnotherColourIsAccepted();
    void sameNameWithAnotherIconIsAccepted();
    void caseAndBlankSpaceVariantsAreRefused();
    void emptyKeysEqualEachOther();
    void editingWithoutChangingTheIdentityIsAccepted();
    void changingIntoAClashIsRefused();
    void renameIconAndColourChangesAreChecked();
    void anEditKeepsWorkingWhenOldDataAlreadyClashes();
    void refusedSaveWritesNothing();
    void duplicateKeepsIconAndColourAndAlwaysPasses();
    void duplicateSkipsSuffixesThatClashOnTheTriple();
    void problemsOfReportsTheIdentityWithoutWriting();
    void problemsOfSkipsTheComparisonForAnUnchangedIdentity();
    void summariesCountBlocksAndSteps();
private:
    Workout sample(const QString &name);
    TestDb t_;
    QString a_, b_, c_;
};

void TstWorkoutStore::init()
{
    t_.db.close();
    QDir(t_.dir.path()).removeRecursively();
    QVERIFY(t_.open());
    a_ = addExercise(t_.db, "A");
    b_ = addExercise(t_.db, "B");
    c_ = addExercise(t_.db, "C");
}

Workout TstWorkoutStore::sample(const QString &name)
{
    Workout w = workout(QList<Block>()
        << block(QList<Step>() << timedStep(StepType::Warmup, a_, 60) << rest(5) << work(b_, 30), 1, 0)
        << block(QList<Step>() << work(a_, 40) << rest(15) << repStep(c_, 12) << rest(15), 3, 20)
        << block(QList<Step>() << timedStep(StepType::Cooldown, c_, 120)), 45);
    w.name = name;
    return w;
}

void TstWorkoutStore::treeRoundTripKeepsOrder()
{
    WorkoutStore store(&t_.db);
    const Workout w = sample("Full body");
    int id = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    QVERIFY(id > 0);

    Workout read;
    QVERIFY(store.get(id, &read));
    QCOMPARE(read.id, id);
    QVERIFY(!read.favorite);
    QVERIFY(sameWorkout(read, w));
    QCOMPARE(read.blocks.at(1).steps.at(2).exerciseKey, c_);
    QCOMPARE(read.blocks.at(1).steps.at(2).reps, 12);
    QCOMPARE(read.blocks.at(1).steps.at(2).durationSec, 0);
    QVERIFY(integrityOk(t_.db));
}

void TstWorkoutStore::stepTypesAndKindsRoundTrip()
{
    WorkoutStore store(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>()
        << timedStep(StepType::Warmup, a_, 1) << work(b_, 3600) << repStep(c_, 999) << rest(1)
        << timedStep(StepType::Cooldown, a_, 2), 99, 600));
    w.restBetweenBlocksSec = 600;
    w.name = QString(80, 'n');
    int id = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    Workout read;
    QVERIFY(store.get(id, &read));
    QVERIFY(sameWorkout(read, w));
    QVERIFY(read.blocks.first().steps.at(0).type == StepType::Warmup);
    QVERIFY(read.blocks.first().steps.at(3).type == StepType::Rest);
    QVERIFY(read.blocks.first().steps.at(3).exerciseKey.isEmpty());
}

void TstWorkoutStore::updateReplacesTheTree()
{
    WorkoutStore store(&t_.db);
    int id = 0;
    QVERIFY(store.create(sample("Before"), &id) == StoreStatus::Ok);
    QVERIFY(store.setFavorite(id, true) == StoreStatus::Ok);
    const int blocksBefore = countRows(t_.db, "block");
    QVERIFY(blocksBefore == 3);

    Workout next = workout(QList<Block>() << block(QList<Step>() << work(a_, 10), 2, 7));
    next.name = "After";
    QVERIFY(store.update(id, next) == StoreStatus::Ok);

    Workout read;
    QVERIFY(store.get(id, &read));
    QVERIFY(sameWorkout(read, next));
    QVERIFY(read.favorite);
    QCOMPARE(countRows(t_.db, "workout"), 1);
    QCOMPARE(countRows(t_.db, "block"), 1);
    QCOMPARE(countRows(t_.db, "step"), 1);
    QVERIFY(integrityOk(t_.db));
}

void TstWorkoutStore::forcedFailureRollsBackUpdate()
{
    WorkoutStore store(&t_.db);
    const Workout original = sample("Original");
    int id = 0;
    QVERIFY(store.create(original, &id) == StoreStatus::Ok);

    // The trigger aborts the insert of a step at position 3, in the middle of the rewrite.
    QVERIFY(t_.db.run("CREATE TRIGGER fail_step BEFORE INSERT ON step WHEN NEW.position = 3 "
                      "BEGIN SELECT RAISE(ABORT, 'forced'); END"));
    Workout changed = workout(QList<Block>()
        << block(QList<Step>() << work(a_, 10) << work(a_, 11) << work(a_, 12) << work(a_, 13) << work(a_, 14)));
    changed.name = "Changed";
    QVERIFY(store.update(id, changed) == StoreStatus::Failed);
    QVERIFY(t_.db.run("DROP TRIGGER fail_step"));

    Workout read;
    QVERIFY(store.get(id, &read));
    QVERIFY(sameWorkout(read, original));
    QCOMPARE(countRows(t_.db, "block"), 3);
    QVERIFY(integrityOk(t_.db));

    // The connection is usable again after the failure.
    QVERIFY(store.update(id, changed) == StoreStatus::Ok);
    QVERIFY(store.get(id, &read));
    QVERIFY(sameWorkout(read, changed));
}

void TstWorkoutStore::forcedFailureRollsBackCreate()
{
    WorkoutStore store(&t_.db);
    QVERIFY(t_.db.run("CREATE TRIGGER fail_step BEFORE INSERT ON step WHEN NEW.position = 1 "
                      "BEGIN SELECT RAISE(ABORT, 'forced'); END"));
    QVERIFY(store.create(sample("Never stored")) == StoreStatus::Failed);
    QCOMPARE(countRows(t_.db, "workout"), 0);
    QCOMPARE(countRows(t_.db, "block"), 0);
    QCOMPARE(countRows(t_.db, "step"), 0);
    QVERIFY(integrityOk(t_.db));
}

void TstWorkoutStore::listPutsCopyNumbersAfterThePlainCopy()
{
    WorkoutStore store(&t_.db);
    int id = 0;
    QVERIFY(store.create(sample("Row"), &id) == StoreStatus::Ok);
    for (int i = 0; i < 11; ++i)
        QVERIFY(store.duplicate(id) == StoreStatus::Ok);
    QStringList names;
    for (const WorkoutSummary &s : store.list())
        names.append(s.name);
    QCOMPARE(names, QStringList() << "Row" << "Row (copy)" << "Row (copy 2)" << "Row (copy 3)" << "Row (copy 4)"
                                  << "Row (copy 5)" << "Row (copy 6)" << "Row (copy 7)" << "Row (copy 8)"
                                  << "Row (copy 9)" << "Row (copy 10)" << "Row (copy 11)");
}

void TstWorkoutStore::duplicateIsIndependent()
{
    WorkoutStore store(&t_.db);
    int id = 0, copyId = 0;
    QVERIFY(store.create(sample("Original"), &id) == StoreStatus::Ok);
    QVERIFY(store.setFavorite(id, true) == StoreStatus::Ok);
    QVERIFY(store.duplicate(id, &copyId) == StoreStatus::Ok);
    QVERIFY(copyId != id);

    Workout copy, original;
    QVERIFY(store.get(copyId, &copy));
    QCOMPARE(copy.name, QStringLiteral("Original (copy)"));
    QVERIFY(!copy.favorite);
    Workout expected = sample("Original (copy)");
    QVERIFY(sameWorkout(copy, expected));

    // Changing the copy leaves the original alone, and the other way round.
    Workout edited = workout(QList<Block>() << block(QList<Step>() << work(a_, 99)));
    edited.name = "Edited copy";
    QVERIFY(store.update(copyId, edited) == StoreStatus::Ok);
    QVERIFY(store.get(id, &original));
    QVERIFY(sameWorkout(original, sample("Original")));

    QVERIFY(store.remove(id) == StoreStatus::Ok);
    QVERIFY(store.get(copyId, &copy));
    QVERIFY(sameWorkout(copy, edited));
    QVERIFY(integrityOk(t_.db));
}

void TstWorkoutStore::startModeRoundTrips()
{
    WorkoutStore store(&t_.db);
    Workout w = workout(QList<Block>()
        << block(QList<Step>() << work(a_, 30) << onTap(rest(10)) << repStep(b_, 8)
                               << onTap(repStep(c_, 7)) << repStep(a_, 5), 2, 5));
    w.name = "Modes";
    int id = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    Workout read;
    QVERIFY(store.get(id, &read));
    QVERIFY(sameWorkout(read, w));
    const QList<Step> &steps = read.blocks.first().steps;
    QVERIFY(!steps.at(0).startOnTap);
    QVERIFY(steps.at(1).startOnTap);
    QVERIFY(!steps.at(2).startOnTap);
    QVERIFY(steps.at(3).startOnTap);
    QVERIFY(!steps.at(4).startOnTap);
    QVERIFY(integrityOk(t_.db));
}

void TstWorkoutStore::planUsesTheCurrentTempoOfTheExercise()
{
    WorkoutStore store(&t_.db);
    MetronomeStore metronomes(&t_.db);
    ExerciseStore exercises(&t_.db);
    const int mid = addMetronome(t_.db, "Slow", phasesOf({ 4, 2, 2 }, 30));
    QVERIFY(exercises.setMetronome(b_, mid) == StoreStatus::Ok);
    Workout w = workout(QList<Block>() << block(QList<Step>() << work(a_, 10) << repStep(b_, 3) << repStep(a_, 3), 2));
    w.name = "Plan";
    int id = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    Workout read;
    QVERIFY(store.get(id, &read));

    const QList<PlanStep> plan = store.plan(read);
    QCOMPARE(plan.size(), 6);
    QVERIFY(plan.at(1).hasTempo);
    // The step keeps its 3 repetitions; the metronome's 30, 5 sets and 20 s rest are not used.
    QCOMPARE(plan.at(1).reps, 3);
    QVERIFY(plan.at(1).tempo == phasesOf({ 4, 2, 2 }, 3));
    QCOMPARE(plan.at(1).plannedMs(), qint64(12000));
    QVERIFY(!plan.at(0).hasTempo);
    QVERIFY(!plan.at(2).hasTempo);

    // The plan is a snapshot: a later change of the metronome does not reach it.
    SavedMetronome m;
    QVERIFY(metronomes.get(mid, &m));
    m.tempo = rhythmOf(Tempo::timePerRep(1000, 4));
    QVERIFY(metronomes.update(mid, m) == StoreStatus::Ok);
    QVERIFY(plan.at(1).tempo == phasesOf({ 4, 2, 2 }, 3));
    const QList<PlanStep> later = store.plan(read);
    QCOMPARE(later.at(1).reps, 3);
    QCOMPARE(later.at(1).plannedMs(), qint64(3000));
}

void TstWorkoutStore::changingAMetronomeChangesThePlannedTime()
{
    WorkoutStore store(&t_.db);
    MetronomeStore metronomes(&t_.db);
    ExerciseStore exercises(&t_.db);
    const int mid = addMetronome(t_.db, "Shared", Tempo::timePerRep(1500, 4));
    // Two exercises share the metronome.
    QVERIFY(exercises.setMetronome(a_, mid) == StoreStatus::Ok);
    QVERIFY(exercises.setMetronome(b_, mid) == StoreStatus::Ok);
    int first = 0, second = 0;
    Workout one = workout(QList<Block>() << block(QList<Step>() << repStep(a_, 4) << rest(10)));
    one.name = "One";
    Workout two = workout(QList<Block>() << block(QList<Step>() << repStep(b_, 4) << work(c_, 30), 2));
    two.name = "Two";
    QVERIFY(store.create(one, &first) == StoreStatus::Ok);
    QVERIFY(store.create(two, &second) == StoreStatus::Ok);
    auto seconds = [&](int id) {
        const QList<WorkoutSummary> list = store.list();
        for (const WorkoutSummary &s : list) {
            if (s.id == id)
                return s.plannedSeconds;
        }
        return -1;
    };
    QCOMPARE(seconds(first), 6 + 10);
    QCOMPARE(seconds(second), 2 * (6 + 30));

    SavedMetronome m;
    QVERIFY(metronomes.get(mid, &m));
    m.tempo = rhythmOf(Tempo::timePerRep(2000, 5));
    QList<ExerciseRef> affected;
    QVERIFY(metronomes.update(mid, m, nullptr, &affected) == StoreStatus::Ok);
    QCOMPARE(affected.size(), 2);
    QCOMPARE(seconds(first), 8 + 10);
    QCOMPARE(seconds(second), 2 * (8 + 30));
}

void TstWorkoutStore::exerciseThatLostItsMetronomeRunsAsPlainRepetitionStep()
{
    WorkoutStore store(&t_.db);
    MetronomeStore metronomes(&t_.db);
    ExerciseStore exercises(&t_.db);
    const int mid = addMetronome(t_.db, "Doomed", Tempo::timePerRep(1000, 4));
    QVERIFY(exercises.setMetronome(a_, mid) == StoreStatus::Ok);
    Workout w = workout(QList<Block>() << block(QList<Step>() << repStep(a_, 6) << rest(10)));
    w.name = "Lost";
    int id = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    QVERIFY(store.plan(w).first().hasTempo);

    // The metronome is deleted: the step is untouched and becomes a plain repetition step.
    QVERIFY(metronomes.remove(mid) == StoreStatus::Ok);
    Workout read;
    QVERIFY(store.get(id, &read));
    QVERIFY(sameWorkout(read, w));
    const QList<PlanStep> plan = store.plan(read);
    QVERIFY(!plan.first().hasTempo);
    QCOMPARE(plan.first().reps, 6);
    const QList<WorkoutSummary> list = store.list();
    QCOMPARE(list.first().plannedSeconds, 10);
    QVERIFY(list.first().hasRepSteps);
    QVERIFY(!list.first().hasTempoSteps);
    QVERIFY(store.update(id, read) == StoreStatus::Ok);
}

void TstWorkoutStore::newFieldsDefaultToOff()
{
    WorkoutStore store(&t_.db);
    int id = 0;
    QVERIFY(store.create(sample("Plain"), &id) == StoreStatus::Ok);
    QSqlQuery q;
    QVERIFY(t_.db.run("SELECT start_on_tap FROM step", QVariantList(), &q));
    int rows = 0;
    while (q.next()) {
        QCOMPARE(q.value(0).toInt(), 0);
        ++rows;
    }
    QVERIFY(rows > 0);
    Workout read;
    QVERIFY(store.get(id, &read));
    for (const Block &b : read.blocks) {
        for (const Step &s : b.steps) {
            QVERIFY(!s.startOnTap);
        }
    }
}

void TstWorkoutStore::duplicateCopiesStartMode()
{
    WorkoutStore store(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << repStep(a_, 4) << onTap(rest(5))));
    w.name = "Original";
    int id = 0, copyId = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    QVERIFY(store.duplicate(id, &copyId) == StoreStatus::Ok);
    Workout copy;
    QVERIFY(store.get(copyId, &copy));
    Workout expected = w;
    expected.name = QStringLiteral("Original (copy)");
    QVERIFY(sameWorkout(copy, expected));
    QVERIFY(copy.blocks.first().steps.at(1).startOnTap);
}

void TstWorkoutStore::aStepStoresNoMetronomeLink()
{
    // Schema 9: the step table has no link column; the tempo belongs to the exercise.
    QSqlQuery q;
    QVERIFY(t_.db.run("PRAGMA table_info(step)", QVariantList(), &q));
    QStringList columns;
    while (q.next())
        columns << q.value(1).toString();
    QVERIFY(!columns.contains("metronome_id"));
    QVERIFY(!columns.contains("tempo"));
    QVERIFY(columns.contains("start_on_tap"));
}

void TstWorkoutStore::updateChangesStartMode()
{
    WorkoutStore store(&t_.db);
    int id = 0;
    Workout w = workout(QList<Block>() << block(QList<Step>() << repStep(a_, 5)));
    w.name = "W";
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    Workout edited = workout(QList<Block>() << block(QList<Step>() << repStep(a_, 5) << onTap(rest(5))));
    edited.name = "W";
    QVERIFY(store.update(id, edited) == StoreStatus::Ok);
    Workout read;
    QVERIFY(store.get(id, &read));
    QVERIFY(read.blocks.first().steps.at(1).startOnTap);
}

void TstWorkoutStore::summaryCountsTempoSteps()
{
    WorkoutStore store(&t_.db);
    ExerciseStore exercises(&t_.db);
    int id = 0;
    const int mid = addMetronome(t_.db, "Pace", Tempo::timePerRep(1500, 4));
    QVERIFY(exercises.setMetronome(a_, mid) == StoreStatus::Ok);
    Workout w = workout(QList<Block>() << block(QList<Step>() << repStep(a_, 4) << rest(10)));
    w.name = "Paced";
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    QList<WorkoutSummary> list = store.list();
    QCOMPARE(list.size(), 1);
    QCOMPARE(list.first().plannedSeconds, 16);
    QVERIFY(!list.first().hasRepSteps);
    QVERIFY(list.first().hasTempoSteps);

    // A repetition step of an exercise without a metronome is left out, and the display says so.
    w.name = "Mixed";
    w.blocks.first().steps.append(repStep(b_, 9));
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    list = store.list();
    QCOMPARE(list.first().plannedSeconds, 16);
    QVERIFY(list.first().hasRepSteps);
    QVERIFY(list.first().hasTempoSteps);
}

static bool firstFlag(WorkoutStore &store, int id)
{
    Workout w;
    return store.get(id, &w) && w.blocks.first().steps.first().startOnTap;
}

void TstWorkoutStore::createClearsTheFlagOnTheFirstStep()
{
    WorkoutStore store(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << onTap(work(a_, 30)) << onTap(rest(5))));
    w.name = "W";
    int id = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    QVERIFY(!firstFlag(store, id));
    QSqlQuery q;
    QVERIFY(t_.db.run("SELECT start_on_tap FROM step ORDER BY id", QVariantList(), &q));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 0);       // not even stored
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 1);
}

void TstWorkoutStore::updateClearsTheFlagOnTheFirstStep()
{
    WorkoutStore store(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << work(a_, 30) << rest(5)));
    w.name = "W";
    int id = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    w.blocks.first().steps.first().startOnTap = true;
    QVERIFY(store.update(id, w) == StoreStatus::Ok);
    QVERIFY(!firstFlag(store, id));
}

void TstWorkoutStore::aStepMovedToTheFirstPlaceLosesTheFlag()
{
    WorkoutStore store(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << work(a_, 30) << onTap(work(b_, 20)) << rest(5)));
    w.name = "W";
    int id = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    // The flagged step is moved to the front.
    Workout moved = w;
    moved.blocks.first().steps.move(1, 0);
    QVERIFY(moved.blocks.first().steps.first().startOnTap);
    QVERIFY(store.update(id, moved) == StoreStatus::Ok);
    Workout read;
    QVERIFY(store.get(id, &read));
    QCOMPARE(read.blocks.first().steps.first().exerciseKey, b_);
    QVERIFY(!read.blocks.first().steps.first().startOnTap);
    QVERIFY(!read.blocks.first().steps.at(1).startOnTap);
}

void TstWorkoutStore::removingTheFirstStepLeavesNoFlagOnTheNewFirst()
{
    WorkoutStore store(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << work(a_, 30) << onTap(work(b_, 20)) << onTap(rest(5))));
    w.name = "W";
    int id = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    Workout shorter = w;
    shorter.blocks.first().steps.removeFirst();
    QVERIFY(store.update(id, shorter) == StoreStatus::Ok);
    Workout read;
    QVERIFY(store.get(id, &read));
    QCOMPARE(read.blocks.first().steps.size(), 2);
    QVERIFY(!read.blocks.first().steps.first().startOnTap);
    QVERIFY(read.blocks.first().steps.at(1).startOnTap);
    // The first block removed: the first step of the next block becomes the first of the workout.
    Workout twoBlocks = workout(QList<Block>() << block(QList<Step>() << work(a_, 30)) << block(QList<Step>() << onTap(work(b_, 20))));
    twoBlocks.name = "Two";
    int two = 0;
    QVERIFY(store.create(twoBlocks, &two) == StoreStatus::Ok);
    Workout read2;
    QVERIFY(store.get(two, &read2));
    QVERIFY(read2.blocks.at(1).steps.first().startOnTap);
    twoBlocks.blocks.removeFirst();
    QVERIFY(store.update(two, twoBlocks) == StoreStatus::Ok);
    QVERIFY(!firstFlag(store, two));
}

void TstWorkoutStore::duplicateOfAFlaggedWorkoutHasNoFlagOnTheFirstStep()
{
    WorkoutStore store(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << work(a_, 30) << onTap(rest(5))));
    w.name = "W";
    int id = 0, copy = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    // A flag put on the first step behind the store's back is not copied.
    QVERIFY(t_.db.run("UPDATE step SET start_on_tap = 1"));
    QVERIFY(store.duplicate(id, &copy) == StoreStatus::Ok);
    QVERIFY(!firstFlag(store, copy));
    QSqlQuery q;
    QVERIFY(t_.db.run("SELECT s.start_on_tap FROM step s JOIN block b ON b.id = s.block_id JOIN workout w ON w.id = b.workout_id "
                      "WHERE w.id = ? ORDER BY s.position", QVariantList() << copy, &q));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 0);
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 1);
}

void TstWorkoutStore::readingNormalizesWhatIsStored()
{
    WorkoutStore store(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << work(a_, 30) << work(b_, 30)));
    w.name = "W";
    int id = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    QVERIFY(t_.db.run("UPDATE step SET start_on_tap = 1"));
    Workout read;
    QVERIFY(store.get(id, &read));
    QVERIFY(!read.blocks.first().steps.at(0).startOnTap);
    QVERIFY(read.blocks.first().steps.at(1).startOnTap);
    // The list read normalises too, and the data stays as it was stored.
    QCOMPARE(store.list().size(), 1);
    QVERIFY(!firstFlag(store, id));
}

void TstWorkoutStore::flagsOnLaterStepsAndBlocksStay()
{
    WorkoutStore store(&t_.db);
    Workout w = workout(QList<Block>()
        << block(QList<Step>() << work(a_, 30) << onTap(work(b_, 20)))
        << block(QList<Step>() << onTap(work(c_, 20))));
    w.name = "W";
    int id = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    Workout read;
    QVERIFY(store.get(id, &read));
    QVERIFY(sameWorkout(read, w));
    QVERIFY(read.blocks.at(1).steps.first().startOnTap);
}

void TstWorkoutStore::duplicateNamesAreUnique()
{
    WorkoutStore store(&t_.db);
    int id = 0, c1 = 0, c2 = 0, c3 = 0;
    QVERIFY(store.create(sample("Base"), &id) == StoreStatus::Ok);
    QVERIFY(store.duplicate(id, &c1) == StoreStatus::Ok);
    QVERIFY(store.duplicate(id, &c2) == StoreStatus::Ok);
    QVERIFY(store.duplicate(c1, &c3) == StoreStatus::Ok);
    Workout w;
    QVERIFY(store.get(c1, &w));
    QCOMPARE(w.name, QStringLiteral("Base (copy)"));
    QVERIFY(store.get(c2, &w));
    QCOMPARE(w.name, QStringLiteral("Base (copy 2)"));
    QVERIFY(store.get(c3, &w));
    QCOMPARE(w.name, QStringLiteral("Base (copy) (copy)"));

    // A name of the maximum length stays within the limit.
    int longId = 0, longCopy = 0;
    QVERIFY(store.create(sample(QString(80, 'x')), &longId) == StoreStatus::Ok);
    QVERIFY(store.duplicate(longId, &longCopy) == StoreStatus::Ok);
    QVERIFY(store.get(longCopy, &w));
    QCOMPARE(w.name.length(), 80);
    QVERIFY(w.name.endsWith(" (copy)"));
}

void TstWorkoutStore::favoritesComeFirst()
{
    WorkoutStore store(&t_.db);
    int ib = 0, ia = 0, ic = 0, id = 0;
    QVERIFY(store.create(sample("beta"), &ib) == StoreStatus::Ok);
    QVERIFY(store.create(sample("Alpha"), &ia) == StoreStatus::Ok);
    QVERIFY(store.create(sample("gamma"), &ic) == StoreStatus::Ok);
    QVERIFY(store.create(sample("Delta"), &id) == StoreStatus::Ok);

    QStringList names;
    for (const WorkoutSummary &s : store.list())
        names << s.name;
    QCOMPARE(names, QStringList() << "Alpha" << "beta" << "Delta" << "gamma");

    QVERIFY(store.setFavorite(ic, true) == StoreStatus::Ok);
    QVERIFY(store.setFavorite(ib, true) == StoreStatus::Ok);
    names.clear();
    for (const WorkoutSummary &s : store.list())
        names << s.name;
    QCOMPARE(names, QStringList() << "beta" << "gamma" << "Alpha" << "Delta");

    QVERIFY(store.setFavorite(ic, false) == StoreStatus::Ok);
    QVERIFY(store.list().first().name == "beta");
    QVERIFY(store.list().first().favorite);
}

void TstWorkoutStore::summariesCarryTotalsAndFlag()
{
    WorkoutStore store(&t_.db);
    int timed = 0, reps = 0;
    Workout w1 = workout(QList<Block>() << block(QList<Step>() << work(a_, 30) << rest(10), 3, 5));
    w1.name = "Timed";
    QVERIFY(store.create(w1, &timed) == StoreStatus::Ok);
    Workout w2 = workout(QList<Block>() << block(QList<Step>() << repStep(a_, 10) << rest(20), 2));
    w2.name = "Reps";
    QVERIFY(store.create(w2, &reps) == StoreStatus::Ok);

    for (const WorkoutSummary &s : store.list()) {
        if (s.id == timed) {
            QCOMPARE(s.plannedSeconds, 3 * 40 + 2 * 5);
            QVERIFY(!s.hasRepSteps);
        } else {
            QCOMPARE(s.id, reps);
            QCOMPARE(s.plannedSeconds, 2 * 20);
            QVERIFY(s.hasRepSteps);
        }
    }
}

void TstWorkoutStore::invalidWorkoutsAreRejected()
{
    WorkoutStore store(&t_.db);
    QList<Problem> problems;

    Workout noName = sample("");
    QVERIFY(store.create(noName, nullptr, &problems) == StoreStatus::Invalid);
    QVERIFY(hasProblem(problems, Problem::NameEmpty));
    Workout blank = sample("   ");
    QVERIFY(store.create(blank, nullptr, &problems) == StoreStatus::Invalid);
    QVERIFY(hasProblem(problems, Problem::NameEmpty));
    Workout longName = sample(QString(81, 'n'));
    QVERIFY(store.create(longName, nullptr, &problems) == StoreStatus::Invalid);
    QVERIFY(hasProblem(problems, Problem::NameTooLong));

    Workout noBlocks;
    noBlocks.name = "Empty";
    QVERIFY(store.create(noBlocks, nullptr, &problems) == StoreStatus::Invalid);
    QVERIFY(hasProblem(problems, Problem::NoBlocks));

    Workout badStep = workout(QList<Block>() << block(QList<Step>() << work(a_, 3601)));
    badStep.name = "Bad step";
    QVERIFY(store.create(badStep, nullptr, &problems) == StoreStatus::Invalid);
    QVERIFY(hasProblem(problems, Problem::DurationOutOfRange));

    Workout badRounds = workout(QList<Block>() << block(QList<Step>() << work(a_, 10), 100));
    badRounds.name = "Bad rounds";
    QVERIFY(store.create(badRounds, nullptr, &problems) == StoreStatus::Invalid);
    QVERIFY(hasProblem(problems, Problem::RoundsOutOfRange));

    QCOMPARE(countRows(t_.db, "workout"), 0);
    QCOMPARE(countRows(t_.db, "block"), 0);
    QCOMPARE(countRows(t_.db, "step"), 0);
}

void TstWorkoutStore::invalidUpdateKeepsOriginal()
{
    WorkoutStore store(&t_.db);
    int id = 0;
    const Workout original = sample("Original");
    QVERIFY(store.create(original, &id) == StoreStatus::Ok);

    Workout bad = sample("Original changed");
    bad.blocks[1].rounds = 0;
    QVERIFY(store.update(id, bad) == StoreStatus::Invalid);
    Workout read;
    QVERIFY(store.get(id, &read));
    QVERIFY(sameWorkout(read, original));
}

void TstWorkoutStore::unknownExerciseIsRejected()
{
    WorkoutStore store(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << work(a_, 10) << work("u-missing", 10)));
    w.name = "Unknown";
    QList<Problem> problems;
    QVERIFY(store.create(w, nullptr, &problems) == StoreStatus::Invalid);
    QCOMPARE(problems.size(), 1);
    QVERIFY(problems.first().code == Problem::UnknownExercise);
    QCOMPARE(problems.first().blockIndex, 0);
    QCOMPARE(problems.first().stepIndex, 1);
    QCOMPARE(countRows(t_.db, "workout"), 0);

    int id = 0;
    QVERIFY(store.create(sample("Fine"), &id) == StoreStatus::Ok);
    QVERIFY(store.update(id, w) == StoreStatus::Invalid);
    Workout read;
    QVERIFY(store.get(id, &read));
    QVERIFY(sameWorkout(read, sample("Fine")));
}

void TstWorkoutStore::removeDeletesTheTreeOnly()
{
    WorkoutStore store(&t_.db);
    int id = 0, other = 0;
    QVERIFY(store.create(sample("One"), &id) == StoreStatus::Ok);
    QVERIFY(store.create(sample("Two"), &other) == StoreStatus::Ok);
    QVERIFY(store.remove(id) == StoreStatus::Ok);
    QCOMPARE(countRows(t_.db, "workout"), 1);
    QCOMPARE(countRows(t_.db, "block"), 3);
    QCOMPARE(countRows(t_.db, "exercise"), 3);
    Workout read;
    QVERIFY(!store.get(id, &read));
    QVERIFY(store.get(other, &read));
    QVERIFY(integrityOk(t_.db));
}

void TstWorkoutStore::unknownIdIsNotFound()
{
    WorkoutStore store(&t_.db);
    QVERIFY(store.update(42, sample("X")) == StoreStatus::NotFound);
    QVERIFY(store.remove(42) == StoreStatus::NotFound);
    QVERIFY(store.duplicate(42) == StoreStatus::NotFound);
    QVERIFY(store.setFavorite(42, true) == StoreStatus::NotFound);
    Workout w;
    QVERIFY(!store.get(42, &w));
}

void TstWorkoutStore::volume()
{
    WorkoutStore store(&t_.db);
    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i < 200; ++i) {
        QList<Block> blocks;
        for (int b = 0; b < 5; ++b) {
            QList<Step> steps;
            for (int s = 0; s < 10; ++s)
                steps.append(s % 3 == 2 ? rest(10) : (s % 3 == 1 ? repStep(b_, 10) : work(a_, 30 + s)));
            blocks.append(block(steps, 2, 5));
        }
        Workout w = workout(blocks, 30);
        w.name = QStringLiteral("Volume %1").arg(i, 3, 10, QLatin1Char('0'));
        QVERIFY(store.create(w) == StoreStatus::Ok);
    }
    const qint64 writeMs = timer.elapsed();

    timer.restart();
    const QList<WorkoutSummary> summaries = store.list();
    const qint64 listMs = timer.elapsed();
    QCOMPARE(summaries.size(), 200);
    QCOMPARE(countRows(t_.db, "step"), 200 * 5 * 10);
    QVERIFY(summaries.first().plannedSeconds > 0);
    QVERIFY(integrityOk(t_.db));
    qInfo("volume: 200 workouts (10000 steps) written in %lld ms, listed in %lld ms", writeMs, listMs);
}

void TstWorkoutStore::iconAndColourRoundTrip()
{
    WorkoutStore store(&t_.db);
    Workout w = sample("Keys");
    w.icon = QStringLiteral("run-fast");
    w.colour = QStringLiteral("teal");
    int id = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    Workout read;
    QVERIFY(store.get(id, &read));
    QCOMPARE(read.icon, QStringLiteral("run-fast"));
    QCOMPARE(read.colour, QStringLiteral("teal"));
    QVERIFY(sameWorkout(read, w));

    w.icon = QStringLiteral("swim");
    w.colour = QStringLiteral("blue");
    QVERIFY(store.update(id, w) == StoreStatus::Ok);
    QVERIFY(store.get(id, &read));
    QCOMPARE(read.icon, QStringLiteral("swim"));
    QCOMPARE(read.colour, QStringLiteral("blue"));
    w.icon.clear();
    w.colour.clear();
    QVERIFY(store.update(id, w) == StoreStatus::Ok);
    QVERIFY(store.get(id, &read));
    QCOMPARE(read.icon, QString());
    QCOMPARE(read.colour, QString());
}

void TstWorkoutStore::defaultsAreEmptyKeys()
{
    WorkoutStore store(&t_.db);
    int id = 0;
    QVERIFY(store.create(sample("Plain"), &id) == StoreStatus::Ok);
    QSqlQuery q;
    QVERIFY(t_.db.run("SELECT icon, colour FROM workout", QVariantList(), &q));
    QVERIFY(q.next());
    QVERIFY(!q.value(0).isNull());
    QVERIFY(!q.value(1).isNull());
    QCOMPARE(q.value(0).toString(), QString());
    QCOMPARE(q.value(1).toString(), QString());
}

void TstWorkoutStore::unknownStoredColourAndIconAreKept()
{
    WorkoutStore store(&t_.db);
    int id = 0;
    QVERIFY(store.create(sample("Later"), &id) == StoreStatus::Ok);
    QVERIFY(t_.db.run("UPDATE workout SET icon = 'future-icon', colour = 'mauve' WHERE id = ?", QVariantList() << id));
    Workout read;
    QVERIFY(store.get(id, &read));
    QCOMPARE(read.icon, QStringLiteral("future-icon"));
    QCOMPARE(read.colour, QStringLiteral("mauve"));
    const QList<WorkoutSummary> list = store.list();
    QCOMPARE(list.first().icon, QStringLiteral("future-icon"));
    QCOMPARE(list.first().colour, QStringLiteral("mauve"));
}

void TstWorkoutStore::invalidIconAndColourAreRefusedOnWrite()
{
    WorkoutStore store(&t_.db);
    Workout w = sample("Bad");
    w.colour = QStringLiteral("mauve");
    QList<Problem> problems;
    QVERIFY(store.create(w, nullptr, &problems) == StoreStatus::Invalid);
    QCOMPARE(problems.size(), 1);
    QCOMPARE(problems.first().code, Problem::ColourUnknown);

    w.colour.clear();
    for (const QString &icon : { QStringLiteral("Run"), QStringLiteral("run fast"), QStringLiteral("run_fast"),
                                 QString(Limits::maxIconKeyChars + 1, QLatin1Char('a')) }) {
        w.icon = icon;
        problems.clear();
        QVERIFY(store.create(w, nullptr, &problems) == StoreStatus::Invalid);
        QCOMPARE(problems.first().code, Problem::IconInvalid);
    }
    // Any key of the right form is accepted, known or not, up to the length limit.
    w.icon = QString(Limits::maxIconKeyChars, QLatin1Char('a'));
    QVERIFY(store.create(w) == StoreStatus::Ok);
    QCOMPARE(countRows(t_.db, "workout"), 1);
}

void TstWorkoutStore::listCarriesIconAndColour()
{
    WorkoutStore store(&t_.db);
    Workout w = sample("Listed");
    w.icon = QStringLiteral("bike");
    w.colour = QStringLiteral("green");
    QVERIFY(store.create(w) == StoreStatus::Ok);
    const QList<WorkoutSummary> list = store.list();
    QCOMPARE(list.size(), 1);
    QCOMPARE(list.first().icon, QStringLiteral("bike"));
    QCOMPARE(list.first().colour, QStringLiteral("green"));
}

void TstWorkoutStore::equalIdentityIsRefused()
{
    WorkoutStore store(&t_.db);
    Workout w = sample("Same");
    w.icon = QStringLiteral("bike");
    w.colour = QStringLiteral("green");
    QVERIFY(store.create(w) == StoreStatus::Ok);
    QList<Problem> problems;
    QVERIFY(store.create(w, nullptr, &problems) == StoreStatus::Invalid);
    QCOMPARE(problems.size(), 1);
    QCOMPARE(problems.first().code, Problem::IdentityTaken);
    QCOMPARE(countRows(t_.db, "workout"), 1);
}

void TstWorkoutStore::sameNameWithAnotherColourIsAccepted()
{
    WorkoutStore store(&t_.db);
    Workout w = sample("Same");
    w.colour = QStringLiteral("green");
    QVERIFY(store.create(w) == StoreStatus::Ok);
    w.colour = QStringLiteral("red");
    QVERIFY(store.create(w) == StoreStatus::Ok);
    w.colour.clear();
    QVERIFY(store.create(w) == StoreStatus::Ok);
    QCOMPARE(countRows(t_.db, "workout"), 3);
}

void TstWorkoutStore::sameNameWithAnotherIconIsAccepted()
{
    WorkoutStore store(&t_.db);
    Workout w = sample("Same");
    w.icon = QStringLiteral("bike");
    QVERIFY(store.create(w) == StoreStatus::Ok);
    w.icon = QStringLiteral("swim");
    QVERIFY(store.create(w) == StoreStatus::Ok);
    w.icon.clear();
    QVERIFY(store.create(w) == StoreStatus::Ok);
    QCOMPARE(countRows(t_.db, "workout"), 3);
}

void TstWorkoutStore::caseAndBlankSpaceVariantsAreRefused()
{
    WorkoutStore store(&t_.db);
    QVERIFY(store.create(sample("Morning Run")) == StoreStatus::Ok);
    const QString variants[] = { QStringLiteral("morning run"), QStringLiteral("MORNING RUN"),
                                 QStringLiteral("  Morning Run"), QStringLiteral("Morning Run \t\n"),
                                 QStringLiteral(" mORNING rUN ") };
    for (const QString &name : variants) {
        QList<Problem> problems;
        QVERIFY2(store.create(sample(name), nullptr, &problems) == StoreStatus::Invalid, qPrintable(name));
        QCOMPARE(problems.first().code, Problem::IdentityTaken);
    }
    // Blank space inside the name still counts.
    QVERIFY(store.create(sample("Morning  Run")) == StoreStatus::Ok);
    QCOMPARE(countRows(t_.db, "workout"), 2);
}

void TstWorkoutStore::emptyKeysEqualEachOther()
{
    WorkoutStore store(&t_.db);
    QVERIFY(store.create(sample("Plain")) == StoreStatus::Ok);
    QList<Problem> problems;
    QVERIFY(store.create(sample("plain"), nullptr, &problems) == StoreStatus::Invalid);
    QCOMPARE(problems.first().code, Problem::IdentityTaken);
}

void TstWorkoutStore::editingWithoutChangingTheIdentityIsAccepted()
{
    WorkoutStore store(&t_.db);
    int id = 0;
    Workout w = sample("Mine");
    w.colour = QStringLiteral("blue");
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    QVERIFY(store.update(id, w) == StoreStatus::Ok);
    w.restBetweenBlocksSec = 99;
    QVERIFY(store.update(id, w) == StoreStatus::Ok);
    // A change of case alone keeps the compared identity; the record is not compared with itself.
    w.name = QStringLiteral("MINE");
    QVERIFY(store.update(id, w) == StoreStatus::Ok);
    Workout read;
    QVERIFY(store.get(id, &read));
    QCOMPARE(read.name, QStringLiteral("MINE"));
    QCOMPARE(read.restBetweenBlocksSec, 99);
}

void TstWorkoutStore::changingIntoAClashIsRefused()
{
    WorkoutStore store(&t_.db);
    Workout a = sample("Alpha");
    a.colour = QStringLiteral("red");
    Workout b = sample("Alpha");
    b.colour = QStringLiteral("blue");
    int idB = 0;
    QVERIFY(store.create(a) == StoreStatus::Ok);
    QVERIFY(store.create(b, &idB) == StoreStatus::Ok);
    b.colour = QStringLiteral("red");
    QList<Problem> problems;
    QVERIFY(store.update(idB, b, &problems) == StoreStatus::Invalid);
    QCOMPARE(problems.first().code, Problem::IdentityTaken);
    Workout read;
    QVERIFY(store.get(idB, &read));
    QCOMPARE(read.colour, QStringLiteral("blue"));
}

void TstWorkoutStore::renameIconAndColourChangesAreChecked()
{
    WorkoutStore store(&t_.db);
    Workout a = sample("Alpha");
    Workout b = sample("Beta");
    int idB = 0;
    QVERIFY(store.create(a) == StoreStatus::Ok);
    QVERIFY(store.create(b, &idB) == StoreStatus::Ok);
    // Rename into the name of the other one.
    b.name = QStringLiteral(" alpha ");
    QVERIFY(store.update(idB, b) == StoreStatus::Invalid);
    // The icon and the colour then make it distinct.
    b.icon = QStringLiteral("bike");
    QVERIFY(store.update(idB, b) == StoreStatus::Ok);
    b.icon.clear();
    QVERIFY(store.update(idB, b) == StoreStatus::Invalid);
    b.colour = QStringLiteral("pink");
    QVERIFY(store.update(idB, b) == StoreStatus::Ok);
}

void TstWorkoutStore::anEditKeepsWorkingWhenOldDataAlreadyClashes()
{
    WorkoutStore store(&t_.db);
    int idA = 0, idB = 0;
    QVERIFY(store.create(sample("Twin"), &idA) == StoreStatus::Ok);
    QVERIFY(store.create(sample("Other"), &idB) == StoreStatus::Ok);
    // Rows written before the rule can be equal.
    QVERIFY(t_.db.run("UPDATE workout SET name = 'Twin' WHERE id = ?", QVariantList() << idB));
    Workout w;
    QVERIFY(store.get(idB, &w));
    w.restBetweenBlocksSec = 7;
    QVERIFY(store.update(idB, w) == StoreStatus::Ok);
    QVERIFY(store.get(idB, &w));
    QCOMPARE(w.restBetweenBlocksSec, 7);
}

void TstWorkoutStore::refusedSaveWritesNothing()
{
    WorkoutStore store(&t_.db);
    int id = 0;
    QVERIFY(store.create(sample("One"), &id) == StoreStatus::Ok);
    QVERIFY(store.create(sample("Two")) == StoreStatus::Ok);
    const int blocks = countRows(t_.db, "block");
    const int steps = countRows(t_.db, "step");
    Workout w = sample("two");
    w.blocks.removeLast();
    QVERIFY(store.update(id, w) == StoreStatus::Invalid);
    QVERIFY(store.create(sample("TWO")) == StoreStatus::Invalid);
    QCOMPARE(countRows(t_.db, "block"), blocks);
    QCOMPARE(countRows(t_.db, "step"), steps);
    QCOMPARE(countRows(t_.db, "workout"), 2);
    Workout read;
    QVERIFY(store.get(id, &read));
    QCOMPARE(read.name, QStringLiteral("One"));
    QCOMPARE(read.blocks.size(), 3);
}

void TstWorkoutStore::duplicateKeepsIconAndColourAndAlwaysPasses()
{
    WorkoutStore store(&t_.db);
    Workout w = sample("Base");
    w.icon = QStringLiteral("bike");
    w.colour = QStringLiteral("green");
    int id = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    for (int n = 0; n < 3; ++n) {
        int copyId = 0;
        QVERIFY(store.duplicate(id, &copyId) == StoreStatus::Ok);
        Workout copy;
        QVERIFY(store.get(copyId, &copy));
        QCOMPARE(copy.icon, QStringLiteral("bike"));
        QCOMPARE(copy.colour, QStringLiteral("green"));
        QCOMPARE(copy.name, n == 0 ? QStringLiteral("Base (copy)") : QStringLiteral("Base (copy %1)").arg(n + 1));
    }
    // A copy of a copy does not clash either.
    int copyId = 0;
    QVERIFY(store.duplicate(id, &copyId) == StoreStatus::Ok);
    QVERIFY(store.duplicate(copyId) == StoreStatus::Ok);
    QCOMPARE(countRows(t_.db, "workout"), 6);
}

void TstWorkoutStore::duplicateSkipsSuffixesThatClashOnTheTriple()
{
    WorkoutStore store(&t_.db);
    Workout w = sample("Base");
    w.colour = QStringLiteral("green");
    int id = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    // "Base (copy)" in another colour does not clash with the copy of the green one.
    Workout other = sample("Base (copy)");
    other.colour = QStringLiteral("red");
    QVERIFY(store.create(other) == StoreStatus::Ok);
    // "Base (copy)" in the same colour does.
    Workout same = sample("base (COPY) ");
    same.colour = QStringLiteral("green");
    QVERIFY(store.create(same) == StoreStatus::Ok);
    int copyId = 0;
    QVERIFY(store.duplicate(id, &copyId) == StoreStatus::Ok);
    Workout copy;
    QVERIFY(store.get(copyId, &copy));
    QCOMPARE(copy.name, QStringLiteral("Base (copy 2)"));
}

void TstWorkoutStore::problemsOfReportsTheIdentityWithoutWriting()
{
    WorkoutStore store(&t_.db);
    Workout w = sample("Same");
    w.colour = QStringLiteral("green");
    QVERIFY(store.create(w) == StoreStatus::Ok);
    QVERIFY(store.problemsOf(w).first().code == Problem::IdentityTaken);
    w.name = QStringLiteral("  SAME ");
    QCOMPARE(store.problemsOf(w).first().code, Problem::IdentityTaken);
    w.colour = QStringLiteral("blue");
    QVERIFY(store.problemsOf(w).isEmpty());
    QCOMPARE(countRows(t_.db, "workout"), 1);
}

void TstWorkoutStore::problemsOfSkipsTheComparisonForAnUnchangedIdentity()
{
    WorkoutStore store(&t_.db);
    int id = 0;
    QVERIFY(store.create(sample("One"), &id) == StoreStatus::Ok);
    Workout w;
    QVERIFY(store.get(id, &w));
    bool failed = true;
    QVERIFY(store.problemsOf(w, id, &failed).isEmpty());
    QVERIFY(!failed);
    QCOMPARE(store.problemsOf(w).first().code, Problem::IdentityTaken);
}

void TstWorkoutStore::summariesCountBlocksAndSteps()
{
    WorkoutStore store(&t_.db);
    Workout w = sample("Counted");
    QVERIFY(store.create(w) == StoreStatus::Ok);
    const QList<WorkoutSummary> list = store.list();
    QCOMPARE(list.size(), 1);
    QCOMPARE(list.first().blockCount, 3);
    QCOMPARE(list.first().stepCount, 8);
}

void TstWorkoutStore::stepPaceRoundTrips()
{
    WorkoutStore store(&t_.db);
    const QString a = addExercise(t_.db, "A");
    Workout w = workout(QList<Block>() << block(QList<Step>()
        << pacedStep(a, 10, PaceKind::Total, 30000) << pacedStep(a, 6, PaceKind::PerRep, 2500) << repStep(a, 4)
        << work(a, 20)));
    w.name = "Paced";
    int id = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    Workout read;
    QVERIFY(store.get(id, &read));
    const QList<Step> steps = read.blocks.first().steps;
    QVERIFY(steps.at(0).pace == PaceKind::Total);
    QCOMPARE(steps.at(0).paceMs, 30000);
    QVERIFY(steps.at(1).pace == PaceKind::PerRep);
    QCOMPARE(steps.at(1).paceMs, 2500);
    QVERIFY(steps.at(2).pace == PaceKind::None);
    QCOMPARE(steps.at(2).paceMs, 0);
    QVERIFY(steps.at(3).pace == PaceKind::None);
    // The plan and the list carry the paces: 30 s + 15 s + 20 s counted, the open step left out.
    const QList<WorkoutSummary> list = store.list();
    QCOMPARE(list.first().plannedSeconds, 30 + 15 + 20);
    QVERIFY(list.first().hasRepSteps);
    QVERIFY(list.first().hasTempoSteps);
    const QList<PlanStep> plan = store.plan(read);
    QVERIFY(plan.at(0).hasTempo && plan.at(0).pace == PaceKind::Total);
    // An edit can remove the pace.
    read.blocks[0].steps[0].pace = PaceKind::None;
    read.blocks[0].steps[0].paceMs = 0;
    QVERIFY(store.update(id, read) == StoreStatus::Ok);
    Workout again;
    QVERIFY(store.get(id, &again));
    QVERIFY(again.blocks.first().steps.at(0).pace == PaceKind::None);
    QVERIFY(integrityOk(t_.db));
}

void TstWorkoutStore::aPaceBelowTheLimitIsRefusedAndNothingIsWritten()
{
    WorkoutStore store(&t_.db);
    const QString a = addExercise(t_.db, "A");
    Workout w = workout(QList<Block>() << block(QList<Step>() << pacedStep(a, 10, PaceKind::Total, 4000)));
    w.name = "Too fast";
    QList<Problem> problems;
    QVERIFY(store.create(w, nullptr, &problems) == StoreStatus::Invalid);
    QCOMPARE(problems.size(), 1);
    QVERIFY(problems.first().code == Problem::PaceRepTooShort);
    QCOMPARE(countRows(t_.db, "workout"), 0);
    w.blocks[0].steps[0].paceMs = 5000;
    QVERIFY(store.create(w) == StoreStatus::Ok);
}

void TstWorkoutStore::aStoredPaceIsIgnoredWhileTheExerciseHasARhythm()
{
    WorkoutStore store(&t_.db);
    const QString a = addExercise(t_.db, "A");
    Workout w = workout(QList<Block>() << block(QList<Step>() << pacedStep(a, 4, PaceKind::PerRep, 2000)));
    w.name = "W";
    int id = 0;
    QVERIFY(store.create(w, &id) == StoreStatus::Ok);
    QCOMPARE(store.list().first().plannedSeconds, 8);

    // The exercise gets a rhythm of 3 s: the step runs it and the stored pace stays unused.
    const int rhythm = addMetronome(t_.db, "R", phasesOf({ 2, 4 }, 1));
    QVERIFY(ExerciseStore(&t_.db).setMetronome(a, rhythm) == StoreStatus::Ok);
    QCOMPARE(store.list().first().plannedSeconds, 12);
    Workout read;
    QVERIFY(store.get(id, &read));
    QVERIFY(read.blocks.first().steps.first().pace == PaceKind::PerRep);
    QVERIFY(store.plan(read).first().pace == PaceKind::None);
    // A stored pace that would be invalid is not checked while the rhythm exists.
    read.blocks[0].steps[0].paceMs = 100;
    QVERIFY(store.update(id, read) == StoreStatus::Ok);
    // Detaching the rhythm brings the pace back, and then it is checked again.
    QVERIFY(ExerciseStore(&t_.db).clearMetronome(a) == StoreStatus::Ok);
    QVERIFY(store.update(id, read) == StoreStatus::Invalid);
}

QTEST_APPLESS_MAIN(TstWorkoutStore)

#include "tst_workoutstore.moc"

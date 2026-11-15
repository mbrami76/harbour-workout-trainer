// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "fakeclock.h"
#include "sessionlog.h"
#include "testdb.h"

class TstSessionLog : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void repeatedStepsAreMerged();
    void wideMuscleSetsAreReadBackWhole();
    void zeroTimeEntriesAreDropped();
    void roundingToWholeSeconds();
    void totalIsRoundedSumOfMilliseconds();
    void namesAndMusclesAreCopied();
    void unknownExerciseGivesEmptyName();
    void plannedValuesAreKept();
    void outOfRangeRecordsAreIgnored();
    void storedEntryReadsBack();
    void endStatesAreStored();
    void newestComesFirst();
    void entryStaysAfterExerciseChanges();
    void builtFromEngineRecords();
    void failedAddWritesNothing();
    void stopInFirstLeadInIsNotLoggable();
    void lastCompletedEndIgnoresStoppedSessions();
    void lastCompletedEndIsFalseWithoutOne();

private:
    ExerciseLookup lookupFor(const QMap<QString, Exercise> &exercises) const;
    TestDb t_;
};

void TstSessionLog::init()
{
    t_.db.close();
    QDir(t_.dir.path()).removeRecursively();
    QVERIFY(t_.open());
}

ExerciseLookup TstSessionLog::lookupFor(const QMap<QString, Exercise> &exercises) const
{
    return [exercises](const QString &key, QString *name, MuscleSet *muscles) {
        if (!exercises.contains(key))
            return false;
        *name = exercises.value(key).name;
        *muscles = exercises.value(key).muscles;
        return true;
    };
}

static QList<PlanStep> planOf(const QList<Step> &steps)
{
    return expandPlan(workout(QList<Block>() << block(steps)));
}

static StepRecord record(int index, qint64 ms)
{
    StepRecord r;
    r.stepIndex = index;
    r.activeMs = ms;
    return r;
}

static QMap<QString, Exercise> library()
{
    QMap<QString, Exercise> m;
    Exercise a = makeExercise("Squat", muscleBit(MuscleGroup::Quadriceps) | muscleBit(MuscleGroup::Glutes));
    a.key = "a";
    Exercise b = makeExercise("Push-up", muscleBit(MuscleGroup::Chest));
    b.key = "b";
    m.insert("a", a);
    m.insert("b", b);
    return m;
}

void TstSessionLog::repeatedStepsAreMerged()
{
    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 30) << rest(10) << work("b", 40));
    // Step 0 is entered, left and entered again: the entries of one step are added up.
    const QList<StepRecord> records = QList<StepRecord>()
        << record(0, 1200) << record(1, 3000) << record(0, 2300) << record(2, 6000);
    const LogEntry e = buildLogEntry("Merged", 1000, plan, records, EndState::Completed, lookupFor(library()));
    QCOMPARE(e.steps.size(), 3);
    QCOMPARE(e.steps.at(0).actualSec, 4);
    QCOMPARE(e.steps.at(1).actualSec, 3);
    QCOMPARE(e.steps.at(2).actualSec, 6);
    QCOMPARE(e.steps.at(0).position, 0);
    QCOMPARE(e.steps.at(2).position, 2);
    QCOMPARE(e.totalSec, 13);
}

void TstSessionLog::zeroTimeEntriesAreDropped()
{
    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 30) << rest(10) << work("b", 40));
    const QList<StepRecord> records = QList<StepRecord>() << record(0, 0) << record(1, 4000) << record(2, 0) << record(2, 0);
    const LogEntry e = buildLogEntry("Dropped", 0, plan, records, EndState::Stopped, lookupFor(library()));
    QCOMPARE(e.steps.size(), 1);
    QVERIFY(e.steps.first().type == StepType::Rest);
    QCOMPARE(e.steps.first().position, 0);

    // The first entry decides the order even when its own time is zero.
    const QList<StepRecord> reordered = QList<StepRecord>() << record(2, 0) << record(1, 2000) << record(2, 3000);
    const LogEntry r = buildLogEntry("Order", 0, plan, reordered, EndState::Stopped, lookupFor(library()));
    QCOMPARE(r.steps.size(), 2);
    QVERIFY(r.steps.at(0).type == StepType::Work);
    QVERIFY(r.steps.at(1).type == StepType::Rest);
}

void TstSessionLog::roundingToWholeSeconds()
{
    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 30) << work("a", 30) << work("a", 30) << work("a", 30));
    const QList<StepRecord> records = QList<StepRecord>()
        << record(0, 499) << record(1, 500) << record(2, 1499) << record(3, 1500);
    const LogEntry e = buildLogEntry("Rounding", 0, plan, records, EndState::Completed, lookupFor(library()));
    QCOMPARE(e.steps.size(), 4);
    QCOMPARE(e.steps.at(0).actualSec, 0);
    QCOMPARE(e.steps.at(1).actualSec, 1);
    QCOMPARE(e.steps.at(2).actualSec, 1);
    QCOMPARE(e.steps.at(3).actualSec, 2);
}

void TstSessionLog::totalIsRoundedSumOfMilliseconds()
{
    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 30) << work("a", 30) << work("a", 30));
    const QList<StepRecord> records = QList<StepRecord>() << record(0, 400) << record(1, 400) << record(2, 400);
    const LogEntry e = buildLogEntry("Total", 0, plan, records, EndState::Completed, lookupFor(library()));
    QCOMPARE(e.steps.at(0).actualSec + e.steps.at(1).actualSec + e.steps.at(2).actualSec, 0);
    QCOMPARE(e.totalSec, 1);
}

void TstSessionLog::namesAndMusclesAreCopied()
{
    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 30) << rest(5) << work("b", 40));
    const QList<StepRecord> records = QList<StepRecord>() << record(0, 30000) << record(1, 5000) << record(2, 40000);
    const LogEntry e = buildLogEntry("Copied", 0, plan, records, EndState::Completed, lookupFor(library()));
    QCOMPARE(e.steps.at(0).exerciseName, QStringLiteral("Squat"));
    QCOMPARE(e.steps.at(0).muscles, muscleBit(MuscleGroup::Quadriceps) | muscleBit(MuscleGroup::Glutes));
    QVERIFY(e.steps.at(1).exerciseName.isEmpty());
    QCOMPARE(e.steps.at(1).muscles, MuscleSet(0));
    QCOMPARE(e.steps.at(2).exerciseName, QStringLiteral("Push-up"));
}

void TstSessionLog::unknownExerciseGivesEmptyName()
{
    const QList<PlanStep> plan = planOf(QList<Step>() << work("gone", 30));
    const LogEntry e = buildLogEntry("Gone", 0, plan, QList<StepRecord>() << record(0, 5000), EndState::Stopped,
                                     lookupFor(library()));
    QCOMPARE(e.steps.size(), 1);
    QVERIFY(e.steps.first().exerciseName.isEmpty());
    QCOMPARE(e.steps.first().muscles, MuscleSet(0));
}

void TstSessionLog::plannedValuesAreKept()
{
    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 30) << repStep("b", 12));
    const LogEntry e = buildLogEntry("Planned", 0, plan, QList<StepRecord>() << record(0, 30000) << record(1, 20000),
                                     EndState::Completed, lookupFor(library()));
    QCOMPARE(e.steps.at(0).plannedSec, 30);
    QCOMPARE(e.steps.at(0).plannedReps, 0);
    QCOMPARE(e.steps.at(1).plannedSec, 0);
    QCOMPARE(e.steps.at(1).plannedReps, 12);
}

void TstSessionLog::outOfRangeRecordsAreIgnored()
{
    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 30));
    const LogEntry e = buildLogEntry("Range", 0, plan, QList<StepRecord>() << record(-1, 1000) << record(5, 1000) << record(0, 2000),
                                     EndState::Completed, lookupFor(library()));
    QCOMPARE(e.steps.size(), 1);
    QCOMPARE(e.totalSec, 2);
}

void TstSessionLog::storedEntryReadsBack()
{
    SessionLog log(&t_.db);
    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 30) << rest(10) << repStep("b", 12));
    const LogEntry built = buildLogEntry("Evening", 1793000000123LL, plan,
        QList<StepRecord>() << record(0, 30000) << record(1, 10000) << record(2, 25400),
        EndState::Completed, lookupFor(library()));
    int id = 0;
    QVERIFY2(log.add(built, &id) == StoreStatus::Ok, qPrintable(t_.db.lastError()));
    QVERIFY(id > 0);

    const QList<LogEntry> read = log.list();
    QCOMPARE(read.size(), 1);
    const LogEntry &e = read.first();
    QCOMPARE(e.id, id);
    QCOMPARE(e.workoutName, QStringLiteral("Evening"));
    QCOMPARE(e.startedAtMs, qint64(1793000000123LL));
    QCOMPARE(e.totalSec, 65);
    QCOMPARE(e.steps.size(), 3);
    QCOMPARE(e.steps.at(0).exerciseName, QStringLiteral("Squat"));
    QCOMPARE(e.steps.at(0).muscles, muscleBit(MuscleGroup::Quadriceps) | muscleBit(MuscleGroup::Glutes));
    QCOMPARE(e.steps.at(0).plannedSec, 30);
    QCOMPARE(e.steps.at(0).actualSec, 30);
    QVERIFY(e.steps.at(1).type == StepType::Rest);
    QVERIFY(e.steps.at(1).exerciseName.isEmpty());
    QCOMPARE(e.steps.at(2).plannedReps, 12);
    QCOMPARE(e.steps.at(2).plannedSec, 0);
    QCOMPARE(e.steps.at(2).actualSec, 25);
    QVERIFY(integrityOk(t_.db));
}

void TstSessionLog::endStatesAreStored()
{
    SessionLog log(&t_.db);
    LogEntry done;
    done.workoutName = "Done";
    done.endState = EndState::Completed;
    done.startedAtMs = 1;
    LogEntry stopped = done;
    stopped.workoutName = "Stopped";
    stopped.endState = EndState::Stopped;
    stopped.startedAtMs = 2;
    QVERIFY(log.add(done) == StoreStatus::Ok);
    QVERIFY(log.add(stopped) == StoreStatus::Ok);
    const QList<LogEntry> read = log.list();
    QVERIFY(read.at(0).endState == EndState::Stopped);
    QVERIFY(read.at(1).endState == EndState::Completed);
}

void TstSessionLog::newestComesFirst()
{
    SessionLog log(&t_.db);
    const qint64 starts[] = { 5000, 9000, 1000, 9000 };
    for (int i = 0; i < 4; ++i) {
        LogEntry e;
        e.workoutName = QStringLiteral("S%1").arg(i);
        e.startedAtMs = starts[i];
        QVERIFY(log.add(e) == StoreStatus::Ok);
    }
    QStringList names;
    for (const LogEntry &e : log.list())
        names << e.workoutName;
    QCOMPARE(names, QStringList() << "S3" << "S1" << "S0" << "S2");
    QCOMPARE(log.list(2).size(), 2);
    QCOMPARE(log.list(2).first().workoutName, QStringLiteral("S3"));
}

void TstSessionLog::entryStaysAfterExerciseChanges()
{
    ExerciseStore exercises(&t_.db);
    SessionLog log(&t_.db);
    Exercise e = makeExercise("Original name", muscleBit(MuscleGroup::Arms));
    QVERIFY(exercises.create(&e) == StoreStatus::Ok);

    const ExerciseLookup lookup = [&exercises](const QString &key, QString *name, MuscleSet *muscles) {
        Exercise found;
        if (!exercises.get(key, &found))
            return false;
        *name = found.name;
        *muscles = found.muscles;
        return true;
    };
    const QList<PlanStep> plan = planOf(QList<Step>() << work(e.key, 30));
    const LogEntry entry = buildLogEntry("Kept", 10, plan, QList<StepRecord>() << record(0, 30000),
                                         EndState::Completed, lookup);
    QVERIFY(log.add(entry) == StoreStatus::Ok);

    e.name = "Renamed";
    e.muscles = muscleBit(MuscleGroup::Calves);
    QVERIFY(exercises.update(e) == StoreStatus::Ok);
    QCOMPARE(log.list().first().steps.first().exerciseName, QStringLiteral("Original name"));
    QVERIFY(exercises.remove(e.key) == StoreStatus::Ok);
    QCOMPARE(log.list().first().steps.first().exerciseName, QStringLiteral("Original name"));
    QCOMPARE(log.list().first().steps.first().muscles, muscleBit(MuscleGroup::Arms));
}

void TstSessionLog::builtFromEngineRecords()
{
    FakeClock clock;
    EngineConfig config;
    config.leadInSec = 0;
    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 5) << rest(3) << work("b", 4));
    SessionEngine engine(&clock, plan, config);
    engine.start();
    clock.set(2000);
    engine.update();
    clock.set(14000);
    engine.update();
    QVERIFY(engine.state() == SessionState::Finished);

    const LogEntry e = buildLogEntry("Engine", 0, plan, engine.records(), EndState::Completed, lookupFor(library()));
    QCOMPARE(e.steps.size(), 3);
    QCOMPARE(e.steps.at(0).actualSec, 5);
    QCOMPARE(e.steps.at(1).actualSec, 3);
    QCOMPARE(e.steps.at(2).actualSec, 4);
    QCOMPARE(e.totalSec, 12);
}

void TstSessionLog::stopInFirstLeadInIsNotLoggable()
{
    FakeClock clock;
    EngineConfig config;
    config.leadInSec = 3;
    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 5) << rest(3));
    SessionEngine engine(&clock, plan, config);
    engine.start();
    clock.set(1500);
    engine.update();
    engine.stop();
    QVERIFY(engine.state() == SessionState::Stopped);
    const LogEntry lead = buildLogEntry("Lead", 0, plan, engine.records(), EndState::Stopped, lookupFor(library()));
    QVERIFY(!isLoggable(lead));

    SessionEngine later(&clock, plan, config);
    later.start();
    clock.set(1500 + 3000 + 2000);
    later.update();
    later.stop();
    const LogEntry ran = buildLogEntry("Ran", 0, plan, later.records(), EndState::Stopped, lookupFor(library()));
    QVERIFY(isLoggable(ran));
}

void TstSessionLog::failedAddWritesNothing()
{
    SessionLog log(&t_.db);
    QVERIFY(t_.db.run("CREATE TRIGGER fail_step BEFORE INSERT ON session_step WHEN NEW.position = 1 "
                      "BEGIN SELECT RAISE(ABORT, 'forced'); END"));
    LogEntry e;
    e.workoutName = "Broken";
    for (int i = 0; i < 3; ++i) {
        LoggedStep s;
        s.position = i;
        s.actualSec = 5;
        e.steps.append(s);
    }
    QVERIFY(log.add(e) == StoreStatus::Failed);
    QCOMPARE(countRows(t_.db, "session"), 0);
    QCOMPARE(countRows(t_.db, "session_step"), 0);
    QVERIFY(integrityOk(t_.db));
}

void TstSessionLog::wideMuscleSetsAreReadBackWhole()
{
    const MuscleSet wide = (MuscleSet(1) << 62) | (MuscleSet(1) << 33) | muscleBit(MuscleGroup::Calves);
    QVERIFY(t_.db.run("INSERT INTO session (workout_name, started_at, total_sec, end_state) VALUES ('W', 1, 5, 'completed')"));
    const qint64 id = t_.db.lastInsertId();
    QVERIFY(t_.db.run("INSERT INTO session_step (session_id, position, exercise_name, muscles, type, actual_sec) "
                      "VALUES (?, 0, 'X', ?, 'work', 5)", QVariantList() << id << wide));
    SessionLog log(&t_.db);
    const QList<LogEntry> read = log.list();
    QCOMPARE(read.size(), 1);
    QCOMPARE(read.first().steps.first().muscles, wide);
}

void TstSessionLog::lastCompletedEndIgnoresStoppedSessions()
{
    SessionLog log(&t_.db);
    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 30));
    const QList<StepRecord> records = QList<StepRecord>() << record(0, 30000);
    QVERIFY(log.add(buildLogEntry("Old", 1000000, plan, records, EndState::Completed, lookupFor(library()))) == StoreStatus::Ok);
    QVERIFY(log.add(buildLogEntry("Newer", 2000000, plan, records, EndState::Completed, lookupFor(library()))) == StoreStatus::Ok);
    QVERIFY(log.add(buildLogEntry("Stopped", 3000000, plan, records, EndState::Stopped, lookupFor(library()))) == StoreStatus::Ok);

    qint64 end = 0;
    QVERIFY(log.lastCompletedEndMs(&end));
    QCOMPARE(end, qint64(2000000 + 30000));
}

void TstSessionLog::lastCompletedEndIsFalseWithoutOne()
{
    SessionLog log(&t_.db);
    qint64 end = 7;
    QVERIFY(!log.lastCompletedEndMs(&end));

    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 30));
    QVERIFY(log.add(buildLogEntry("Stopped", 3000000, plan, QList<StepRecord>() << record(0, 10000),
                                  EndState::Stopped, lookupFor(library()))) == StoreStatus::Ok);
    QVERIFY(!log.lastCompletedEndMs(&end));
    QCOMPARE(end, qint64(7));
}

QTEST_APPLESS_MAIN(TstSessionLog)

#include "tst_sessionlog.moc"

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>
#include <QSqlRecord>
#include <random>

#include "colours.h"
#include "exchange.h"
#include "musclegroups.h"
#include "testdb.h"
#include "workoutlimits.h"
#include "workoutstore.h"

using namespace Exchange;

namespace {

bool has(const QList<ImportIssue> &list, ImportIssue::Code code, const QString &location)
{
    for (const ImportIssue &i : list) {
        if (i.code == code && i.location == location)
            return true;
    }
    return false;
}

int countCode(const QList<ImportIssue> &list, ImportIssue::Code code)
{
    int n = 0;
    for (const ImportIssue &i : list) {
        if (i.code == code)
            ++n;
    }
    return n;
}

// Text of the main tables, to compare the state of two points in time.
QString snapshot(Database &db)
{
    QString text;
    const char *queries[] = {
        "SELECT key, name, muscles, instructions, tips, starter, icon, colour, language FROM exercise ORDER BY key",
        "SELECT id, name, favorite, rest_between_blocks FROM workout ORDER BY id",
        "SELECT id, workout_id, position, rounds, rest_between_rounds FROM block ORDER BY id",
        "SELECT id, block_id, position, type, exercise_key, duration_sec, reps FROM step ORDER BY id",
        "SELECT kind, id FROM starter_offered ORDER BY kind, id"
    };
    for (const char *sql : queries) {
        QSqlQuery q;
        if (!db.run(QString::fromLatin1(sql), QVariantList(), &q))
            return QStringLiteral("error");
        while (q.next()) {
            for (int i = 0; i < q.record().count(); ++i)
                text += q.value(i).toString() + QLatin1Char('|');
            text += QLatin1Char('\n');
        }
        text += QStringLiteral("--\n");
    }
    return text;
}

// Text builders for hand-written files.
QByteArray doc(const QString &exercises, const QString &workouts, const QString &head = QString())
{
    const QString h = head.isEmpty() ? QStringLiteral("\"format\":\"workout-trainer-workouts\",\"version\":4") : head;
    return QStringLiteral("{%1,\"exercises\":[%2],\"workouts\":[%3]}").arg(h, exercises, workouts).toUtf8();
}

QString ex(const QString &key, const QString &name, const QString &muscles = QStringLiteral("\"chest\""),
           const QString &extra = QString())
{
    return QStringLiteral("{\"key\":\"%1\",\"name\":\"%2\",\"muscles\":[%3]%4}").arg(key, name, muscles, extra);
}

QString step(const QString &key, const QString &tail = QStringLiteral("\"durationSec\":30"))
{
    return QStringLiteral("{\"type\":\"work\",\"exercise\":\"%1\",%2}").arg(key, tail);
}

QString wk(const QString &name, const QString &steps, const QString &extra = QString())
{
    return QStringLiteral("{\"name\":\"%1\",\"blocks\":[{\"rounds\":1,\"steps\":[%2]}]%3}").arg(name, steps, extra);
}

ImportResult importNow(Database &db, const QByteArray &bytes, const ImportChoices &choices = ImportChoices())
{
    const ImportPreview p = preview(bytes, &db);
    return apply(p, choices, &db);
}

}

class TstExchange : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void exportOneWorkoutExactJson();
    void exportIncludesOnlyUsedExercises();
    void exportHasNoIdsFlagsOrStarterMark();
    void exportKeyOrderIsStable();
    void exportAllFollowsTheIds();
    void exportRejectsEmptyListAndUnknownId();
    void roundTripGivesIdenticalBytes();
    void importingTheSameFileTwice();
    void importedExercisesNeverCarryTheStarterFlag();
    void everyIssueHasADistinctCodeName();
    void workoutFileNamesAreSafeAndNeverTaken();

    void rejectsInvalidUtf8();
    void rejectsInvalidJson();
    void rejectsWrongFormat();
    void rejectsNewerVersion();
    void rejectsBadVersionValues();
    void rejectsFilesOverFourMiB();
    void acceptsAFileOfExactlyFourMiB();
    void rejectsTooManyWorkouts();
    void rejectsTooManyExercises();
    void rejectsWrongTypes();
    void rejectsMissingFields();
    void rejectsValuesOutsideTheLimits();
    void rejectsDuplicateExerciseKeys();
    void rejectsUnknownExerciseReferences();
    void rejectsInvalidIconKeys();
    void rejectsInvalidKeysAndNames();
    void rejectedFileChangesNothing();
    void unknownExtraFieldsAreIgnored();
    void namesAreTrimmed();

    void unknownColourIsDroppedWithNotice();
    void iconAndColourSurviveARoundTrip();
    void unknownButValidIconIsKept();
    void startModeIsWrittenOnlyWhenSet();
    void filesWithoutStartModeAreRead();
    void invalidStartModeIsAFileProblem();
    void stepsCarryNoMetronome();
    void exercisesCarryTheirMetronomeKey();
    void onlyTheMetronomesOfTheExportedExercisesTravel();
    void metronomeLinksRoundTrip();
    void phaseNamesTravelInTheFile();
    void aSharedMetronomeStaysShared();
    void sameKeyAndContentIsNotDuplicated();
    void sameKeyWithOtherContentIsAddedAsNew();
    void metronomeIdentityClashGetsTheSuffix();
    void rhythmIconInAFileIsIgnored();
    void rhythmsThatDifferOnlyByAnOldIconClash();
    void linkToAMetronomeOfTheLibraryIsAccepted();
    void anExerciseLinkToAnUnknownMetronomeIsAFileProblem();
    void invalidExerciseMetronomeKeysAreFileProblems();
    void onlyTheCurrentVersionIsRead();
    void invalidRhythmObjectsAreFileProblems();
    void aStepPaceTravelsInTheFile();
    void invalidStepPacesAreFileProblems();
    void aMissingMetronomeIsWrittenAsNoLink();
    void importedExerciseWithAnotherMetronomeIsAConflict();
    void sameExerciseWithTheSameMetronomeIsReused();
    void replaceTakesTheLinkOfTheFile();
    void importAsNewKeepsTheLink();
    void keepLeavesTheStoredLink();
    void aFileWithoutALinkDiffersFromALinkedExercise();
    void aFlaggedFirstStepIsReadAsAutomatic();
    void exportWritesTheNormalisedFirstStep();
    void languageIsWrittenAndRoundTrips();
    void missingLanguageIsEnglish();
    void malformedLanguageMakesTheFileInvalid();
    void unknownButWellFormedLanguageIsKept();
    void differentLanguageIsAConflict();
    void unknownGroupsAreDroppedWithNotice();
    void retiredKeysAreReadAsTheirReplacements();
    void newGroupKeysSurviveARoundTrip();
    void exportOfALegacyStoredSetUsesActiveKeys();
    void exerciseWithOnlyUnknownGroupsIsSkippedWithItsWorkouts();
    void fileWhereEverythingIsSkippedImportsNothing();
    void emptyFileHasNothingToImport();
    void sameNameWithFreeKeyIsAddedSeparately();

    void previewListsWhatWouldHappen();
    void previewListsWorkoutsUsingAConflictingExercise();
    void defaultChoiceOverwritesNothing();
    void choiceReplace();
    void choiceImportAsNew();
    void invalidChoicesAreRejectedBeforeWriting();
    void applyRejectsAnInvalidPreview();
    void applyRechecksTheLibrary();
    void workoutNamesInUseGetASuffix();
    void applyRollsBackCompletely();

    void writerReplacesTheFile();
    void writerLeavesTheExistingFileUntouched();

    void truncatedFilesAreRejected();
    void byteOrderMarkIsAccepted();
    void deepNestingDoesNotCrash();
    void hugeNumbersAreRejected();
    void emptyAndRandomInputAreRejected();
    void damagedFilesNeverCrash();


    void workoutIconAndColourAreExportedAsKeys();
    void emptyWorkoutKeysAreOmitted();
    void workoutIconAndColourRoundTrip();
    void oldFileWithoutKeysImportsWithDefaults();
    void unknownWorkoutColourIsDroppedWithNotice();
    void unknownWorkoutIconKeyIsKept();
    void invalidWorkoutIconKeyRejectsTheFile();
    void importResolvesAnIdentityClashWithTheSuffix();
    void importKeepsTheNameWhenIconOrColourDiffer();
    void importedCopiesOfOneFileGetDistinctNames();
    void importNeverRefusesOnAClash();
private:
    // Two exercises and two workouts, one of them a favorite, plus one unused exercise.
    void buildFixture(Database &db);
    TestDb t_;
};

void TstExchange::init()
{
    QVERIFY(t_.dir.isValid());
    t_.db.close();
    QDir dir(t_.dir.path());
    dir.removeRecursively();
    QVERIFY(t_.open());
}

void TstExchange::buildFixture(Database &db)
{
    ExerciseStore exercises(&db);
    Exercise push = makeExercise("Push-up", muscleBit(MuscleGroup::Chest) | muscleBit(MuscleGroup::Arms));
    push.key = QStringLiteral("fix-push");
    push.icon = QStringLiteral("push");
    push.colour = QStringLiteral("red");
    push.language = QStringLiteral("pt-br");
    QVERIFY(exercises.createKeyed(push) == StoreStatus::Ok);
    Exercise squat = makeExercise("Squat", muscleBit(MuscleGroup::Quadriceps) | muscleBit(MuscleGroup::Glutes));
    squat.key = QStringLiteral("fix-squat");
    QVERIFY(exercises.createKeyed(squat) == StoreStatus::Ok);
    Exercise unused = makeExercise("Burpee", muscleBit(MuscleGroup::Core));
    unused.key = QStringLiteral("fix-unused");
    QVERIFY(exercises.createKeyed(unused) == StoreStatus::Ok);

    WorkoutStore workouts(&db);
    Workout one = workout(QList<Block>() << block(QList<Step>() << work("fix-push", 40) << rest(15) << repStep("fix-squat", 12), 3, 20)
                                         << block(QList<Step>() << timedStep(StepType::Warmup, "fix-squat", 60)
                                                                << timedStep(StepType::Cooldown, "fix-push", 30)),
                          30);
    one.name = QStringLiteral("Morning");
    int id = 0;
    QVERIFY(workouts.create(one, &id) == StoreStatus::Ok);
    QVERIFY(workouts.setFavorite(id, true) == StoreStatus::Ok);
    Workout two = workout(QList<Block>() << block(QList<Step>() << work("fix-squat", 45), 2));
    two.name = QStringLiteral("Legs");
    QVERIFY(workouts.create(two) == StoreStatus::Ok);
}

void TstExchange::exportOneWorkoutExactJson()
{
    ExerciseStore exercises(&t_.db);
    Exercise push = makeExercise("Push-up", muscleBit(MuscleGroup::Chest) | muscleBit(MuscleGroup::Arms));
    push.key = QStringLiteral("fix-push");
    push.icon = QStringLiteral("push");
    push.colour = QStringLiteral("red");
    QVERIFY(exercises.createKeyed(push) == StoreStatus::Ok);
    WorkoutStore workouts(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << work("fix-push", 30) << rest(10), 2, 10));
    w.name = QStringLiteral("Fixture");
    int id = 0;
    QVERIFY(workouts.create(w, &id) == StoreStatus::Ok);

    QByteArray bytes;
    QVERIFY(exportWorkouts(&t_.db, QList<int>() << id, &bytes));
    const char expected[] =
        "{\n"
        "    \"exercises\": [\n"
        "        {\n"
        "            \"colour\": \"red\",\n"
        "            \"icon\": \"push\",\n"
        "            \"instructions\": \"Do it slowly.\",\n"
        "            \"key\": \"fix-push\",\n"
        "            \"language\": \"en\",\n"
        "            \"muscles\": [\n"
        "                \"upper-chest\",\n"
        "                \"lower-chest\",\n"
        "                \"biceps\",\n"
        "                \"triceps\",\n"
        "                \"forearms\"\n"
        "            ],\n"
        "            \"name\": \"Push-up\",\n"
        "            \"tips\": \"Keep the back straight.\"\n"
        "        }\n"
        "    ],\n"
        "    \"format\": \"workout-trainer-workouts\",\n"
        "    \"metronomes\": [\n"
        "    ],\n"
        "    \"version\": 4,\n"
        "    \"workouts\": [\n"
        "        {\n"
        "            \"blocks\": [\n"
        "                {\n"
        "                    \"restBetweenRoundsSec\": 10,\n"
        "                    \"rounds\": 2,\n"
        "                    \"steps\": [\n"
        "                        {\n"
        "                            \"durationSec\": 30,\n"
        "                            \"exercise\": \"fix-push\",\n"
        "                            \"type\": \"work\"\n"
        "                        },\n"
        "                        {\n"
        "                            \"durationSec\": 10,\n"
        "                            \"type\": \"rest\"\n"
        "                        }\n"
        "                    ]\n"
        "                }\n"
        "            ],\n"
        "            \"name\": \"Fixture\",\n"
        "            \"restBetweenBlocksSec\": 0\n"
        "        }\n"
        "    ]\n"
        "}\n";
    QCOMPARE(QString::fromUtf8(bytes), QString::fromLatin1(expected));
    QVERIFY(bytes.endsWith('\n'));
}

void TstExchange::exportIncludesOnlyUsedExercises()
{
    buildFixture(t_.db);
    WorkoutStore workouts(&t_.db);
    int legs = 0;
    for (const WorkoutSummary &s : workouts.list()) {
        if (s.name == QLatin1String("Legs"))
            legs = s.id;
    }
    QByteArray bytes;
    QVERIFY(exportWorkouts(&t_.db, QList<int>() << legs, &bytes));
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QCOMPARE(p.exercises.size(), 1);
    QCOMPARE(p.exercises.first().key, QStringLiteral("fix-squat"));
    QCOMPARE(p.exercises.first().instructions, QStringLiteral("Do it slowly."));
    QCOMPARE(p.exercises.first().tips, QStringLiteral("Keep the back straight."));
    QVERIFY(!bytes.contains("fix-unused"));
    QVERIFY(!bytes.contains("Burpee"));
}

void TstExchange::exportHasNoIdsFlagsOrStarterMark()
{
    buildFixture(t_.db);
    ExerciseStore exercises(&t_.db);
    Exercise starterEntry = makeExercise("Plank", muscleBit(MuscleGroup::Core));
    starterEntry.key = QStringLiteral("starter-plank");
    starterEntry.starterId = QStringLiteral("plank");
    QVERIFY(exercises.offerStarter(starterEntry) == StoreStatus::Ok);
    WorkoutStore workouts(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << work("starter-plank", 30)));
    QVERIFY(workouts.create(w) == StoreStatus::Ok);

    QByteArray bytes;
    QVERIFY(exportAll(&t_.db, &bytes));
    QVERIFY(!bytes.contains("\"id\""));
    QVERIFY(!bytes.contains("favorite"));
    QVERIFY(!bytes.contains("\"starter\""));
    QVERIFY(bytes.contains("\"key\": \"starter-plank\""));
}

void TstExchange::exportKeyOrderIsStable()
{
    buildFixture(t_.db);
    QByteArray first;
    QByteArray second;
    QVERIFY(exportAll(&t_.db, &first));
    QVERIFY(exportAll(&t_.db, &second));
    QCOMPARE(first, second);
    // Exercises in name order, whatever the order of use.
    const int burpee = first.indexOf("Push-up");
    const int squat = first.indexOf("Squat");
    QVERIFY(burpee > 0 && squat > burpee);
}

void TstExchange::exportAllFollowsTheIds()
{
    buildFixture(t_.db);
    QByteArray bytes;
    QVERIFY(exportAll(&t_.db, &bytes));
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QCOMPARE(p.workouts.size(), 2);
    QCOMPARE(p.workouts.at(0).name, QStringLiteral("Morning"));
    QCOMPARE(p.workouts.at(1).name, QStringLiteral("Legs"));

    WorkoutStore workouts(&t_.db);
    QList<int> ids;
    for (const WorkoutSummary &s : workouts.list())
        ids.prepend(s.id);   // summaries are favorites first; reverse puts Legs first
    QByteArray one;
    QVERIFY(exportWorkouts(&t_.db, ids, &one));
    const ImportPreview q = preview(one, &t_.db);
    QCOMPARE(q.workouts.size(), 2);
    QCOMPARE(q.workouts.at(0).name, QStringLiteral("Legs"));
}

void TstExchange::exportRejectsEmptyListAndUnknownId()
{
    buildFixture(t_.db);
    QByteArray bytes;
    QVERIFY(!exportWorkouts(&t_.db, QList<int>(), &bytes));
    QVERIFY(!exportWorkouts(&t_.db, QList<int>() << 9999, &bytes));
}

void TstExchange::roundTripGivesIdenticalBytes()
{
    buildFixture(t_.db);
    QByteArray before;
    QVERIFY(exportAll(&t_.db, &before));

    TestDb other;
    QVERIFY(other.open());
    const ImportResult result = importNow(other.db, before);
    QVERIFY(result.status == StoreStatus::Ok);
    QCOMPARE(result.workoutsAdded, 2);
    QCOMPARE(result.exercisesAdded, 2);

    QByteArray after;
    QVERIFY(exportAll(&other.db, &after));
    QCOMPARE(after, before);
    QVERIFY(integrityOk(other.db));

    // The contents survive: the first workout has the same tree.
    WorkoutStore a(&t_.db);
    WorkoutStore b(&other.db);
    Workout x;
    Workout y;
    for (const WorkoutSummary &s : a.list()) {
        QVERIFY(a.get(s.id, &x));
        bool found = false;
        for (const WorkoutSummary &o : b.list()) {
            if (o.name == s.name) {
                QVERIFY(b.get(o.id, &y));
                found = true;
            }
        }
        QVERIFY(found);
        QVERIFY(sameWorkout(x, y));
    }
}

void TstExchange::importingTheSameFileTwice()
{
    buildFixture(t_.db);
    QByteArray bytes;
    QVERIFY(exportAll(&t_.db, &bytes));

    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QCOMPARE(p.exercisesReused, 2);
    QCOMPARE(p.exercisesToAdd, 0);
    QVERIFY(p.conflicts.isEmpty());

    const ImportResult result = apply(p, ImportChoices(), &t_.db);
    QVERIFY(result.status == StoreStatus::Ok);
    QCOMPARE(result.workoutsAdded, 2);
    QCOMPARE(result.exercisesReused, 2);
    QCOMPARE(result.exercisesAdded, 0);
    QCOMPARE(result.workoutsRenamed, 2);
    QCOMPARE(countRows(t_.db, "exercise"), 3);
    QCOMPARE(countRows(t_.db, "workout"), 4);

    WorkoutStore workouts(&t_.db);
    QStringList names;
    for (const WorkoutSummary &s : workouts.list())
        names.append(s.name);
    QVERIFY(names.contains("Morning"));
    QVERIFY(names.contains("Morning (imported)"));
    QVERIFY(names.contains("Legs (imported)"));

    // A third time: the next free suffix.
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    names.clear();
    for (const WorkoutSummary &s : workouts.list())
        names.append(s.name);
    QVERIFY(names.contains("Morning (imported 2)"));
    QVERIFY(integrityOk(t_.db));
}

void TstExchange::workoutFileNamesAreSafeAndNeverTaken()
{
    QCOMPARE(workoutFileBase("Morning run"), QStringLiteral("Morning-run"));
    QCOMPARE(workoutFileBase("../../etc/passwd"), QStringLiteral("etcpasswd"));
    QCOMPARE(workoutFileBase(QString::fromUtf8("\xC3\x89t\xC3\xA9 \xE2\x9C\x93")), QStringLiteral("t"));
    QCOMPARE(workoutFileBase(QString::fromUtf8("\xE2\x9C\x93\xE2\x9C\x93")), QStringLiteral("workout"));
    QCOMPARE(workoutFileBase(QString(200, QLatin1Char('a'))).size(), 40);
    QCOMPARE(workoutFileBase("a   b  "), QStringLiteral("a-b"));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QCOMPARE(freeFileName(dir.path(), "plan"), QStringLiteral("plan.json"));
    QVERIFY(writeFile(inDir(dir, "plan.json"), "x"));
    QCOMPARE(freeFileName(dir.path(), "plan"), QStringLiteral("plan-2.json"));
    QVERIFY(writeFile(inDir(dir, "plan-2.json"), "x"));
    QCOMPARE(freeFileName(dir.path(), "plan"), QStringLiteral("plan-3.json"));
}

void TstExchange::everyIssueHasADistinctCodeName()
{
    QSet<QString> names;
    for (int c = int(ImportIssue::TooLarge); c <= int(ImportIssue::NothingToImport); ++c) {
        const QString name = codeName(ImportIssue::Code(c));
        QVERIFY2(name != QLatin1String("invalid"), qPrintable(QString::number(c)));
        QVERIFY2(!names.contains(name), qPrintable(name));
        names.insert(name);
    }
}

void TstExchange::importedExercisesNeverCarryTheStarterFlag()
{
    const QByteArray bytes = doc(ex("starter-fake", "Imported"), wk("W", step("starter-fake")));
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    ExerciseStore exercises(&t_.db);
    Exercise e;
    QVERIFY(exercises.get("starter-fake", &e));
    QVERIFY(!e.starter);
}

void TstExchange::rejectsInvalidUtf8()
{
    QByteArray bad = doc(QString(), QString());
    bad.replace("workout-trainer", "round\xC3timer");
    ImportPreview p = preview(bad, &t_.db);
    QVERIFY(!p.valid);
    QVERIFY(has(p.problems, ImportIssue::NotUtf8, QString()));

    // Overlong form, lone surrogate and a truncated sequence.
    QVERIFY(!preview(QByteArray("{\"format\":\"\xC0\xAF\"}"), &t_.db).valid);
    QVERIFY(!preview(QByteArray("{\"format\":\"\xED\xA0\x80\"}"), &t_.db).valid);
    QVERIFY(has(preview(QByteArray("{\"a\":\"\xE2\x82"), &t_.db).problems, ImportIssue::NotUtf8, QString()));
}

void TstExchange::rejectsInvalidJson()
{
    QVERIFY(has(preview("{\"format\": ", &t_.db).problems, ImportIssue::NotJson, QString()));
    QVERIFY(has(preview("not json at all", &t_.db).problems, ImportIssue::NotJson, QString()));
    QVERIFY(has(preview("[1, 2]", &t_.db).problems, ImportIssue::WrongType, QString()));
}

void TstExchange::rejectsWrongFormat()
{
    ImportPreview p = preview(doc(QString(), QString(), "\"format\":\"workout-trainer-starter\",\"version\":1"), &t_.db);
    QVERIFY(!p.valid);
    QVERIFY(has(p.problems, ImportIssue::WrongFormat, "format"));
    p = preview(doc(QString(), QString(), "\"format\":\"other\",\"version\":1"), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::WrongFormat, "format"));
    p = preview(doc(QString(), QString(), "\"format\":7,\"version\":1"), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::WrongType, "format"));
    p = preview("{\"version\":1,\"exercises\":[],\"workouts\":[]}", &t_.db);
    QVERIFY(has(p.problems, ImportIssue::Missing, "format"));
}

void TstExchange::rejectsNewerVersion()
{
    const ImportPreview p = preview(doc(QString(), QString(), "\"format\":\"workout-trainer-workouts\",\"version\":5"), &t_.db);
    QVERIFY(!p.valid);
    QVERIFY(has(p.problems, ImportIssue::NewerVersion, "version"));
    ImportIssue newer;
    newer.code = ImportIssue::NewerVersion;
    QVERIFY(newer.text().contains("newer version of the app"));
}

void TstExchange::rejectsBadVersionValues()
{
    const QString format = QStringLiteral("\"format\":\"workout-trainer-workouts\",");
    QVERIFY(has(preview(doc(QString(), QString(), format + "\"version\":\"1\""), &t_.db).problems, ImportIssue::WrongType, "version"));
    QVERIFY(has(preview(doc(QString(), QString(), format + "\"version\":1.5"), &t_.db).problems, ImportIssue::WrongType, "version"));
    QVERIFY(has(preview(doc(QString(), QString(), format + "\"version\":0"), &t_.db).problems, ImportIssue::UnsupportedVersion, "version"));
    QVERIFY(has(preview(doc(QString(), QString(), format + "\"version\":-3"), &t_.db).problems, ImportIssue::UnsupportedVersion, "version"));
    QVERIFY(has(preview("{\"format\":\"workout-trainer-workouts\",\"exercises\":[],\"workouts\":[]}", &t_.db).problems,
                ImportIssue::Missing, "version"));
}

void TstExchange::rejectsFilesOverFourMiB()
{
    QByteArray big = doc(QString(), QString());
    big.append(QByteArray(int(maxFileBytes) + 1 - big.size(), ' '));
    QCOMPARE(qint64(big.size()), maxFileBytes + 1);
    const ImportPreview p = preview(big, &t_.db);
    QVERIFY(!p.valid);
    QVERIFY(has(p.problems, ImportIssue::TooLarge, QString()));
}

void TstExchange::acceptsAFileOfExactlyFourMiB()
{
    QByteArray big = doc(QString(), QString());
    big.append(QByteArray(int(maxFileBytes) - big.size(), ' '));
    QCOMPARE(qint64(big.size()), maxFileBytes);
    const ImportPreview p = preview(big, &t_.db);
    QVERIFY(p.valid);
    QVERIFY(p.nothingToImport);
}

void TstExchange::rejectsTooManyWorkouts()
{
    auto many = [](int count) {
        QStringList list;
        for (int i = 0; i < count; ++i)
            list.append(wk(QStringLiteral("W%1").arg(i), step("e1")));
        return doc(ex("e1", "Squat"), list.join(","));
    };
    ImportPreview p = preview(many(200), &t_.db);
    QVERIFY2(p.valid, "200 workouts are allowed");
    QCOMPARE(p.workoutsToAdd, 200);
    p = preview(many(201), &t_.db);
    QVERIFY(!p.valid);
    QVERIFY(has(p.problems, ImportIssue::TooMany, "workouts"));
}

void TstExchange::rejectsTooManyExercises()
{
    auto many = [](int count) {
        QStringList list;
        for (int i = 0; i < count; ++i)
            list.append(ex(QStringLiteral("k%1").arg(i), QStringLiteral("E%1").arg(i)));
        return doc(list.join(","), QString());
    };
    ImportPreview p = preview(many(1000), &t_.db);
    QVERIFY2(p.valid, "1000 exercises are allowed");
    QCOMPARE(p.exercisesToAdd, 1000);
    p = preview(many(1001), &t_.db);
    QVERIFY(!p.valid);
    QVERIFY(has(p.problems, ImportIssue::TooMany, "exercises"));
}

void TstExchange::rejectsWrongTypes()
{
    ImportPreview p = preview("{\"format\":\"workout-trainer-workouts\",\"version\":4,\"exercises\":{},\"workouts\":[]}", &t_.db);
    QVERIFY(has(p.problems, ImportIssue::WrongType, "exercises"));
    p = preview("{\"format\":\"workout-trainer-workouts\",\"version\":4,\"exercises\":[],\"workouts\":\"x\"}", &t_.db);
    QVERIFY(has(p.problems, ImportIssue::WrongType, "workouts"));
    p = preview(doc("5", QString()), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::WrongType, "exercises[0]"));
    p = preview(doc("{\"key\":\"a\",\"name\":7,\"muscles\":[\"chest\"]}", QString()), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::WrongType, "exercises[0].name"));
    p = preview(doc("{\"key\":\"a\",\"name\":\"A\",\"muscles\":\"chest\"}", QString()), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::WrongType, "exercises[0].muscles"));
    p = preview(doc("{\"key\":\"a\",\"name\":\"A\",\"muscles\":[3]}", QString()), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::WrongType, "exercises[0].muscles[0]"));
    p = preview(doc("{\"key\":\"a\",\"name\":\"A\",\"muscles\":[\"chest\"],\"tips\":[]}", QString()), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::WrongType, "exercises[0].tips"));
    p = preview(doc(ex("a", "A"), "[]"), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::WrongType, "workouts[0]"));
    p = preview(doc(ex("a", "A"), wk("W", step("a", "\"durationSec\":\"30\""))), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::WrongType, "workouts[0].blocks[0].steps[0].durationSec"));
    p = preview(doc(ex("a", "A"), wk("W", step("a", "\"durationSec\":30.5"))), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::WrongType, "workouts[0].blocks[0].steps[0].durationSec"));
    p = preview(doc(ex("a", "A"), "{\"name\":\"W\",\"blocks\":{}}"), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::WrongType, "workouts[0].blocks"));
    p = preview(doc(ex("a", "A"), "{\"name\":\"W\",\"blocks\":[5]}"), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::WrongType, "workouts[0].blocks[0]"));
    p = preview(doc(ex("a", "A"), "{\"name\":\"W\",\"blocks\":[{\"steps\":[7]}]}"), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::WrongType, "workouts[0].blocks[0].steps[0]"));
    p = preview(doc(ex("a", "A"), "{\"name\":\"W\",\"blocks\":[{\"rounds\":\"2\",\"steps\":[]}]}"), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::WrongType, "workouts[0].blocks[0].rounds"));
}

void TstExchange::rejectsMissingFields()
{
    ImportPreview p = preview("{\"format\":\"workout-trainer-workouts\",\"version\":4,\"workouts\":[]}", &t_.db);
    QVERIFY(has(p.problems, ImportIssue::Missing, "exercises"));
    p = preview("{\"format\":\"workout-trainer-workouts\",\"version\":4,\"exercises\":[]}", &t_.db);
    QVERIFY(has(p.problems, ImportIssue::Missing, "workouts"));
    p = preview(doc("{\"name\":\"A\",\"muscles\":[\"chest\"]}", QString()), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::Missing, "exercises[0].key"));
    p = preview(doc("{\"key\":\"a\",\"muscles\":[\"chest\"]}", QString()), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::Missing, "exercises[0].name"));
    p = preview(doc("{\"key\":\"a\",\"name\":\"A\"}", QString()), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::Missing, "exercises[0].muscles"));
    p = preview(doc("{\"key\":\"a\",\"name\":\"A\",\"muscles\":[]}", QString()), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::NoMuscles, "exercises[0].muscles"));
    p = preview(doc(ex("a", "A"), "{\"blocks\":[]}"), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::Missing, "workouts[0].name"));
    p = preview(doc(ex("a", "A"), "{\"name\":\"W\"}"), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::Missing, "workouts[0].blocks"));
    p = preview(doc(ex("a", "A"), "{\"name\":\"W\",\"blocks\":[]}"), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::Missing, "workouts[0].blocks"));
    p = preview(doc(ex("a", "A"), "{\"name\":\"W\",\"blocks\":[{\"rounds\":1}]}"), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::Missing, "workouts[0].blocks[0].steps"));
    p = preview(doc(ex("a", "A"), wk("W", "{\"exercise\":\"a\",\"durationSec\":5}")), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::Missing, "workouts[0].blocks[0].steps[0].type"));
    p = preview(doc(ex("a", "A"), wk("W", "{\"type\":\"work\",\"durationSec\":5}")), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::Missing, "workouts[0].blocks[0].steps[0].exercise"));
    p = preview(doc(ex("a", "A"), wk("W", "{\"type\":\"work\",\"exercise\":\"a\"}")), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::Missing, "workouts[0].blocks[0].steps[0]"));
    p = preview(doc(ex("a", "A"), wk("W", "{\"type\":\"work\",\"exercise\":\"a\"}", "")), &t_.db);
    QVERIFY(!p.valid);
}

void TstExchange::rejectsValuesOutsideTheLimits()
{
    auto problemFor = [&](const QString &stepTail, const QString &blockExtra = QString(), const QString &workoutExtra = QString()) {
        const QString w = QStringLiteral("{\"name\":\"W\"%1,\"blocks\":[{\"rounds\":1%2,\"steps\":[%3]}]}")
                              .arg(workoutExtra, blockExtra, step("a", stepTail));
        return preview(doc(ex("a", "A"), w), &t_.db);
    };
    QVERIFY(problemFor("\"durationSec\":3600").valid);
    QVERIFY(problemFor("\"reps\":999").valid);
    ImportPreview p = problemFor("\"reps\":1000");
    QVERIFY(has(p.problems, ImportIssue::OutOfRange, "workouts[0].blocks[0].steps[0].reps"));
    p = problemFor("\"durationSec\":3601");
    QVERIFY(has(p.problems, ImportIssue::OutOfRange, "workouts[0].blocks[0].steps[0].durationSec"));
    p = problemFor("\"durationSec\":-1");
    QVERIFY(has(p.problems, ImportIssue::OutOfRange, "workouts[0].blocks[0].steps[0].durationSec"));
    p = problemFor("\"durationSec\":30,\"reps\":5");
    QVERIFY(has(p.problems, ImportIssue::NotAllowed, "workouts[0].blocks[0].steps[0]"));
    p = problemFor("\"durationSec\":30", ",\"restBetweenRoundsSec\":601");
    QVERIFY(has(p.problems, ImportIssue::OutOfRange, "workouts[0].blocks[0].restBetweenRoundsSec"));
    p = preview(doc(ex("a", "A"), "{\"name\":\"W\",\"blocks\":[{\"rounds\":100,\"steps\":[" + step("a") + "]}]}"), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::OutOfRange, "workouts[0].blocks[0].rounds"));
    p = preview(doc(ex("a", "A"), "{\"name\":\"W\",\"blocks\":[{\"rounds\":0,\"steps\":[" + step("a") + "]}]}"), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::OutOfRange, "workouts[0].blocks[0].rounds"));
    p = problemFor("\"durationSec\":30", QString(), ",\"restBetweenBlocksSec\":601");
    QVERIFY(has(p.problems, ImportIssue::OutOfRange, "workouts[0].restBetweenBlocksSec"));
    p = preview(doc(ex("a", "A"), wk("W", "{\"type\":\"rest\",\"exercise\":\"a\",\"durationSec\":5}")), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::NotAllowed, "workouts[0].blocks[0].steps[0].exercise"));
    p = preview(doc(ex("a", "A"), wk("W", "{\"type\":\"rest\",\"reps\":5}")), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::NotAllowed, "workouts[0].blocks[0].steps[0].reps"));
    p = preview(doc(ex("a", "A"), wk("W", "{\"type\":\"run\",\"exercise\":\"a\",\"durationSec\":5}")), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::OutOfRange, "workouts[0].blocks[0].steps[0].type"));
    p = preview(doc(ex("a", "A"), wk(QString(81, 'x'), step("a"))), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::TooLong, "workouts[0].name"));
    p = preview(doc(ex("a", "A"), wk("   ", step("a"))), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::NameEmpty, "workouts[0].name"));

    QStringList steps;
    for (int i = 0; i < 51; ++i)
        steps.append(step("a"));
    p = preview(doc(ex("a", "A"), wk("W", steps.join(","))), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::TooMany, "workouts[0].blocks[0].steps"));
    steps.removeLast();
    QVERIFY(preview(doc(ex("a", "A"), wk("W", steps.join(","))), &t_.db).valid);

    QStringList blocks;
    for (int i = 0; i < 51; ++i)
        blocks.append("{\"rounds\":1,\"steps\":[" + step("a") + "]}");
    p = preview(doc(ex("a", "A"), "{\"name\":\"W\",\"blocks\":[" + blocks.join(",") + "]}"), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::TooMany, "workouts[0].blocks"));

    // Exercise texts.
    p = preview(doc(ex("a", QString(81, 'x')), QString()), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::TooLong, "exercises[0].name"));
    p = preview(doc(ex("a", "A", "\"chest\"", ",\"instructions\":\"" + QString(4001, 'x') + "\""), QString()), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::TooLong, "exercises[0].instructions"));
    p = preview(doc(ex("a", "A", "\"chest\"", ",\"tips\":\"" + QString(2001, 'x') + "\""), QString()), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::TooLong, "exercises[0].tips"));
    QVERIFY(preview(doc(ex("a", QString(80, 'x'), "\"chest\"", ",\"instructions\":\"" + QString(4000, 'x') + "\",\"tips\":\"" + QString(2000, 'x') + "\""), QString()), &t_.db).valid);
    p = preview(doc(ex("a", "A"), wk("W", step("a")) + "," + wk("W2", step("a", "\"reps\":0"))), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::Missing, "workouts[1].blocks[0].steps[0]"));
}

void TstExchange::rejectsDuplicateExerciseKeys()
{
    const ImportPreview p = preview(doc(ex("a", "First") + "," + ex("b", "Other") + "," + ex("a", "Second"), QString()), &t_.db);
    QVERIFY(!p.valid);
    QVERIFY(has(p.problems, ImportIssue::DuplicateKey, "exercises[2].key"));
    QCOMPARE(countCode(p.problems, ImportIssue::DuplicateKey), 1);
}

void TstExchange::rejectsUnknownExerciseReferences()
{
    ImportPreview p = preview(doc(ex("a", "A"), wk("W", step("missing"))), &t_.db);
    QVERIFY(!p.valid);
    QVERIFY(has(p.problems, ImportIssue::UnknownExercise, "workouts[0].blocks[0].steps[0].exercise"));

    // A key of the library is enough.
    ExerciseStore exercises(&t_.db);
    Exercise e = makeExercise("Library entry");
    e.key = QStringLiteral("lib");
    QVERIFY(exercises.createKeyed(e) == StoreStatus::Ok);
    p = preview(doc(QString(), wk("W", step("lib"))), &t_.db);
    QVERIFY(p.valid);
    QVERIFY(importNow(t_.db, doc(QString(), wk("W", step("lib")))).status == StoreStatus::Ok);
    QCOMPARE(countRows(t_.db, "workout"), 1);
}

void TstExchange::rejectsInvalidIconKeys()
{
    const QStringList bad = QStringList() << "Push" << "push up" << "push_up" << QString(41, 'a') << "pu\\u00fcsh";
    for (const QString &icon : bad) {
        const ImportPreview p = preview(doc(ex("a", "A", "\"chest\"", ",\"icon\":\"" + icon + "\""), QString()), &t_.db);
        QVERIFY2(!p.valid, qPrintable(icon));
        QVERIFY(has(p.problems, ImportIssue::IconInvalid, "exercises[0].icon"));
    }
    QVERIFY(preview(doc(ex("a", "A", "\"chest\"", ",\"icon\":\"" + QString(40, 'a') + "\""), QString()), &t_.db).valid);
    QVERIFY(preview(doc(ex("a", "A", "\"chest\"", ",\"icon\":\"\""), QString()), &t_.db).valid);
    QVERIFY(preview(doc(ex("a", "A", "\"chest\"", ",\"icon\":\"arm-3\""), QString()), &t_.db).valid);
}

void TstExchange::rejectsInvalidKeysAndNames()
{
    ImportPreview p = preview(doc(ex("", "A"), QString()), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::KeyInvalid, "exercises[0].key"));
    p = preview(doc(ex("a b", "A"), QString()), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::KeyInvalid, "exercises[0].key"));
    p = preview(doc(ex(QString(65, 'a'), "A"), QString()), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::KeyInvalid, "exercises[0].key"));
    QVERIFY(preview(doc(ex(QString(64, 'a'), "A"), QString()), &t_.db).valid);
    p = preview(doc(ex("a", "  "), QString()), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::NameEmpty, "exercises[0].name"));
}

void TstExchange::rejectedFileChangesNothing()
{
    buildFixture(t_.db);
    const QString before = snapshot(t_.db);
    // One good workout and one bad one: nothing of the file is written.
    const QByteArray bytes = doc(ex("n1", "New"), wk("Good", step("n1")) + "," + wk("Bad", step("n1", "\"reps\":5000")));
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(!p.valid);
    QVERIFY(p.exercises.isEmpty());
    QVERIFY(p.workouts.isEmpty());
    const ImportResult result = apply(p, ImportChoices(), &t_.db);
    QVERIFY(result.status == StoreStatus::Invalid);
    QCOMPARE(snapshot(t_.db), before);
}

void TstExchange::unknownExtraFieldsAreIgnored()
{
    const QByteArray bytes =
        "{\"format\":\"workout-trainer-workouts\",\"version\":4,\"future\":{\"a\":1},"
        "\"exercises\":[{\"key\":\"a\",\"name\":\"A\",\"muscles\":[\"chest\"],\"video\":\"x\"}],"
        "\"workouts\":[{\"name\":\"W\",\"mood\":3,\"blocks\":[{\"rounds\":1,\"extra\":[],\"steps\":"
        "[{\"type\":\"work\",\"exercise\":\"a\",\"durationSec\":5,\"note\":\"n\"}]}]}]}";
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QVERIFY(p.notices.isEmpty());
    QCOMPARE(p.workoutsToAdd, 1);
}

void TstExchange::namesAreTrimmed()
{
    const ImportPreview p = preview(doc(ex("a", "  Squat \\t"), wk("  Legs  ", step("a"))), &t_.db);
    QVERIFY(p.valid);
    QCOMPARE(p.exercises.first().name, QStringLiteral("Squat"));
    QCOMPARE(p.workouts.first().name, QStringLiteral("Legs"));
}

void TstExchange::unknownColourIsDroppedWithNotice()
{
    const QByteArray bytes = doc(ex("a", "A", "\"chest\"", ",\"colour\":\"magenta\""), QString());
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QVERIFY(has(p.notices, ImportIssue::UnknownColour, "exercises[0].colour"));
    QCOMPARE(p.exercises.first().colour, QString());
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    Exercise e;
    QVERIFY(ExerciseStore(&t_.db).get("a", &e));
    QCOMPARE(e.colour, QString());
}

void TstExchange::iconAndColourSurviveARoundTrip()
{
    buildFixture(t_.db);
    QByteArray bytes;
    QVERIFY(exportAll(&t_.db, &bytes));
    TestDb other;
    QVERIFY(other.open());
    QVERIFY(importNow(other.db, bytes).status == StoreStatus::Ok);
    Exercise e;
    QVERIFY(ExerciseStore(&other.db).get("fix-push", &e));
    QCOMPARE(e.icon, QStringLiteral("push"));
    QCOMPARE(e.colour, QStringLiteral("red"));
    QVERIFY(ExerciseStore(&other.db).get("fix-squat", &e));
    QCOMPARE(e.icon, QString());
    QCOMPARE(e.colour, QString());
    // Empty values are not written.
    QVERIFY(!bytes.mid(bytes.indexOf("fix-squat")).startsWith("\"icon\""));
    for (const QString &c : Colours::keys())
        QVERIFY(Colours::isKnown(c));
}

void TstExchange::unknownButValidIconIsKept()
{
    const QByteArray bytes = doc(ex("a", "A", "\"chest\"", ",\"icon\":\"from-newer-set-9\""), QString());
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    Exercise e;
    QVERIFY(ExerciseStore(&t_.db).get("a", &e));
    QCOMPARE(e.icon, QStringLiteral("from-newer-set-9"));
}

// A workout with one exercise "a" and the given steps, as file text.
static QByteArray fileWithSteps(const QString &steps)
{
    return doc(ex("a", "A"), wk("W", steps));
}

static QString repsStep(const QString &tail)
{
    return QStringLiteral("{\"type\":\"work\",\"exercise\":\"a\",\"reps\":8%1}").arg(tail);
}

// A keyed exercise "a" and a saved metronome linked to it; returns the metronome id.
static int linkedExercise(Database &db, const QString &key, const QString &metronomeName, const Tempo &tempo)
{
    ExerciseStore exercises(&db);
    Exercise e = makeExercise(key.toUpper(), muscleBit(MuscleGroup::Core));
    e.key = key;
    if (exercises.createKeyed(e) != StoreStatus::Ok)
        return 0;
    const int mid = addMetronome(db, metronomeName, tempo);
    return exercises.setMetronome(key, mid) == StoreStatus::Ok ? mid : 0;
}

void TstExchange::startModeIsWrittenOnlyWhenSet()
{
    ExerciseStore exercises(&t_.db);
    Exercise e = makeExercise("A", muscleBit(MuscleGroup::Core));
    e.key = QStringLiteral("a");
    QVERIFY(exercises.createKeyed(e) == StoreStatus::Ok);
    WorkoutStore workouts(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>()
        << work("a", 30) << onTap(rest(10)) << onTap(repStep("a", 7)) << repStep("a", 5)));
    w.name = "Modes";
    int id = 0;
    QVERIFY(workouts.create(w, &id) == StoreStatus::Ok);
    QByteArray bytes;
    QVERIFY(exportWorkouts(&t_.db, QList<int>() << id, &bytes));
    const QString expected = QStringLiteral(
        "                    \"steps\": [\n"
        "                        {\n"
        "                            \"durationSec\": 30,\n"
        "                            \"exercise\": \"a\",\n"
        "                            \"type\": \"work\"\n"
        "                        },\n"
        "                        {\n"
        "                            \"durationSec\": 10,\n"
        "                            \"startOnTap\": true,\n"
        "                            \"type\": \"rest\"\n"
        "                        },\n"
        "                        {\n"
        "                            \"exercise\": \"a\",\n"
        "                            \"reps\": 7,\n"
        "                            \"startOnTap\": true,\n"
        "                            \"type\": \"work\"\n"
        "                        },\n"
        "                        {\n"
        "                            \"exercise\": \"a\",\n"
        "                            \"reps\": 5,\n"
        "                            \"type\": \"work\"\n"
        "                        }\n"
        "                    ]\n");
    QVERIFY2(bytes.contains(expected.toUtf8()), bytes.constData());
    QCOMPARE(bytes.count("startOnTap"), 2);
}

void TstExchange::stepsCarryNoMetronome()
{
    // The tempo belongs to the exercise: a paced step is written like any repetition step.
    const int mid = linkedExercise(t_.db, "a", "Pace", Tempo::timePerRep(1500, 4));
    QVERIFY(mid > 0);
    WorkoutStore workouts(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << repStep("a", 7) << rest(10)));
    w.name = "W";
    int id = 0;
    QVERIFY(workouts.create(w, &id) == StoreStatus::Ok);
    QByteArray bytes;
    QVERIFY(exportWorkouts(&t_.db, QList<int>() << id, &bytes));
    const QByteArray steps = bytes.mid(bytes.indexOf("\"workouts\""));
    QVERIFY(!steps.contains("\"metronome\""));
    QVERIFY(!steps.contains("\"tempo\""));
    QVERIFY(steps.contains("\"reps\": 7"));
}

void TstExchange::exercisesCarryTheirMetronomeKey()
{
    const int mid = linkedExercise(t_.db, "a", "Three phases", phasesOf({ 2, 3, 1 }, 1));
    QVERIFY(mid > 0);
    ExerciseStore exercises(&t_.db);
    Exercise plain = makeExercise("Plain", muscleBit(MuscleGroup::Core));
    plain.key = QStringLiteral("p");
    QVERIFY(exercises.createKeyed(plain) == StoreStatus::Ok);
    WorkoutStore workouts(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << repStep("a", 6) << repStep("p", 3)));
    w.name = "W";
    int id = 0;
    QVERIFY(workouts.create(w, &id) == StoreStatus::Ok);
    SavedMetronome m;
    QVERIFY(MetronomeStore(&t_.db).get(mid, &m));
    QByteArray bytes;
    QVERIFY(exportWorkouts(&t_.db, QList<int>() << id, &bytes));
    QVERIFY(bytes.contains("\"version\": 4"));
    const QString expectedExercise = QStringLiteral(
        "            \"key\": \"a\",\n"
        "            \"language\": \"en\",\n"
        "            \"metronome\": \"%1\",\n").arg(m.key);
    QVERIFY2(bytes.contains(expectedExercise.toUtf8()), bytes.constData());
    // The exercise without a metronome has no key for it.
    QCOMPARE(bytes.count("\"metronome\""), 1);
    const QString expectedList = QStringLiteral(
        "    \"metronomes\": [\n"
        "        {\n"
        "            \"key\": \"%1\",\n"
        "            \"name\": \"Three phases\",\n"
        "            \"phases\": [\n"
        "                2,\n"
        "                3,\n"
        "                1\n"
        "            ]\n"
        "        }\n"
        "    ],\n").arg(m.key);
    QVERIFY2(bytes.contains(expectedList.toUtf8()), bytes.constData());
    QVERIFY(m.key.startsWith(QLatin1String("m-")));
}

void TstExchange::onlyTheMetronomesOfTheExportedExercisesTravel()
{
    QVERIFY(linkedExercise(t_.db, "a", "Used", Tempo::timePerRep(1000, 3)) > 0);
    QVERIFY(linkedExercise(t_.db, "b", "Of an unused exercise", Tempo::timePerRep(1100, 3)) > 0);
    QVERIFY(addMetronome(t_.db, "Not linked", Tempo::timePerRep(1200, 3)) > 0);
    WorkoutStore workouts(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << repStep("a", 6)));
    w.name = "W";
    int id = 0;
    QVERIFY(workouts.create(w, &id) == StoreStatus::Ok);
    QByteArray bytes;
    QVERIFY(exportWorkouts(&t_.db, QList<int>() << id, &bytes));
    QVERIFY(bytes.contains("\"Used\""));
    QVERIFY(!bytes.contains("Of an unused exercise"));
    QVERIFY(!bytes.contains("Not linked"));
    QCOMPARE(bytes.count("\"phases\""), 1);
}

void TstExchange::phaseNamesTravelInTheFile()
{
    const QString emoji = QString::fromUtf8("\xf0\x9f\x98\x80");
    const Tempo named = Tempo::phases({ 2, 3, 1 }, 1, QStringList() << "a;b\"c" << "" << emoji + QString::fromUtf8("\xc3\xa9") + "123456789");
    const Tempo plain = phasesOf({ 2, 3, 1 }, 1);
    QVERIFY(named.isValid());
    QVERIFY(linkedExercise(t_.db, "a", "Named", named) > 0);
    QVERIFY(linkedExercise(t_.db, "b", "Plain", plain) > 0);
    WorkoutStore workouts(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << repStep("a", 6) << repStep("b", 6), 1, 0));
    w.name = "Names";
    int id = 0;
    QVERIFY(workouts.create(w, &id) == StoreStatus::Ok);
    QByteArray bytes;
    QVERIFY(exportAll(&t_.db, &bytes));
    // The names are written for the named tempo only.
    QCOMPARE(QString::fromUtf8(bytes).count(QLatin1String("phaseNames")), 1);
    QVERIFY(bytes.contains("\"version\": 4"));

    TestDb other;
    QVERIFY(other.open());
    const ImportPreview p = preview(bytes, &other.db);
    QVERIFY(p.valid);
    QVERIFY(p.problems.isEmpty());
    const ImportResult r = apply(p, ImportChoices(), &other.db);
    QVERIFY(r.status == StoreStatus::Ok);
    MetronomeStore store(&other.db);
    int found = 0;
    for (const MetronomeSummary &m : store.list()) {
        SavedMetronome got;
        QVERIFY(store.get(m.id, &got));
        if (got.name == QLatin1String("Named")) {
            QVERIFY(got.tempo == named);
            QCOMPARE(got.tempo.phaseName(1), QStringLiteral("a;b\"c"));
            ++found;
        } else if (got.name == QLatin1String("Plain")) {
            QVERIFY(got.tempo == plain);
            QCOMPARE(got.tempo.phaseNameList(), QStringList() << "" << "" << "");
            ++found;
        }
    }
    QCOMPARE(found, 2);

    // The imported exercise has the tempo with the names; importing again links the metronomes.
    const ExerciseTempos tempos = ExerciseStore(&other.db).tempoMap();
    QVERIFY(tempos.value("a") == named);
    const ImportPreview again = preview(bytes, &other.db);
    QCOMPARE(again.metronomesToAdd, 0);
}

void TstExchange::metronomeLinksRoundTrip()
{
    const int m1 = linkedExercise(t_.db, "a", "Even", Tempo::repsAndTotal(7, 20000));
    QVERIFY(linkedExercise(t_.db, "b", "Fast", Tempo::timePerRep(500, 999)) > 0);
    QVERIFY(linkedExercise(t_.db, "c", "One", phasesOf({ 1 }, 1)) > 0);
    QVERIFY(linkedExercise(t_.db, "d", "Eight", phasesOf({ 20, 20, 20, 20, 20, 20 }, 3)) > 0);
    ExerciseStore exercises(&t_.db);
    Exercise plain = makeExercise("Plain", muscleBit(MuscleGroup::Core));
    plain.key = QStringLiteral("p");
    QVERIFY(exercises.createKeyed(plain) == StoreStatus::Ok);
    WorkoutStore workouts(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>()
        << work("a", 30) << onTap(rest(10)) << repStep("a", 7) << onTap(repStep("b", 20))
        << repStep("c", 1) << repStep("d", 3) << repStep("p", 4), 2, 5));
    w.name = "Modes";
    int id = 0;
    QVERIFY(workouts.create(w, &id) == StoreStatus::Ok);
    QByteArray first;
    QVERIFY(exportAll(&t_.db, &first));

    TestDb other;
    QVERIFY(other.open());
    const ImportPreview p = preview(first, &other.db);
    QVERIFY(p.valid);
    QCOMPARE(p.metronomes.size(), 4);
    QCOMPARE(p.metronomesToAdd, 4);
    const ImportResult r = apply(p, ImportChoices(), &other.db);
    QVERIFY(r.status == StoreStatus::Ok);
    QCOMPARE(r.metronomesAdded, 4);
    QCOMPARE(r.metronomesReused, 0);
    QCOMPARE(countRows(other.db, "metronome"), 4);

    ExerciseStore otherExercises(&other.db);
    MetronomeStore otherMetronomes(&other.db);
    Exercise read;
    SavedMetronome got;
    QVERIFY(otherExercises.get("a", &read));
    QVERIFY(read.metronomeId > 0);
    QVERIFY(otherMetronomes.get(read.metronomeId, &got));
    QCOMPARE(got.name, QStringLiteral("Even"));
    QVERIFY(got.tempo == rhythmOf(Tempo::repsAndTotal(7, 20000)));
    QVERIFY(otherExercises.get("d", &read));
    QVERIFY(otherMetronomes.get(read.metronomeId, &got));
    QVERIFY(got.tempo == phasesOf({ 20, 20, 20, 20, 20, 20 }, 1));
    // The exercise without a metronome stays without one.
    QVERIFY(otherExercises.get("p", &read));
    QCOMPARE(read.metronomeId, 0);
    // The keys travel with the file.
    SavedMetronome original;
    QVERIFY(MetronomeStore(&t_.db).get(m1, &original));
    QVERIFY(otherMetronomes.getByKey(original.key, &got));
    QCOMPARE(got.name, QStringLiteral("Even"));
    // The step that waits for a tap keeps its flag.
    WorkoutStore otherWorkouts(&other.db);
    Workout back;
    QVERIFY(otherWorkouts.get(otherWorkouts.list().first().id, &back));
    QVERIFY(back.blocks.first().steps.at(3).startOnTap);
    // The second export is the same text.
    QByteArray second;
    QVERIFY(exportAll(&other.db, &second));
    QCOMPARE(second, first);
}

void TstExchange::aSharedMetronomeStaysShared()
{
    const int mid = linkedExercise(t_.db, "a", "Shared", Tempo::timePerRep(1500, 4));
    QVERIFY(mid > 0);
    ExerciseStore exercises(&t_.db);
    Exercise e = makeExercise("B", muscleBit(MuscleGroup::Core));
    e.key = QStringLiteral("b");
    QVERIFY(exercises.createKeyed(e) == StoreStatus::Ok);
    QVERIFY(exercises.setMetronome("b", mid) == StoreStatus::Ok);
    WorkoutStore workouts(&t_.db);
    Workout one = workout(QList<Block>() << block(QList<Step>() << repStep("a", 4) << repStep("b", 4)));
    one.name = "One";
    QVERIFY(workouts.create(one) == StoreStatus::Ok);
    QByteArray bytes;
    QVERIFY(exportAll(&t_.db, &bytes));
    // One metronome in the file, two exercises that name it.
    QCOMPARE(bytes.count("\"phases\""), 1);
    QCOMPARE(bytes.count("\"metronome\""), 2);

    TestDb other;
    QVERIFY(other.open());
    QVERIFY(importNow(other.db, bytes).status == StoreStatus::Ok);
    QCOMPARE(countRows(other.db, "metronome"), 1);
    ExerciseStore otherExercises(&other.db);
    Exercise a, b;
    QVERIFY(otherExercises.get("a", &a));
    QVERIFY(otherExercises.get("b", &b));
    QVERIFY(a.metronomeId > 0);
    QCOMPARE(a.metronomeId, b.metronomeId);
    QCOMPARE(MetronomeStore(&other.db).list().first().exerciseCount, 2);
}

void TstExchange::sameKeyAndContentIsNotDuplicated()
{
    QVERIFY(linkedExercise(t_.db, "a", "Pace", Tempo::timePerRep(1500, 4)) > 0);
    WorkoutStore workouts(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << repStep("a", 4)));
    w.name = "W";
    QVERIFY(workouts.create(w) == StoreStatus::Ok);
    QByteArray bytes;
    QVERIFY(exportAll(&t_.db, &bytes));

    // The same file into the same library: the stored metronome is linked again and the exercise
    // is the same exercise.
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QCOMPARE(p.metronomesReused, 1);
    QCOMPARE(p.metronomesToAdd, 0);
    QCOMPARE(p.exercisesReused, 1);
    QVERIFY(p.conflicts.isEmpty());
    const ImportResult r = apply(p, ImportChoices(), &t_.db);
    QVERIFY(r.status == StoreStatus::Ok);
    QCOMPARE(r.metronomesReused, 1);
    QCOMPARE(r.metronomesAdded, 0);
    QCOMPARE(countRows(t_.db, "metronome"), 1);
    QCOMPARE(workouts.list().size(), 2);
    QCOMPARE(MetronomeStore(&t_.db).list().first().exerciseCount, 1);
}

void TstExchange::sameKeyWithOtherContentIsAddedAsNew()
{
    const int mid = linkedExercise(t_.db, "a", "Pace", Tempo::timePerRep(1500, 4));
    WorkoutStore workouts(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << repStep("a", 4)));
    w.name = "W";
    QVERIFY(workouts.create(w) == StoreStatus::Ok);
    QByteArray bytes;
    QVERIFY(exportAll(&t_.db, &bytes));

    // The stored metronome changes after the export: the file's content differs.
    SavedMetronome m;
    MetronomeStore store(&t_.db);
    QVERIFY(store.get(mid, &m));
    m.tempo = rhythmOf(Tempo::timePerRep(2000, 4));
    QVERIFY(store.update(mid, m) == StoreStatus::Ok);

    // The exercise of the file links a metronome the stored exercise does not have: a conflict.
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QCOMPARE(p.conflicts.size(), 1);
    QCOMPARE(p.metronomesToAdd, 1);
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    QCOMPARE(countRows(t_.db, "metronome"), 2);
    // The stored one and the exercise that uses it are unchanged (the default choice keeps it).
    SavedMetronome again;
    QVERIFY(store.get(mid, &again));
    QVERIFY(again.tempo == rhythmOf(Tempo::timePerRep(2000, 4)));
    QCOMPARE(again.key, m.key);
    Exercise kept;
    QVERIFY(ExerciseStore(&t_.db).get("a", &kept));
    QCOMPARE(kept.metronomeId, mid);

    // Replacing the exercise links the new metronome, which has the file's pace, a new key and a free name.
    ImportChoices choices;
    choices.insert("a", ConflictChoice::Replace);
    const ImportPreview again2 = preview(bytes, &t_.db);
    QVERIFY(apply(again2, choices, &t_.db).status == StoreStatus::Ok);
    Exercise replaced;
    QVERIFY(ExerciseStore(&t_.db).get("a", &replaced));
    SavedMetronome added;
    QVERIFY(store.get(replaced.metronomeId, &added));
    QVERIFY(added.id != mid);
    QVERIFY(added.tempo == rhythmOf(Tempo::timePerRep(1500, 4)));
    QVERIFY(added.key != m.key);
    QVERIFY(added.name.startsWith(QLatin1String("Pace (imported")));
}

void TstExchange::metronomeIdentityClashGetsTheSuffix()
{
    const QString metronomes = QStringLiteral(
        "{\"key\":\"k1\",\"name\":\"Pace\",\"phases\":[2]},"
        "{\"key\":\"k2\",\"name\":\"pace \",\"phases\":[4]},"
        "{\"key\":\"k3\",\"name\":\"Pace\",\"colour\":\"red\",\"phases\":[6]}");
    const QByteArray bytes = QStringLiteral("{\"format\":\"workout-trainer-workouts\",\"version\":4,\"exercises\":[],"
                                          "\"metronomes\":[%1],\"workouts\":[]}").arg(metronomes).toUtf8();
    QVERIFY(addMetronome(t_.db, "PACE", Tempo::timePerRep(700, 2)) > 0);
    const ImportResult clashed = importNow(t_.db, bytes);
    QVERIFY(clashed.status == StoreStatus::Ok);
    QCOMPARE(clashed.metronomesRenamed, 3);
    QStringList names;
    for (const MetronomeSummary &s : MetronomeStore(&t_.db).list())
        names << s.name;
    // Every one clashes by name with the stored "PACE" or with an earlier one of the file.
    names.sort(Qt::CaseSensitive);
    QCOMPARE(names, QStringList() << "PACE" << "Pace (imported 3)" << "Pace (imported)" << "pace (imported 2)");
}

void TstExchange::rhythmIconInAFileIsIgnored()
{
    // Files of older versions carry an icon or a colour on a rhythm: any value, even an invalid one, is ignored.
    const QString metronomes = QStringLiteral(
        "{\"key\":\"k1\",\"name\":\"Even\",\"icon\":\"wave\",\"colour\":\"red\",\"phases\":[2,2]},"
        "{\"key\":\"k2\",\"name\":\"Odd\",\"icon\":\"Bad Icon\",\"phases\":[3]},"
        "{\"key\":\"k3\",\"name\":\"Other\",\"icon\":5,\"phases\":[4]}");
    const QByteArray bytes = QStringLiteral("{\"format\":\"workout-trainer-workouts\",\"version\":4,\"exercises\":[],"
                                          "\"metronomes\":[%1],\"workouts\":[]}").arg(metronomes).toUtf8();
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QVERIFY(p.problems.isEmpty());
    QCOMPARE(p.metronomesToAdd, 3);
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    QCOMPARE(countRows(t_.db, "metronome"), 3);
    // Nothing of the icon is stored.
    QSqlQuery q;
    QVERIFY(t_.db.run("SELECT COUNT(*) FROM metronome WHERE icon <> ''", QVariantList(), &q) && q.next());
    QCOMPARE(q.value(0).toInt(), 0);
}

void TstExchange::rhythmsThatDifferOnlyByAnOldIconClash()
{
    const QString metronomes = QStringLiteral(
        "{\"key\":\"k1\",\"name\":\"Pace\",\"icon\":\"a\",\"phases\":[2]},"
        "{\"key\":\"k2\",\"name\":\"Pace\",\"icon\":\"b\",\"phases\":[4]}");
    const QByteArray bytes = QStringLiteral("{\"format\":\"workout-trainer-workouts\",\"version\":4,\"exercises\":[],"
                                          "\"metronomes\":[%1],\"workouts\":[]}").arg(metronomes).toUtf8();
    const ImportResult r = importNow(t_.db, bytes);
    QVERIFY(r.status == StoreStatus::Ok);
    QCOMPARE(r.metronomesRenamed, 1);
    QStringList names;
    for (const MetronomeSummary &s : MetronomeStore(&t_.db).list())
        names << s.name;
    QCOMPARE(names, QStringList() << "Pace" << "Pace (imported)");
}

void TstExchange::linkToAMetronomeOfTheLibraryIsAccepted()
{
    const int mid = addMetronome(t_.db, "Local", Tempo::timePerRep(1000, 4));
    SavedMetronome m;
    QVERIFY(MetronomeStore(&t_.db).get(mid, &m));
    const QByteArray bytes = doc(ex("a", "A", "\"chest\"", QStringLiteral(",\"metronome\":\"%1\"").arg(m.key)),
                                 wk("W", repsStep(QString())), "\"format\":\"workout-trainer-workouts\",\"version\":4");
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QVERIFY(p.problems.isEmpty());
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    Exercise e;
    QVERIFY(ExerciseStore(&t_.db).get("a", &e));
    QCOMPARE(e.metronomeId, mid);
    QCOMPARE(countRows(t_.db, "metronome"), 1);
}

void TstExchange::anExerciseLinkToAnUnknownMetronomeIsAFileProblem()
{
    const QByteArray bytes = doc(ex("a", "A"), wk("W", repsStep(QString())), "\"format\":\"workout-trainer-workouts\",\"version\":4");
    QVERIFY(preview(bytes, &t_.db).valid);
    const QByteArray linked = doc(ex("b", "B") + "," + ex("a", "A", "\"chest\"", ",\"metronome\":\"nowhere\""),
                                  wk("W", repsStep(QString())), "\"format\":\"workout-trainer-workouts\",\"version\":4");
    const ImportPreview p = preview(linked, &t_.db);
    QVERIFY(!p.valid);
    QVERIFY(has(p.problems, ImportIssue::UnknownMetronome, "exercises[1].metronome"));
    QVERIFY(!p.problems.first().text().isEmpty());
    // Nothing is written, not even the valid exercise or the workout.
    QVERIFY(importNow(t_.db, linked).status == StoreStatus::Invalid);
    QCOMPARE(countRows(t_.db, "exercise"), 0);
    QCOMPARE(countRows(t_.db, "workout"), 0);
    QCOMPARE(countRows(t_.db, "metronome"), 0);
}

void TstExchange::invalidExerciseMetronomeKeysAreFileProblems()
{
    const QString head = QStringLiteral("\"format\":\"workout-trainer-workouts\",\"version\":4");
    const QString list = QStringLiteral("{\"key\":\"k1\",\"name\":\"M\",\"phases\":[2]}");
    struct Case { QString extra; ImportIssue::Code code; QString where; };
    const QList<Case> cases = QList<Case>()
        << Case{ ",\"metronome\":5", ImportIssue::WrongType, "exercises[0].metronome" }
        << Case{ ",\"metronome\":\"bad key\"", ImportIssue::KeyInvalid, "exercises[0].metronome" }
        << Case{ ",\"metronome\":\"\"", ImportIssue::KeyInvalid, "exercises[0].metronome" }
        << Case{ ",\"metronome\":\"k2\"", ImportIssue::UnknownMetronome, "exercises[0].metronome" };
    for (const Case &c : cases) {
        const QByteArray bytes = QStringLiteral("{%1,\"exercises\":[%2],\"metronomes\":[%3],\"workouts\":[]}")
                                     .arg(head, ex("a", "A", "\"chest\"", c.extra), list).toUtf8();
        const ImportPreview p = preview(bytes, &t_.db);
        QVERIFY2(!p.valid, qPrintable(c.extra));
        QVERIFY2(has(p.problems, c.code, c.where), qPrintable(c.extra));
    }
    // A key in the file's list is accepted.
    const QByteArray good = QStringLiteral("{%1,\"exercises\":[%2],\"metronomes\":[%3],\"workouts\":[]}")
                                .arg(head, ex("a", "A", "\"chest\"", ",\"metronome\":\"k1\""), list).toUtf8();
    QVERIFY(preview(good, &t_.db).valid);
    QCOMPARE(countRows(t_.db, "exercise"), 0);
    QCOMPARE(countRows(t_.db, "metronome"), 0);
}

void TstExchange::invalidRhythmObjectsAreFileProblems()
{
    const QString base = QStringLiteral("\"key\":\"k1\",\"name\":\"M\",\"phases\":[2,3]");
    struct Case { QString object; ImportIssue::Code code; QString where; };
    const QString m = QStringLiteral("metronomes[0]");
    const QList<Case> cases = QList<Case>()
        << Case{ "5", ImportIssue::WrongType, m }
        << Case{ "{\"name\":\"M\",\"phases\":[2]}", ImportIssue::Missing, m + ".key" }
        << Case{ "{\"key\":\"bad key\",\"name\":\"M\",\"phases\":[2]}", ImportIssue::KeyInvalid, m + ".key" }
        << Case{ "{\"key\":\"k1\",\"name\":\"  \",\"phases\":[2]}", ImportIssue::NameEmpty, m + ".name" }
        << Case{ "{\"key\":\"k1\",\"name\":\"" + QString(81, 'x') + "\",\"phases\":[2]}", ImportIssue::TooLong, m + ".name" }
        << Case{ "{\"key\":\"k1\",\"name\":\"M\"}", ImportIssue::Missing, m + ".phases" }
        << Case{ "{\"key\":\"k1\",\"name\":\"M\",\"phases\":5}", ImportIssue::WrongType, m + ".phases" }
        << Case{ "{\"key\":\"k1\",\"name\":\"M\",\"phases\":[]}", ImportIssue::OutOfRange, m + ".phases" }
        << Case{ "{\"key\":\"k1\",\"name\":\"M\",\"phases\":[1,1,1,1,1,1,1,1,1]}", ImportIssue::OutOfRange, m + ".phases" }
        << Case{ "{\"key\":\"k1\",\"name\":\"M\",\"phases\":[2,\"x\"]}", ImportIssue::WrongType, m + ".phases[1]" }
        << Case{ "{\"key\":\"k1\",\"name\":\"M\",\"phases\":[2,3],\"phaseNames\":\"x\"}", ImportIssue::WrongType, m + ".phaseNames" }
        << Case{ "{\"key\":\"k1\",\"name\":\"M\",\"phases\":[2,3],\"phaseNames\":[\"a\"]}", ImportIssue::OutOfRange, m + ".phaseNames" }
        << Case{ "{\"key\":\"k1\",\"name\":\"M\",\"phases\":[2,3],\"phaseNames\":[\"a\",5]}", ImportIssue::WrongType, m + ".phaseNames[1]" }
        << Case{ "{\"key\":\"k1\",\"name\":\"M\",\"phases\":[2,3],\"phaseNames\":[\"a\",\"xxxxxxxxxxxxx\"]}", ImportIssue::TooLong, m + ".phaseNames[1]" }
        << Case{ "{\"key\":\"k1\",\"name\":\"M\",\"phases\":[2,0,3]}", ImportIssue::TempoInvalid, m + ".phases" }
        << Case{ "{\"key\":\"k1\",\"name\":\"M\",\"phases\":[21]}", ImportIssue::TempoInvalid, m + ".phases" }
        << Case{ "{\"key\":\"k1\",\"name\":\"M\",\"phases\":[20,20,20,20,20,20,1]}", ImportIssue::TempoInvalid, m + ".phases" };
    for (const Case &c : cases) {
        const QByteArray bytes = QStringLiteral("{\"format\":\"workout-trainer-workouts\",\"version\":4,\"exercises\":[],"
                                              "\"metronomes\":[%1],\"workouts\":[]}").arg(c.object).toUtf8();
        const ImportPreview p = preview(bytes, &t_.db);
        QVERIFY2(!p.valid, qPrintable(c.object));
        QVERIFY2(has(p.problems, c.code, c.where), qPrintable(c.object));
    }
    // The same key twice; a valid rhythm at the limits; a rhythm has no repetitions, sets or rest, and
    // the values of a version 3 metronome are not read from a version 4 file.
    QByteArray twice = QStringLiteral("{\"format\":\"workout-trainer-workouts\",\"version\":4,\"exercises\":[],"
                                      "\"metronomes\":[{%1},{%1}],\"workouts\":[]}").arg(base).toUtf8();
    QVERIFY(has(preview(twice, &t_.db).problems, ImportIssue::DuplicateKey, "metronomes[1].key"));
    const QByteArray good = QStringLiteral("{\"format\":\"workout-trainer-workouts\",\"version\":4,\"exercises\":[],"
                                         "\"metronomes\":[{\"key\":\"k1\",\"name\":\"Longest\",\"phases\":[20,20,20,20,20,20],\"reps\":99,\"sets\":9},"
                                         "{\"key\":\"k2\",\"name\":\"Shortest\",\"phases\":[1]}],\"workouts\":[]}").toUtf8();
    const ImportPreview p = preview(good, &t_.db);
    QVERIFY(p.valid);
    QCOMPARE(p.metronomes.size(), 2);
    QVERIFY(p.metronomes.first().tempo == phasesOf({ 20, 20, 20, 20, 20, 20 }, 1));
    QCOMPARE(countRows(t_.db, "metronome"), 0);
}

void TstExchange::aStepPaceTravelsInTheFile()
{
    ExerciseStore exercises(&t_.db);
    for (const char *key : { "a", "r" }) {
        Exercise e = makeExercise(QString::fromLatin1(key).toUpper(), muscleBit(MuscleGroup::Core));
        e.key = QString::fromLatin1(key);
        QVERIFY(exercises.createKeyed(e) == StoreStatus::Ok);
    }
    Step total = repStep("a", 10);
    total.pace = PaceKind::Total;
    total.paceMs = 30000;
    Step perRep = repStep("a", 6);
    perRep.pace = PaceKind::PerRep;
    perRep.paceMs = 2500;
    WorkoutStore workouts(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << total << perRep << repStep("a", 4) << work("a", 20)));
    w.name = "Paced";
    int id = 0;
    QVERIFY(workouts.create(w, &id) == StoreStatus::Ok);
    QByteArray bytes;
    QVERIFY(exportWorkouts(&t_.db, QList<int>() << id, &bytes));
    QVERIFY(bytes.contains("\"version\": 4"));
    const QString expected = QStringLiteral(
        "                        {\n"
        "                            \"exercise\": \"a\",\n"
        "                            \"pace\": {\n"
        "                                \"form\": \"total\",\n"
        "                                \"ms\": 30000\n"
        "                            },\n"
        "                            \"reps\": 10,\n"
        "                            \"type\": \"work\"\n"
        "                        },\n"
        "                        {\n"
        "                            \"exercise\": \"a\",\n"
        "                            \"pace\": {\n"
        "                                \"form\": \"perRep\",\n"
        "                                \"ms\": 2500\n"
        "                            },\n");
    QVERIFY2(bytes.contains(expected.toUtf8()), bytes.constData());
    QCOMPARE(bytes.count("\"pace\""), 2);

    TestDb other;
    QVERIFY(other.open());
    const ImportPreview p = preview(bytes, &other.db);
    QVERIFY(p.valid);
    QVERIFY(p.problems.isEmpty());
    QVERIFY(importNow(other.db, bytes).status == StoreStatus::Ok);
    WorkoutStore otherWorkouts(&other.db);
    Workout back;
    QVERIFY(otherWorkouts.get(otherWorkouts.list().first().id, &back));
    const QList<Step> steps = back.blocks.first().steps;
    QVERIFY(steps.at(0).pace == PaceKind::Total);
    QCOMPARE(steps.at(0).paceMs, 30000);
    QVERIFY(steps.at(1).pace == PaceKind::PerRep);
    QCOMPARE(steps.at(1).paceMs, 2500);
    QVERIFY(steps.at(2).pace == PaceKind::None);
    QVERIFY(steps.at(3).pace == PaceKind::None);
    // The second export is the same text.
    QByteArray second;
    QVERIFY(exportWorkouts(&other.db, QList<int>() << otherWorkouts.list().first().id, &second));
    QCOMPARE(second, bytes);
}

// The same as fileWithSteps(), as a file of version 4.
static QByteArray fileV4(const QString &steps)
{
    return doc(ex("a", "A"), wk("W", steps), QStringLiteral("\"format\":\"workout-trainer-workouts\",\"version\":4"));
}

void TstExchange::invalidStepPacesAreFileProblems()
{
    struct Case { QString tail; ImportIssue::Code code; QString where; };
    const QString at = QStringLiteral("workouts[0].blocks[0].steps[0].pace");
    const QList<Case> cases = QList<Case>()
        << Case{ ",\"pace\":5", ImportIssue::WrongType, at }
        << Case{ ",\"pace\":{\"ms\":1000}", ImportIssue::Missing, at + ".form" }
        << Case{ ",\"pace\":{\"form\":\"waltz\",\"ms\":1000}", ImportIssue::OutOfRange, at + ".form" }
        << Case{ ",\"pace\":{\"form\":\"total\"}", ImportIssue::Missing, at + ".ms" }
        << Case{ ",\"pace\":{\"form\":\"total\",\"ms\":\"x\"}", ImportIssue::WrongType, at + ".ms" }
        // 8 repetitions in 3.9 s: less than 0.5 s each.
        << Case{ ",\"pace\":{\"form\":\"total\",\"ms\":3900}", ImportIssue::TempoInvalid, at }
        << Case{ ",\"pace\":{\"form\":\"perRep\",\"ms\":499}", ImportIssue::TempoInvalid, at }
        << Case{ ",\"pace\":{\"form\":\"perRep\",\"ms\":60001}", ImportIssue::TempoInvalid, at }
        << Case{ ",\"pace\":{\"form\":\"total\",\"ms\":481000}", ImportIssue::TempoInvalid, at };
    for (const Case &c : cases) {
        const ImportPreview p = preview(fileV4(repsStep(c.tail)), &t_.db);
        QVERIFY2(!p.valid, qPrintable(c.tail));
        QVERIFY2(has(p.problems, c.code, c.where), qPrintable(c.tail));
    }
    // A pace on a step that ends by duration is not allowed.
    const ImportPreview timed = preview(fileV4(
        QStringLiteral("{\"type\":\"work\",\"exercise\":\"a\",\"durationSec\":30,\"pace\":{\"form\":\"perRep\",\"ms\":1000}}")), &t_.db);
    QVERIFY(!timed.valid);
    QVERIFY(has(timed.problems, ImportIssue::NotAllowed, "workouts[0].blocks[0].steps[0].pace"));
    // Limits that are accepted: 8 repetitions in 4 s (0.5 s each) and at 60 s each.
    QVERIFY(preview(fileV4(repsStep(",\"pace\":{\"form\":\"total\",\"ms\":4000}")), &t_.db).valid);
    QVERIFY(preview(fileV4(repsStep(",\"pace\":{\"form\":\"perRep\",\"ms\":60000}")), &t_.db).valid);
    QVERIFY(preview(fileV4(repsStep(",\"pace\":{\"form\":\"total\",\"ms\":480000}")), &t_.db).valid);
}

void TstExchange::onlyTheCurrentVersionIsRead()
{
    // One version is readable until the first release: others are refused, with a reason, and
    // nothing is written.
    const QString head = QStringLiteral("\"format\":\"workout-trainer-workouts\",\"version\":%1");
    QVERIFY(preview(doc(QString(), QString(), head.arg(Exchange::formatVersion)), &t_.db).valid);
    for (int older : { 1, 2, 3 }) {
        const ImportPreview p = preview(doc(QString(), QString(), head.arg(older)), &t_.db);
        QVERIFY(!p.valid);
        QVERIFY(has(p.problems, ImportIssue::UnsupportedVersion, "version"));
        QVERIFY(!p.problems.first().text().isEmpty());
    }
    const ImportPreview newer = preview(doc(QString(), QString(), head.arg(Exchange::formatVersion + 1)), &t_.db);
    QVERIFY(has(newer.problems, ImportIssue::NewerVersion, "version"));
    QVERIFY(!preview(doc(QString(), QString(), head.arg(0)), &t_.db).valid);
    QCOMPARE(Exchange::formatVersion, 4);
    QCOMPARE(Exchange::starterVersion, 1);
    QCOMPARE(countRows(t_.db, "exercise"), 0);
    QCOMPARE(countRows(t_.db, "metronome"), 0);
}

void TstExchange::aMissingMetronomeIsWrittenAsNoLink()
{
    const int mid = linkedExercise(t_.db, "a", "Doomed", Tempo::timePerRep(1000, 4));
    WorkoutStore workouts(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << repStep("a", 4)));
    w.name = "W";
    int id = 0;
    QVERIFY(workouts.create(w, &id) == StoreStatus::Ok);
    // Damaged data: the metronome row is gone while the exercise still points to it.
    QVERIFY(t_.db.run("PRAGMA foreign_keys = OFF"));
    QVERIFY(t_.db.run("DELETE FROM metronome WHERE id = ?", QVariantList() << mid));
    QByteArray bytes;
    QVERIFY(exportWorkouts(&t_.db, QList<int>() << id, &bytes));
    QCOMPARE(bytes.count("\"metronome\""), 0);
    QVERIFY(preview(bytes, &t_.db).valid);
}

// Exports a library with exercise "a" (linked to a metronome called "Pace" when withLink) and a
// workout that uses it.
static QByteArray exportOfA(Database &db, bool withLink, const Tempo &tempo = Tempo::timePerRep(1500, 4))
{
    ExerciseStore exercises(&db);
    Exercise e = makeExercise("A", muscleBit(MuscleGroup::Core));
    e.key = QStringLiteral("a");
    if (exercises.createKeyed(e) != StoreStatus::Ok)
        return QByteArray();
    if (withLink && exercises.setMetronome("a", addMetronome(db, "Pace", tempo)) != StoreStatus::Ok)
        return QByteArray();
    Workout w = workout(QList<Block>() << block(QList<Step>() << repStep("a", 4)));
    w.name = "W";
    int id = 0;
    QByteArray bytes;
    if (WorkoutStore(&db).create(w, &id) != StoreStatus::Ok || !exportWorkouts(&db, QList<int>() << id, &bytes))
        return QByteArray();
    return bytes;
}

void TstExchange::importedExerciseWithAnotherMetronomeIsAConflict()
{
    TestDb source;
    QVERIFY(source.open());
    const QByteArray bytes = exportOfA(source.db, true, Tempo::timePerRep(2500, 4));
    QVERIFY(!bytes.isEmpty());
    // The library has the same exercise with another metronome.
    QVERIFY(!exportOfA(t_.db, true, Tempo::timePerRep(1500, 4)).isEmpty());
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QCOMPARE(p.exercisesReused, 0);
    QCOMPARE(p.conflicts.size(), 1);
    QCOMPARE(p.conflicts.first().existing.key, QStringLiteral("a"));
    QCOMPARE(p.conflicts.first().workouts, QStringList() << "W");
}

void TstExchange::sameExerciseWithTheSameMetronomeIsReused()
{
    TestDb source;
    QVERIFY(source.open());
    const QByteArray bytes = exportOfA(source.db, true);
    QVERIFY(!bytes.isEmpty());
    // The library imports the file once, then again: the exercise and its metronome are reused.
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QCOMPARE(p.exercisesReused, 1);
    QVERIFY(p.conflicts.isEmpty());
    QCOMPARE(p.metronomesReused, 1);
    QCOMPARE(p.metronomesToAdd, 0);
}

void TstExchange::replaceTakesTheLinkOfTheFile()
{
    TestDb source;
    QVERIFY(source.open());
    const QByteArray bytes = exportOfA(source.db, true, Tempo::timePerRep(2500, 4));
    QVERIFY(!bytes.isEmpty());
    QVERIFY(!exportOfA(t_.db, true, Tempo::timePerRep(1500, 4)).isEmpty());
    ImportChoices choices;
    choices.insert("a", ConflictChoice::Replace);
    const ImportResult r = apply(preview(bytes, &t_.db), choices, &t_.db);
    QVERIFY(r.status == StoreStatus::Ok);
    QCOMPARE(r.exercisesReplaced, 1);
    Exercise e;
    QVERIFY(ExerciseStore(&t_.db).get("a", &e));
    SavedMetronome m;
    QVERIFY(MetronomeStore(&t_.db).get(e.metronomeId, &m));
    QVERIFY(m.tempo == rhythmOf(Tempo::timePerRep(2500, 4)));
    // The metronome that the exercise had before is still saved, now used by none.
    QCOMPARE(countRows(t_.db, "metronome"), 2);
    QVERIFY(integrityOk(t_.db));
}

void TstExchange::importAsNewKeepsTheLink()
{
    TestDb source;
    QVERIFY(source.open());
    const QByteArray bytes = exportOfA(source.db, true, Tempo::timePerRep(2500, 4));
    QVERIFY(!exportOfA(t_.db, false).isEmpty());
    ImportChoices choices;
    choices.insert("a", ConflictChoice::ImportAsNew);
    const ImportPreview p = preview(bytes, &t_.db);
    QCOMPARE(p.conflicts.size(), 1);
    const ImportResult r = apply(p, choices, &t_.db);
    QVERIFY(r.status == StoreStatus::Ok);
    QCOMPARE(r.exercisesImportedAsNew, 1);
    int linked = 0, plain = 0;
    for (const Exercise &e : ExerciseStore(&t_.db).list()) {
        if (e.metronomeId != 0)
            ++linked;
        else
            ++plain;
    }
    // The original stays without a metronome; the new exercise has the file's.
    QCOMPARE(linked, 1);
    QCOMPARE(plain, 1);
    Exercise original;
    QVERIFY(ExerciseStore(&t_.db).get("a", &original));
    QCOMPARE(original.metronomeId, 0);
}

void TstExchange::keepLeavesTheStoredLink()
{
    TestDb source;
    QVERIFY(source.open());
    const QByteArray bytes = exportOfA(source.db, false);
    QVERIFY(!exportOfA(t_.db, true).isEmpty());
    // The file's exercise has no metronome, the library's has one: a conflict whose default
    // choice keeps the library's exercise and its link.
    const ImportPreview p = preview(bytes, &t_.db);
    QCOMPARE(p.conflicts.size(), 1);
    QVERIFY(apply(p, ImportChoices(), &t_.db).status == StoreStatus::Ok);
    Exercise e;
    QVERIFY(ExerciseStore(&t_.db).get("a", &e));
    QVERIFY(e.metronomeId != 0);
}

void TstExchange::aFileWithoutALinkDiffersFromALinkedExercise()
{
    TestDb source;
    QVERIFY(source.open());
    const QByteArray bytes = exportOfA(source.db, false);
    QVERIFY(!exportOfA(t_.db, true).isEmpty());
    ImportChoices choices;
    choices.insert("a", ConflictChoice::Replace);
    QVERIFY(apply(preview(bytes, &t_.db), choices, &t_.db).status == StoreStatus::Ok);
    // Replacing with an exercise that has no metronome detaches the one the library had.
    Exercise e;
    QVERIFY(ExerciseStore(&t_.db).get("a", &e));
    QCOMPARE(e.metronomeId, 0);
    QCOMPARE(countRows(t_.db, "metronome"), 1);
}

void TstExchange::filesWithoutStartModeAreRead()
{
    const QByteArray bytes = fileWithSteps(repsStep(QString()) + "," + QStringLiteral("{\"type\":\"rest\",\"durationSec\":10}"));
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QVERIFY(p.notices.isEmpty());
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    WorkoutStore workouts(&t_.db);
    Workout w;
    QVERIFY(workouts.get(workouts.list().first().id, &w));
    for (const Step &s : w.blocks.first().steps)
        QVERIFY(!s.startOnTap);
    // An explicit false is the same as none.
    const ImportPreview q = preview(fileWithSteps(repsStep(",\"startOnTap\":false")), &t_.db);
    QVERIFY(q.valid);
    QVERIFY(!q.workouts.first().blocks.first().steps.first().startOnTap);
}

void TstExchange::invalidStartModeIsAFileProblem()
{
    const QString at = QStringLiteral("workouts[0].blocks[0].steps[0]");
    struct Case { QString step; ImportIssue::Code code; QString where; };
    const QList<Case> cases = QList<Case>()
        << Case{ repsStep(",\"startOnTap\":\"yes\""), ImportIssue::WrongType, at + ".startOnTap" }
        << Case{ repsStep(",\"startOnTap\":1"), ImportIssue::WrongType, at + ".startOnTap" }
        // The repetitions are checked as repetitions only.
        << Case{ "{\"type\":\"work\",\"exercise\":\"a\",\"reps\":1000,\"tempo\":{\"form\":\"total\",\"totalMs\":20000}}", ImportIssue::OutOfRange, at + ".reps" }
        << Case{ "{\"type\":\"work\",\"exercise\":\"a\",\"tempo\":{\"form\":\"total\",\"totalMs\":20000}}", ImportIssue::Missing, at };
    for (const Case &c : cases) {
        const ImportPreview p = preview(fileWithSteps(c.step), &t_.db);
        QVERIFY2(!p.valid, qPrintable(c.step));
        QVERIFY2(has(p.problems, c.code, c.where), qPrintable(c.step));
        QCOMPARE(p.workoutsToAdd, 0);
    }
    // Nothing of an invalid file is written.
    QCOMPARE(countRows(t_.db, "workout"), 0);
}

void TstExchange::aFlaggedFirstStepIsReadAsAutomatic()
{
    const QByteArray bytes = doc(ex("a", "A"),
        wk("One", repsStep(",\"startOnTap\":true") + "," + repsStep(",\"startOnTap\":true"))
        + "," + QStringLiteral("{\"name\":\"Two\",\"blocks\":[{\"steps\":[%1]},{\"steps\":[%2]}]}")
                    .arg(repsStep(",\"startOnTap\":true"), repsStep(",\"startOnTap\":true")));
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QVERIFY(p.problems.isEmpty());
    QVERIFY(p.notices.isEmpty());
    QCOMPARE(p.workouts.size(), 2);
    QVERIFY(!p.workouts.at(0).blocks.first().steps.at(0).startOnTap);
    QVERIFY(p.workouts.at(0).blocks.first().steps.at(1).startOnTap);
    QVERIFY(!p.workouts.at(1).blocks.at(0).steps.at(0).startOnTap);
    QVERIFY(p.workouts.at(1).blocks.at(1).steps.at(0).startOnTap);
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    WorkoutStore workouts(&t_.db);
    for (const WorkoutSummary &s : workouts.list()) {
        Workout w;
        QVERIFY(workouts.get(s.id, &w));
        QVERIFY(!w.blocks.first().steps.first().startOnTap);
    }
}

void TstExchange::exportWritesTheNormalisedFirstStep()
{
    ExerciseStore exercises(&t_.db);
    Exercise e = makeExercise("A", muscleBit(MuscleGroup::Core));
    e.key = QStringLiteral("a");
    QVERIFY(exercises.createKeyed(e) == StoreStatus::Ok);
    WorkoutStore workouts(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << work("a", 30) << work("a", 20)));
    w.name = "W";
    int id = 0;
    QVERIFY(workouts.create(w, &id) == StoreStatus::Ok);
    QVERIFY(t_.db.run("UPDATE step SET start_on_tap = 1"));     // both steps flagged in the database
    QByteArray bytes;
    QVERIFY(exportWorkouts(&t_.db, QList<int>() << id, &bytes));
    QCOMPARE(bytes.count("startOnTap"), 1);                     // only the second step
}

void TstExchange::languageIsWrittenAndRoundTrips()
{
    buildFixture(t_.db);
    QByteArray bytes;
    QVERIFY(exportAll(&t_.db, &bytes));
    QVERIFY(bytes.contains("\"language\": \"pt-br\""));
    QVERIFY(bytes.contains("\"language\": \"en\""));
    TestDb other;
    QVERIFY(other.open());
    QVERIFY(importNow(other.db, bytes).status == StoreStatus::Ok);
    Exercise e;
    QVERIFY(ExerciseStore(&other.db).get("fix-push", &e));
    QCOMPARE(e.language, QStringLiteral("pt-br"));
    QVERIFY(ExerciseStore(&other.db).get("fix-squat", &e));
    QCOMPARE(e.language, QStringLiteral("en"));
}

void TstExchange::missingLanguageIsEnglish()
{
    const QByteArray bytes = doc(ex("a", "A"), QString());
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QCOMPARE(p.exercises.first().language, QStringLiteral("en"));
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    Exercise e;
    QVERIFY(ExerciseStore(&t_.db).get("a", &e));
    QCOMPARE(e.language, QStringLiteral("en"));
}

void TstExchange::malformedLanguageMakesTheFileInvalid()
{
    const QStringList bad = QStringList() << "" << "e" << "EN" << "en_US" << "1a" << "-a" << QString(36, 'a') << "en us";
    for (const QString &tag : bad) {
        const ImportPreview p = preview(doc(ex("a", "A", "\"chest\"", ",\"language\":\"" + tag + "\""), QString()), &t_.db);
        QVERIFY2(!p.valid, qPrintable(tag));
        QVERIFY(has(p.problems, ImportIssue::LanguageInvalid, "exercises[0].language"));
    }
    ImportPreview p = preview(doc(ex("a", "A", "\"chest\"", ",\"language\":5"), QString()), &t_.db);
    QVERIFY(has(p.problems, ImportIssue::WrongType, "exercises[0].language"));
    QVERIFY(preview(doc(ex("a", "A", "\"chest\"", ",\"language\":\"" + QString(35, 'a') + "\""), QString()), &t_.db).valid);
    QVERIFY(!ImportIssue().text().isEmpty());
    ImportIssue issue;
    issue.code = ImportIssue::LanguageInvalid;
    QVERIFY(!issue.text().isEmpty());
}

void TstExchange::unknownButWellFormedLanguageIsKept()
{
    const QByteArray bytes = doc(ex("a", "A", "\"chest\"", ",\"language\":\"qq-newer-9\""), QString());
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    Exercise e;
    QVERIFY(ExerciseStore(&t_.db).get("a", &e));
    QCOMPARE(e.language, QStringLiteral("qq-newer-9"));
}

void TstExchange::differentLanguageIsAConflict()
{
    ExerciseStore exercises(&t_.db);
    Exercise mine = makeExercise("Push-up", muscleBit(MuscleGroup::UpperChest));
    mine.key = QStringLiteral("k-lang");
    QVERIFY(exercises.createKeyed(mine) == StoreStatus::Ok);
    const QString same = ex("k-lang", "Push-up", "\"upper-chest\"", ",\"instructions\":\"Do it slowly.\",\"tips\":\"Keep the back straight.\"");
    ImportPreview p = preview(doc(same, QString()), &t_.db);
    QVERIFY(p.valid);
    QVERIFY(p.conflicts.isEmpty());
    p = preview(doc(same.left(same.size() - 1) + ",\"language\":\"en\"}", QString()), &t_.db);
    QVERIFY(p.conflicts.isEmpty());
    p = preview(doc(same.left(same.size() - 1) + ",\"language\":\"de\"}", QString()), &t_.db);
    QVERIFY(p.valid);
    QCOMPARE(p.conflicts.size(), 1);
    ImportChoices replace;
    replace.insert("k-lang", ConflictChoice::Replace);
    QVERIFY(apply(p, replace, &t_.db).status == StoreStatus::Ok);
    Exercise e;
    QVERIFY(exercises.get("k-lang", &e));
    QCOMPARE(e.language, QStringLiteral("de"));
}

void TstExchange::unknownGroupsAreDroppedWithNotice()
{
    const QByteArray bytes = doc(ex("a", "A", "\"shoulders\",\"wings\",\"core\",\"tail\""), wk("W", step("a")));
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QCOMPARE(p.exercises.first().muscles, muscleBit(MuscleGroup::Shoulders) | muscleBit(MuscleGroup::Core));
    QVERIFY(has(p.notices, ImportIssue::UnknownGroup, "exercises[0].muscles[1]"));
    QVERIFY(has(p.notices, ImportIssue::UnknownGroup, "exercises[0].muscles[3]"));
    QCOMPARE(countCode(p.notices, ImportIssue::UnknownGroup), 2);
    QCOMPARE(p.workoutsToAdd, 1);
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    Exercise e;
    QVERIFY(ExerciseStore(&t_.db).get("a", &e));
    QCOMPARE(e.muscles, muscleBit(MuscleGroup::Shoulders) | muscleBit(MuscleGroup::Core));
}

void TstExchange::retiredKeysAreReadAsTheirReplacements()
{
    struct Case { const char *key; MuscleSet expected; };
    const Case cases[] = {
        { "chest", muscleBit(MuscleGroup::UpperChest) | muscleBit(MuscleGroup::LowerChest) },
        { "back", muscleBit(MuscleGroup::Trapezius) | muscleBit(MuscleGroup::Lats) | muscleBit(MuscleGroup::MidBack) },
        { "arms", muscleBit(MuscleGroup::Biceps) | muscleBit(MuscleGroup::Triceps) | muscleBit(MuscleGroup::Forearms) }
    };
    for (const Case &c : cases) {
        const QByteArray bytes = doc(ex("a", "A", QStringLiteral("\"%1\"").arg(QLatin1String(c.key))), wk("W", step("a")));
        const ImportPreview p = preview(bytes, &t_.db);
        QVERIFY(p.valid);
        QVERIFY2(p.notices.isEmpty(), c.key);
        QVERIFY(p.problems.isEmpty());
        QCOMPARE(p.exercises.first().muscles, c.expected);
        QCOMPARE(p.workoutsToAdd, 1);
    }
    // Mixed with active and unknown keys: only the unknown one is reported.
    const QByteArray mixed = doc(ex("m", "M", "\"chest\",\"core\",\"arms\",\"wings\""), QString());
    const ImportPreview p = preview(mixed, &t_.db);
    QVERIFY(p.valid);
    QCOMPARE(p.notices.size(), 1);
    QVERIFY(has(p.notices, ImportIssue::UnknownGroup, "exercises[0].muscles[3]"));
    QCOMPARE(p.exercises.first().muscles, cases[0].expected | cases[2].expected | muscleBit(MuscleGroup::Core));
    QVERIFY(importNow(t_.db, mixed).status == StoreStatus::Ok);
    Exercise stored;
    QVERIFY(ExerciseStore(&t_.db).get("m", &stored));
    QCOMPARE(stored.muscles, p.exercises.first().muscles);
}

void TstExchange::newGroupKeysSurviveARoundTrip()
{
    ExerciseStore exercises(&t_.db);
    Exercise e = makeExercise("Wide", activeMuscles());
    e.key = QStringLiteral("wide");
    QVERIFY(exercises.createKeyed(e) == StoreStatus::Ok);
    WorkoutStore workouts(&t_.db);
    Workout w = workout(QList<Block>() << block(QList<Step>() << work("wide", 30)));
    QVERIFY(workouts.create(w) == StoreStatus::Ok);

    QByteArray bytes;
    QVERIFY(exportAll(&t_.db, &bytes));
    // Active keys only, in position order.
    QStringList written;
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QCOMPARE(p.exercises.first().muscles, activeMuscles());
    for (const QString &key : MuscleKeys::keys())
        QVERIFY(bytes.contains(("\"" + key + "\"").toUtf8()));
    for (const char *retired : { "\"chest\"", "\"back\"", "\"arms\"" })
        QVERIFY(!bytes.contains(retired));
    int last = -1;
    for (const QString &key : MuscleKeys::keys()) {
        const int at = bytes.indexOf(("\"" + key + "\"").toUtf8(), bytes.indexOf("\"muscles\""));
        QVERIFY(at > last);
        last = at;
    }

    TestDb other;
    QVERIFY(other.open());
    QVERIFY(importNow(other.db, bytes).status == StoreStatus::Ok);
    Exercise read;
    QVERIFY(ExerciseStore(&other.db).get("wide", &read));
    QCOMPARE(read.muscles, activeMuscles());
    QByteArray again;
    QVERIFY(exportAll(&other.db, &again));
    QCOMPARE(again, bytes);
}

void TstExchange::exportOfALegacyStoredSetUsesActiveKeys()
{
    // A set stored before the conversion, with only retired bits, still exports as a valid file.
    ExerciseStore exercises(&t_.db);
    Exercise e = makeExercise("Old", muscleBit(MuscleGroup::Back));
    e.key = QStringLiteral("old");
    QVERIFY(exercises.createKeyed(e) == StoreStatus::Ok);
    WorkoutStore workouts(&t_.db);
    QVERIFY(workouts.create(workout(QList<Block>() << block(QList<Step>() << work("old", 30)))) == StoreStatus::Ok);
    QByteArray bytes;
    QVERIFY(exportAll(&t_.db, &bytes));
    QVERIFY(!bytes.contains("\"back\""));
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QVERIFY(p.problems.isEmpty());
    // The stored exercise equals the exported one: reused, no conflict.
    QCOMPARE(p.exercisesReused, 1);
    QVERIFY(p.conflicts.isEmpty());
}

void TstExchange::exerciseWithOnlyUnknownGroupsIsSkippedWithItsWorkouts()
{
    const QByteArray bytes = doc(ex("e1", "Known") + "," + ex("e2", "Strange", "\"wings\"") + "," + ex("e3", "Other", "\"tail\",\"fin\""),
                                 wk("Keep", step("e1")) + "," + wk("Mixed", step("e1") + "," + step("e2"))
                                     + "," + wk("Lost", step("e2")) + "," + wk("AlsoKept", step("e1")));
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QVERIFY(has(p.notices, ImportIssue::ExerciseSkipped, "exercises[1]"));
    QVERIFY(has(p.notices, ImportIssue::ExerciseSkipped, "exercises[2]"));
    QVERIFY(has(p.notices, ImportIssue::WorkoutSkipped, "workouts[1]"));
    QVERIFY(has(p.notices, ImportIssue::WorkoutSkipped, "workouts[2]"));
    QCOMPARE(countCode(p.notices, ImportIssue::WorkoutSkipped), 2);
    QCOMPARE(countCode(p.notices, ImportIssue::ExerciseSkipped), 2);
    QCOMPARE(p.workoutsToAdd, 2);
    QCOMPARE(p.exercisesToAdd, 1);
    QVERIFY(!p.nothingToImport);

    const ImportResult result = apply(p, ImportChoices(), &t_.db);
    QVERIFY(result.status == StoreStatus::Ok);
    QCOMPARE(result.workoutsAdded, 2);
    QCOMPARE(result.exercisesAdded, 1);
    QCOMPARE(countRows(t_.db, "exercise"), 1);
    QCOMPARE(countRows(t_.db, "workout"), 2);
    Exercise e;
    QVERIFY(!ExerciseStore(&t_.db).get("e2", &e));
    // The notices carry locations only, never the user's text.
    for (const ImportIssue &i : result.notices) {
        QVERIFY(!i.location.contains("Strange"));
        QVERIFY(!i.text().contains("Strange"));
    }
}

void TstExchange::fileWhereEverythingIsSkippedImportsNothing()
{
    buildFixture(t_.db);
    const QString before = snapshot(t_.db);
    const QByteArray bytes = doc(ex("e2", "Strange", "\"wings\""), wk("Lost", step("e2")));
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QVERIFY(p.nothingToImport);
    QVERIFY(has(p.notices, ImportIssue::NothingToImport, QString()));
    QCOMPARE(p.workoutsToAdd, 0);
    const ImportResult result = apply(p, ImportChoices(), &t_.db);
    QVERIFY(result.status == StoreStatus::Ok);
    QCOMPARE(result.workoutsAdded, 0);
    QCOMPARE(result.exercisesAdded, 0);
    QCOMPARE(snapshot(t_.db), before);
}

void TstExchange::emptyFileHasNothingToImport()
{
    const ImportPreview p = preview(doc(QString(), QString()), &t_.db);
    QVERIFY(p.valid);
    QVERIFY(p.nothingToImport);
}

void TstExchange::sameNameWithFreeKeyIsAddedSeparately()
{
    ExerciseStore exercises(&t_.db);
    Exercise mine = makeExercise("Push-up");
    QVERIFY(exercises.create(&mine) == StoreStatus::Ok);

    const QByteArray bytes = doc(ex("new-1", "Push-up") + "," + ex("new-2", "push-UP") + "," + ex("new-3", "Other"),
                                 wk("W", step("new-1")));
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QVERIFY(p.conflicts.isEmpty());
    QCOMPARE(p.exercisesToAdd, 3);
    QCOMPARE(p.sameNameExercises, 2);
    QCOMPARE(countCode(p.notices, ImportIssue::SameName), 1);
    for (const ImportIssue &i : p.notices) {
        if (i.code == ImportIssue::SameName)
            QCOMPARE(i.count, 2);
    }
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    QCOMPARE(countRows(t_.db, "exercise"), 4);
    QCOMPARE(exercises.list().size(), 4);
}

void TstExchange::previewListsWhatWouldHappen()
{
    ExerciseStore exercises(&t_.db);
    Exercise same = makeExercise("Same", muscleBit(MuscleGroup::Core));
    same.key = QStringLiteral("k-same");
    QVERIFY(exercises.createKeyed(same) == StoreStatus::Ok);
    Exercise differs = makeExercise("Differs");
    differs.key = QStringLiteral("k-diff");
    QVERIFY(exercises.createKeyed(differs) == StoreStatus::Ok);

    const QByteArray bytes = doc(ex("k-same", "Same", "\"core\"", ",\"instructions\":\"Do it slowly.\",\"tips\":\"Keep the back straight.\"")
                                     + "," + ex("k-diff", "Differs", "\"back\"") + "," + ex("k-new", "Brand new"),
                                 wk("One", step("k-same")) + "," + wk("Two", step("k-new")));
    const QString before = snapshot(t_.db);
    const ImportPreview p = preview(bytes, &t_.db);
    QCOMPARE(snapshot(t_.db), before);   // a preview writes nothing
    QVERIFY(p.valid);
    QCOMPARE(p.workoutsToAdd, 2);
    QCOMPARE(p.exercisesToAdd, 1);
    QCOMPARE(p.exercisesReused, 1);
    QCOMPARE(p.conflicts.size(), 1);
    QCOMPARE(p.conflicts.first().incoming.key, QStringLiteral("k-diff"));
}

void TstExchange::previewListsWorkoutsUsingAConflictingExercise()
{
    ExerciseStore exercises(&t_.db);
    Exercise mine = makeExercise("Push-up");
    mine.key = QStringLiteral("k-push");
    QVERIFY(exercises.createKeyed(mine) == StoreStatus::Ok);
    WorkoutStore workouts(&t_.db);
    Workout w1 = workout(QList<Block>() << block(QList<Step>() << work("k-push", 30)));
    w1.name = QStringLiteral("zeta");
    Workout w2 = w1;
    w2.name = QStringLiteral("Alpha");
    Workout w3 = workout(QList<Block>() << block(QList<Step>() << work("k-push", 30) << work("k-push", 20)));
    w3.name = QStringLiteral("Alpha");
    w3.colour = QStringLiteral("red");
    QVERIFY(workouts.create(w1) == StoreStatus::Ok);
    QVERIFY(workouts.create(w2) == StoreStatus::Ok);
    QVERIFY(workouts.create(w3) == StoreStatus::Ok);

    const QByteArray bytes = doc(ex("k-push", "Push-up", "\"chest\",\"arms\""), wk("Imported", step("k-push")));
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QCOMPARE(p.conflicts.size(), 1);
    // Sorted by name; each workout once per name entry of the store.
    QCOMPARE(p.conflicts.first().workouts, exercises.workoutsUsing("k-push"));
    QVERIFY(p.conflicts.first().workouts.contains("Alpha"));
    QVERIFY(p.conflicts.first().workouts.contains("zeta"));
    QCOMPARE(p.conflicts.first().workouts.first(), QStringLiteral("Alpha"));
    QCOMPARE(p.conflicts.first().existing.muscles, muscleBit(MuscleGroup::Chest));
    QCOMPARE(p.conflicts.first().incoming.muscles, MuscleKeys::convertRetired(muscleBit(MuscleGroup::Chest) | muscleBit(MuscleGroup::Arms)));
}

namespace {

// A conflict: the library has "k-push" as chest only, used by the workout "Upper"; the file has
// it as chest and arms with a new name and a workout "Imported".
QByteArray conflictFile()
{
    return doc(ex("k-push", "Push-up v2", "\"chest\",\"arms\"", ",\"icon\":\"push\",\"colour\":\"blue\""),
               wk("Imported", step("k-push")));
}

}

#define CONFLICT_SETUP \
    ExerciseStore exercises(&t_.db); \
    Exercise mine = makeExercise("Push-up"); \
    mine.key = QStringLiteral("k-push"); \
    QVERIFY(exercises.createKeyed(mine) == StoreStatus::Ok); \
    WorkoutStore workouts(&t_.db); \
    Workout upper = workout(QList<Block>() << block(QList<Step>() << work("k-push", 30))); \
    upper.name = QStringLiteral("Upper"); \
    QVERIFY(workouts.create(upper) == StoreStatus::Ok);

void TstExchange::defaultChoiceOverwritesNothing()
{
    CONFLICT_SETUP
    const ImportResult result = importNow(t_.db, conflictFile());
    QVERIFY(result.status == StoreStatus::Ok);
    QCOMPARE(result.exercisesKept, 1);
    QCOMPARE(result.exercisesReplaced, 0);
    QCOMPARE(result.exercisesImportedAsNew, 0);
    QCOMPARE(result.workoutsAdded, 1);
    Exercise e;
    QVERIFY(exercises.get("k-push", &e));
    QCOMPARE(e.name, QStringLiteral("Push-up"));
    QCOMPARE(e.muscles, muscleBit(MuscleGroup::Chest));
    QCOMPARE(e.icon, QString());
    QCOMPARE(countRows(t_.db, "exercise"), 1);

    // An explicit Keep is the same.
    ImportChoices keep;
    keep.insert("k-push", ConflictChoice::Keep);
    QVERIFY(importNow(t_.db, conflictFile(), keep).status == StoreStatus::Ok);
    QVERIFY(exercises.get("k-push", &e));
    QCOMPARE(e.name, QStringLiteral("Push-up"));
}

void TstExchange::choiceReplace()
{
    CONFLICT_SETUP
    ImportChoices choices;
    choices.insert("k-push", ConflictChoice::Replace);
    const ImportResult result = importNow(t_.db, conflictFile(), choices);
    QVERIFY(result.status == StoreStatus::Ok);
    QCOMPARE(result.exercisesReplaced, 1);
    QCOMPARE(result.workoutsAdded, 1);
    Exercise e;
    QVERIFY(exercises.get("k-push", &e));
    QCOMPARE(e.name, QStringLiteral("Push-up v2"));
    QCOMPARE(e.muscles, MuscleKeys::convertRetired(muscleBit(MuscleGroup::Chest) | muscleBit(MuscleGroup::Arms)));
    QCOMPARE(e.icon, QStringLiteral("push"));
    QCOMPARE(e.colour, QStringLiteral("blue"));
    QVERIFY(!e.starter);
    QCOMPARE(countRows(t_.db, "exercise"), 1);
    // Both workouts now use the replaced exercise.
    QCOMPARE(exercises.workoutsUsing("k-push"), QStringList() << "Imported" << "Upper");
    QVERIFY(integrityOk(t_.db));
}

void TstExchange::choiceImportAsNew()
{
    CONFLICT_SETUP
    ImportChoices choices;
    choices.insert("k-push", ConflictChoice::ImportAsNew);
    ImportResult result = importNow(t_.db, conflictFile(), choices);
    QVERIFY(result.status == StoreStatus::Ok);
    QCOMPARE(result.exercisesImportedAsNew, 1);
    QCOMPARE(countRows(t_.db, "exercise"), 2);

    Exercise original;
    QVERIFY(exercises.get("k-push", &original));
    QCOMPARE(original.name, QStringLiteral("Push-up"));
    QCOMPARE(original.muscles, muscleBit(MuscleGroup::Chest));
    QCOMPARE(exercises.workoutsUsing("k-push"), QStringList() << "Upper");

    Exercise added;
    bool found = false;
    for (const Exercise &e : exercises.list()) {
        if (e.name == QLatin1String("Push-up v2 (imported)")) {
            added = e;
            found = true;
        }
    }
    QVERIFY(found);
    QVERIFY(added.key != QLatin1String("k-push"));
    QVERIFY(!added.starter);
    QCOMPARE(added.muscles, MuscleKeys::convertRetired(muscleBit(MuscleGroup::Chest) | muscleBit(MuscleGroup::Arms)));
    QCOMPARE(added.icon, QStringLiteral("push"));
    QCOMPARE(exercises.workoutsUsing(added.key), QStringList() << "Imported");

    // A second time: the next suffix.
    result = importNow(t_.db, conflictFile(), choices);
    QVERIFY(result.status == StoreStatus::Ok);
    bool second = false;
    for (const Exercise &e : exercises.list()) {
        if (e.name == QLatin1String("Push-up v2 (imported 2)"))
            second = true;
    }
    QVERIFY(second);
    QVERIFY(integrityOk(t_.db));

    // A long name is shortened to stay within the limit.
    const QString longName(80, 'n');
    const QByteArray longFile = doc(ex("k-push", longName, "\"chest\",\"arms\""), QString());
    QVERIFY(importNow(t_.db, longFile, choices).status == StoreStatus::Ok);
    for (const Exercise &e : exercises.list())
        QVERIFY(e.name.length() <= 80);
}

void TstExchange::invalidChoicesAreRejectedBeforeWriting()
{
    CONFLICT_SETUP
    const QString before = snapshot(t_.db);
    const ImportPreview p = preview(conflictFile(), &t_.db);
    QVERIFY(p.valid);

    ImportChoices unknown;
    unknown.insert("not-a-conflict", ConflictChoice::Replace);
    ImportResult result = apply(p, unknown, &t_.db);
    QVERIFY(result.status == StoreStatus::Invalid);
    QCOMPARE(snapshot(t_.db), before);

    ImportChoices outOfRange;
    outOfRange.insert("k-push", ConflictChoice(7));
    result = apply(p, outOfRange, &t_.db);
    QVERIFY(result.status == StoreStatus::Invalid);
    QCOMPARE(snapshot(t_.db), before);

    // A valid choice next to an invalid one writes nothing either.
    ImportChoices mixed;
    mixed.insert("k-push", ConflictChoice::Replace);
    mixed.insert("other", ConflictChoice::Keep);
    QVERIFY(apply(p, mixed, &t_.db).status == StoreStatus::Invalid);
    QCOMPARE(snapshot(t_.db), before);
}

void TstExchange::applyRejectsAnInvalidPreview()
{
    const ImportPreview p = preview("garbage", &t_.db);
    QVERIFY(!p.valid);
    QVERIFY(apply(p, ImportChoices(), &t_.db).status == StoreStatus::Invalid);
    QVERIFY(apply(ImportPreview(), ImportChoices(), &t_.db).status == StoreStatus::Invalid);
}

void TstExchange::applyRechecksTheLibrary()
{
    // The library changes between the two phases: the key now exists with other content. The
    // default keeps it and nothing is overwritten.
    const QByteArray bytes = doc(ex("late", "From file", "\"core\""), wk("W", step("late")));
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QCOMPARE(p.exercisesToAdd, 1);
    ExerciseStore exercises(&t_.db);
    Exercise mine = makeExercise("Made meanwhile");
    mine.key = QStringLiteral("late");
    QVERIFY(exercises.createKeyed(mine) == StoreStatus::Ok);
    const ImportResult result = apply(p, ImportChoices(), &t_.db);
    QVERIFY(result.status == StoreStatus::Ok);
    QCOMPARE(result.exercisesKept, 1);
    Exercise e;
    QVERIFY(exercises.get("late", &e));
    QCOMPARE(e.name, QStringLiteral("Made meanwhile"));
    QCOMPARE(countRows(t_.db, "workout"), 1);

    // A reference to a library exercise that was deleted meanwhile: nothing is written.
    Exercise gone = makeExercise("Gone");
    gone.key = QStringLiteral("gone");
    QVERIFY(exercises.createKeyed(gone) == StoreStatus::Ok);
    const ImportPreview q = preview(doc(QString(), wk("X", step("gone"))), &t_.db);
    QVERIFY(q.valid);
    QVERIFY(exercises.remove("gone") == StoreStatus::Ok);
    const QString before = snapshot(t_.db);
    QVERIFY(apply(q, ImportChoices(), &t_.db).status == StoreStatus::Invalid);
    QCOMPARE(snapshot(t_.db), before);
}

void TstExchange::workoutNamesInUseGetASuffix()
{
    WorkoutStore workouts(&t_.db);
    ExerciseStore exercises(&t_.db);
    Exercise e = makeExercise("E");
    e.key = QStringLiteral("e");
    QVERIFY(exercises.createKeyed(e) == StoreStatus::Ok);
    Workout w = workout(QList<Block>() << block(QList<Step>() << work("e", 30)));
    w.name = QStringLiteral("Circuit");
    QVERIFY(workouts.create(w) == StoreStatus::Ok);
    w.name = QStringLiteral("circuit (imported)");
    QVERIFY(workouts.create(w) == StoreStatus::Ok);

    // Two workouts of the file with the same name get distinct names too.
    const QByteArray bytes = doc(QString(), wk("Circuit", step("e")) + "," + wk("Circuit", step("e")) + "," + wk("Fresh", step("e")));
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    QStringList names;
    for (const WorkoutSummary &s : workouts.list())
        names.append(s.name);
    names.sort();
    QCOMPARE(names, QStringList() << "Circuit" << "Circuit (imported 2)" << "Circuit (imported 3)" << "Fresh"
                                  << "circuit (imported)");
}

void TstExchange::applyRollsBackCompletely()
{
    ExerciseStore exercises(&t_.db);
    Exercise mine = makeExercise("Mine");
    mine.key = QStringLiteral("mine");
    QVERIFY(exercises.createKeyed(mine) == StoreStatus::Ok);
    const QByteArray bytes = doc(ex("n1", "New one") + "," + ex("n2", "New two") + "," + ex("mine", "Mine changed", "\"core\""),
                                 wk("A", step("n1") + "," + step("n2")) + "," + wk("B", step("n1") + "," + step("n2")));
    ImportChoices choices;
    choices.insert("mine", ConflictChoice::Replace);
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);

    const QString before = snapshot(t_.db);
    // The third inserted step aborts: halfway through the second workout.
    QVERIFY(t_.db.run("CREATE TRIGGER stop_here BEFORE INSERT ON step WHEN (SELECT COUNT(*) FROM step) >= 3 "
                      "BEGIN SELECT RAISE(ABORT, 'injected'); END"));
    const ImportResult result = apply(p, choices, &t_.db);
    QVERIFY(result.status == StoreStatus::Failed);
    QCOMPARE(result.workoutsAdded, 0);
    QCOMPARE(result.exercisesAdded, 0);
    QCOMPARE(snapshot(t_.db), before);
    QCOMPARE(countRows(t_.db, "exercise"), 1);
    QCOMPARE(countRows(t_.db, "workout"), 0);
    QCOMPARE(countRows(t_.db, "step"), 0);
    QVERIFY(integrityOk(t_.db));

    // Without the failure the same preview applies completely.
    QVERIFY(t_.db.run("DROP TRIGGER stop_here"));
    const ImportResult again = apply(p, choices, &t_.db);
    QVERIFY(again.status == StoreStatus::Ok);
    QCOMPARE(again.workoutsAdded, 2);
    QCOMPARE(again.exercisesAdded, 2);
    QCOMPARE(again.exercisesReplaced, 1);
    QCOMPARE(countRows(t_.db, "step"), 4);
    QVERIFY(integrityOk(t_.db));
}

void TstExchange::writerReplacesTheFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = inDir(dir, "out.json");
    QString error;
    QVERIFY(writeFile(path, "first\n", &error));
    QVERIFY(writeFile(path, "second\n", &error));
    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), QByteArray("second\n"));
    QCOMPARE(QDir(dir.path()).entryList(QDir::Files).size(), 1);
}

void TstExchange::writerLeavesTheExistingFileUntouched()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = inDir(dir, "out.json");
    QVERIFY(writeFile(path, "keep me\n"));

    // The directory cannot take a new temporary file.
    QVERIFY(QFile::setPermissions(dir.path(), QFile::ReadOwner | QFile::ExeOwner));
    QString error;
    const bool written = writeFile(path, "overwritten\n", &error);
    QFile::setPermissions(dir.path(), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    if (written)
        QSKIP("The directory permissions are not enforced for this user.");
    QVERIFY(!error.isEmpty());
    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), QByteArray("keep me\n"));
    QCOMPARE(QDir(dir.path()).entryList(QDir::Files).size(), 1);

    // A missing directory fails the same way.
    QVERIFY(!writeFile(inDir(dir, "missing/out.json"), "x", &error));
    QVERIFY(!error.isEmpty());
}

void TstExchange::truncatedFilesAreRejected()
{
    buildFixture(t_.db);
    QByteArray bytes;
    QVERIFY(exportAll(&t_.db, &bytes));
    const QString before = snapshot(t_.db);
    for (int length = 0; length < bytes.size() - 1; length += 7) {
        const ImportPreview p = preview(bytes.left(length), &t_.db);
        QVERIFY2(!p.valid, qPrintable(QString::number(length)));
    }
    QCOMPARE(snapshot(t_.db), before);
}

void TstExchange::byteOrderMarkIsAccepted()
{
    QByteArray bytes = doc(ex("a", "A"), wk("W", step("a")));
    bytes.prepend("\xEF\xBB\xBF");
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QCOMPARE(p.workoutsToAdd, 1);
}

void TstExchange::deepNestingDoesNotCrash()
{
    const int depth = 200000;
    QVERIFY(!preview(QByteArray(depth, '[') + QByteArray(depth, ']'), &t_.db).valid);
    QByteArray objects;
    for (int i = 0; i < depth / 10; ++i)
        objects += "{\"a\":";
    objects += "1";
    objects += QByteArray(depth / 10, '}');
    QVERIFY(!preview(objects, &t_.db).valid);
    // Deep structure inside a valid frame.
    const QByteArray frame = "{\"format\":\"workout-trainer-workouts\",\"version\":4,\"exercises\":[],\"workouts\":[" +
                             QByteArray(5000, '[') + QByteArray(5000, ']') + "]}";
    QVERIFY(!preview(frame, &t_.db).valid);
    QCOMPARE(countRows(t_.db, "workout"), 0);
}

void TstExchange::hugeNumbersAreRejected()
{
    const QByteArray frame = "{\"format\":\"workout-trainer-workouts\",\"version\":4,\"exercises\":[{\"key\":\"a\",\"name\":\"A\","
                             "\"muscles\":[\"chest\"]}],\"workouts\":[{\"name\":\"W\",\"restBetweenBlocksSec\":%1,"
                             "\"blocks\":[{\"rounds\":%2,\"steps\":[{\"type\":\"work\",\"exercise\":\"a\",\"reps\":%3}]}]}]}";
    const QStringList huge = QStringList() << "1e30" << "99999999999999999999999" << "-99999999999999999999999"
                                           << "4294967296" << "2147483648" << "1e999" << "-0" << "0.5";
    for (const QString &number : huge) {
        for (int slot = 0; slot < 3; ++slot) {
            QStringList values = QStringList() << "0" << "1" << "5";
            values[slot] = number;
            const QByteArray bytes = QString::fromLatin1(frame).arg(values.at(0), values.at(1), values.at(2)).toUtf8();
            const ImportPreview p = preview(bytes, &t_.db);
            if (number == QLatin1String("-0"))
                continue;   // an integral zero: valid or not, it must not crash
            QVERIFY2(!p.valid, qPrintable(number + QString::number(slot)));
        }
    }
    QCOMPARE(countRows(t_.db, "workout"), 0);
}

void TstExchange::emptyAndRandomInputAreRejected()
{
    QVERIFY(!preview(QByteArray(), &t_.db).valid);
    QVERIFY(!preview(QByteArray("   \n"), &t_.db).valid);
    QVERIFY(!preview(QByteArray("\0\0\0", 3), &t_.db).valid);
    buildFixture(t_.db);
    const QString before = snapshot(t_.db);
    std::mt19937 gen(20260101);
    for (int round = 0; round < 300; ++round) {
        QByteArray bytes;
        const int length = int(gen() % 600);
        for (int i = 0; i < length; ++i)
            bytes.append(char(gen() & 0xff));
        QVERIFY(!preview(bytes, &t_.db).valid);
    }
    QCOMPARE(snapshot(t_.db), before);
}

void TstExchange::damagedFilesNeverCrash()
{
    buildFixture(t_.db);
    QByteArray good;
    QVERIFY(exportAll(&t_.db, &good));
    const QString before = snapshot(t_.db);
    std::mt19937 gen(7);
    for (int round = 0; round < 400; ++round) {
        QByteArray bytes = good;
        const int flips = 1 + int(gen() % 3);
        for (int i = 0; i < flips; ++i)
            bytes[int(gen() % uint(bytes.size()))] = char(gen() & 0xff);
        const ImportPreview p = preview(bytes, &t_.db);
        // A preview never writes, valid or not.
        QCOMPARE(snapshot(t_.db), before);
        if (p.valid) {
            // A damaged but still valid file imports without breaking the database.
            TestDb scratch;
            QVERIFY(scratch.open());
            const ImportResult result = apply(p, ImportChoices(), &scratch.db);
            QVERIFY(result.status == StoreStatus::Ok || result.status == StoreStatus::Invalid);
            QVERIFY(integrityOk(scratch.db));
        }
    }
}

namespace {

QString exportedText(Database &db)
{
    QByteArray bytes;
    return exportAll(&db, &bytes) ? QString::fromUtf8(bytes) : QString();
}

}

void TstExchange::workoutIconAndColourAreExportedAsKeys()
{
    ExerciseStore exercises(&t_.db);
    Exercise push = makeExercise("Push-up");
    push.key = QStringLiteral("k-push");
    QVERIFY(exercises.createKeyed(push) == StoreStatus::Ok);
    Workout w = workout(QList<Block>() << block(QList<Step>() << work("k-push", 30)));
    w.name = QStringLiteral("Keyed");
    w.icon = QStringLiteral("bike");
    w.colour = QStringLiteral("teal");
    QVERIFY(WorkoutStore(&t_.db).create(w) == StoreStatus::Ok);
    const QString text = exportedText(t_.db);
    const int at = text.indexOf(QStringLiteral("\"workouts\""));
    QVERIFY(at > 0);
    const QString part = text.mid(at);
    QVERIFY(part.contains(QStringLiteral("\"icon\": \"bike\"")));
    QVERIFY(part.contains(QStringLiteral("\"colour\": \"teal\"")));
}

void TstExchange::emptyWorkoutKeysAreOmitted()
{
    buildFixture(t_.db);
    const QString text = exportedText(t_.db);
    const int at = text.indexOf(QStringLiteral("\"workouts\""));
    QVERIFY(at > 0);
    const QString part = text.mid(at);
    QVERIFY(!part.contains(QStringLiteral("\"icon\"")));
    QVERIFY(!part.contains(QStringLiteral("\"colour\"")));
}

void TstExchange::workoutIconAndColourRoundTrip()
{
    buildFixture(t_.db);
    WorkoutStore source(&t_.db);
    const QList<WorkoutSummary> list = source.list();
    QVERIFY(list.size() >= 2);
    Workout first;
    QVERIFY(source.get(list.at(0).id, &first));
    first.icon = QStringLiteral("bike");
    first.colour = QStringLiteral("purple");
    QVERIFY(source.update(first.id, first) == StoreStatus::Ok);
    Workout second;
    QVERIFY(source.get(list.at(1).id, &second));
    second.icon = QStringLiteral("some-future-icon");
    QVERIFY(source.update(second.id, second) == StoreStatus::Ok);

    QByteArray bytes;
    QVERIFY(exportAll(&t_.db, &bytes));
    TestDb other;
    QVERIFY(other.open());
    QVERIFY(importNow(other.db, bytes).status == StoreStatus::Ok);
    WorkoutStore target(&other.db);
    for (const Workout &expected : { first, second }) {
        bool found = false;
        for (const WorkoutSummary &s : target.list()) {
            Workout w;
            QVERIFY(target.get(s.id, &w));
            if (w.name != expected.name)
                continue;
            found = true;
            QCOMPARE(w.icon, expected.icon);
            QCOMPARE(w.colour, expected.colour);
            QVERIFY(sameWorkout(w, expected));
        }
        QVERIFY(found);
    }
    // The second export of the copy equals the first.
    QByteArray again;
    QVERIFY(exportAll(&other.db, &again));
    QCOMPARE(again, bytes);
}

void TstExchange::oldFileWithoutKeysImportsWithDefaults()
{
    const QByteArray bytes = doc(ex("a", "A"), wk("Old", step("a")));
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QVERIFY(p.notices.isEmpty());
    QCOMPARE(p.workouts.first().icon, QString());
    QCOMPARE(p.workouts.first().colour, QString());
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    WorkoutStore store(&t_.db);
    Workout w;
    QVERIFY(store.get(store.list().first().id, &w));
    QCOMPARE(w.name, QStringLiteral("Old"));
    QCOMPARE(w.icon, QString());
    QCOMPARE(w.colour, QString());
}

void TstExchange::unknownWorkoutColourIsDroppedWithNotice()
{
    const QByteArray bytes = doc(ex("a", "A"), wk("W", step("a"), ",\"colour\":\"magenta\""));
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QVERIFY(has(p.notices, ImportIssue::UnknownColour, "workouts[0].colour"));
    QCOMPARE(p.workouts.first().colour, QString());
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    WorkoutStore store(&t_.db);
    Workout w;
    QVERIFY(store.get(store.list().first().id, &w));
    QCOMPARE(w.colour, QString());
}

void TstExchange::unknownWorkoutIconKeyIsKept()
{
    const QByteArray bytes = doc(ex("a", "A"), wk("W", step("a"), ",\"icon\":\"not-drawn-yet\""));
    const ImportPreview p = preview(bytes, &t_.db);
    QVERIFY(p.valid);
    QVERIFY(p.notices.isEmpty());
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    WorkoutStore store(&t_.db);
    Workout w;
    QVERIFY(store.get(store.list().first().id, &w));
    QCOMPARE(w.icon, QStringLiteral("not-drawn-yet"));
}

void TstExchange::invalidWorkoutIconKeyRejectsTheFile()
{
    const QStringList bad = QStringList() << "Push" << "push up" << QString(41, 'a');
    for (const QString &icon : bad) {
        const ImportPreview p = preview(doc(ex("a", "A"), wk("W", step("a"), ",\"icon\":\"" + icon + "\"")), &t_.db);
        QVERIFY2(!p.valid, qPrintable(icon));
        QVERIFY(has(p.problems, ImportIssue::IconInvalid, "workouts[0].icon"));
    }
    const ImportPreview wrongType = preview(doc(ex("a", "A"), wk("W", step("a"), ",\"colour\":5")), &t_.db);
    QVERIFY(!wrongType.valid);
}

void TstExchange::importResolvesAnIdentityClashWithTheSuffix()
{
    WorkoutStore store(&t_.db);
    const QString key = addExercise(t_.db, "A");
    Workout w = workout(QList<Block>() << block(QList<Step>() << work(key, 30)));
    w.name = QStringLiteral("Same");
    w.icon = QStringLiteral("bike");
    w.colour = QStringLiteral("green");
    QVERIFY(store.create(w) == StoreStatus::Ok);

    const QByteArray bytes = doc(QString(), wk("same", step(key), ",\"icon\":\"bike\",\"colour\":\"green\""));
    const ImportResult first = importNow(t_.db, bytes);
    QVERIFY(first.status == StoreStatus::Ok);
    QCOMPARE(first.workoutsAdded, 1);
    const ImportResult second = importNow(t_.db, bytes);
    QVERIFY(second.status == StoreStatus::Ok);
    QStringList names;
    for (const WorkoutSummary &s : store.list())
        names.append(s.name);
    names.sort();
    QCOMPARE(names, QStringList() << "Same" << "same (imported 2)" << "same (imported)");
}

void TstExchange::importKeepsTheNameWhenIconOrColourDiffer()
{
    WorkoutStore store(&t_.db);
    const QString key = addExercise(t_.db, "A");
    Workout w = workout(QList<Block>() << block(QList<Step>() << work(key, 30)));
    w.name = QStringLiteral("Same");
    w.colour = QStringLiteral("green");
    QVERIFY(store.create(w) == StoreStatus::Ok);

    // Another colour, another icon, and no keys at all (the default colour differs from green).
    const QByteArray bytes = doc(QString(), wk("Same", step(key), ",\"colour\":\"red\"") + ","
                                   + wk("Same", step(key), ",\"colour\":\"green\",\"icon\":\"swim\"") + ","
                                   + wk("Same", step(key)));
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    QCOMPARE(store.list().size(), 4);
    for (const WorkoutSummary &s : store.list())
        QCOMPARE(s.name, QStringLiteral("Same"));
}

void TstExchange::importedCopiesOfOneFileGetDistinctNames()
{
    const QString key = addExercise(t_.db, "A");
    const QByteArray bytes = doc(QString(), wk("Twin", step(key)) + "," + wk("Twin", step(key)) + "," + wk("twin ", step(key)));
    const ImportResult r = importNow(t_.db, bytes);
    QVERIFY(r.status == StoreStatus::Ok);
    QCOMPARE(r.workoutsAdded, 3);
    QStringList names;
    for (const WorkoutSummary &s : WorkoutStore(&t_.db).list())
        names.append(s.name);
    names.sort();
    QCOMPARE(names, QStringList() << "Twin" << "Twin (imported)" << "twin (imported 2)");
}

void TstExchange::importNeverRefusesOnAClash()
{
    const QString key = addExercise(t_.db, "A");
    Workout w = workout(QList<Block>() << block(QList<Step>() << work(key, 30)));
    w.name = QString(Limits::maxNameChars, QLatin1Char('n'));
    WorkoutStore store(&t_.db);
    QVERIFY(store.create(w) == StoreStatus::Ok);
    const QByteArray bytes = doc(QString(), wk(w.name, step(key)));
    QVERIFY(importNow(t_.db, bytes).status == StoreStatus::Ok);
    bool found = false;
    for (const WorkoutSummary &s : store.list()) {
        QVERIFY(s.name.length() <= Limits::maxNameChars);
        found = found || s.name.endsWith(QStringLiteral(" (imported)"));
    }
    QVERIFY(found);
}

QTEST_APPLESS_MAIN(TstExchange)

#include "tst_exchange.moc"

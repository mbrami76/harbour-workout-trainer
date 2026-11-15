// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "colours.h"
#include "testdb.h"
#include "workoutstore.h"

class TstExerciseStore : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void createGetUpdate();
    void keysAreUniqueAndGenerated();
    void listIsSortedWithoutCase();
    void nameLimits();
    void textLimits();
    void muscleRules();
    void newGroupBitsRoundTrip();
    void highBitsAreReadBackWhole();
    void invalidWritesChangeNothing();
    void unknownKeyIsNotFound();

    void colourTable();
    void iconKeyRules();
    void colourRules();
    void languageDefaultsToEnglish();
    void languageTagRules();
    void languageRoundTripsAndUpdates();
    void cloneCopiesTheLanguage();
    void describeChangeListsTheLanguage();
    void fieldsRoundTrip();
    void unknownButValidIconIsKept();
    void starterCarriesTheFields();
    void namesNeedNotBeUnique();
    void cloneCopiesEverythingButIdentity();
    void cloneNameSuffixesInSequence();
    void cloneKeepsNamesWithinTheLimit();
    void cloneOfUnknownKeyIsNotFound();
    void cloneIsIndependent();
    void cloneLeavesWorkoutsAlone();
    void describeChangeWithoutChange();
    void describeChangeEachField();
    void describeChangeSeveralFields();
    void describeChangeListsWorkouts();
    void describeChangeUnknownKey();
    void describeChangeWritesNothing();
    void nullTextIsStoredAsEmpty();
    void workoutsUsingAndRemoval();
    void removeUnusedExercise();
    void starterIsOfferedOnce();
    void starterSurvivesDeletion();
    void starterSurvivesEdit();
    void starterFlagKeptOnEdit();
    void invalidStarterIsRejected();
    void starterWithExistingKeyIsRecordedAndUnchanged();

    void newExerciseHasNoMetronome();
    void setAndClearTheMetronome();
    void settingTheMetronomeChangesNothingElse();
    void setMetronomeRefusesAMissingMetronome();
    void unknownExerciseIsNotFoundForTheLink();
    void updateWritesTheLink();
    void updateWithoutTheLinkClearsIt();
    void createWithALinkStoresIt();
    void aDamagedLinkReadsAsNoLinkAndIsClearedOnTheNextWrite();
    void cloneKeepsTheLink();
    void cloneIsIndependentOfTheLink();
    void describeChangeListsTheMetronome();
    void tempoMapHoldsTheExercisesWithAValidMetronome();
    void deletingTheExerciseKeepsTheMetronome();

private:
    int metronome(const QString &name);
    TestDb t_;
};

void TstExerciseStore::init()
{
    QVERIFY(t_.dir.isValid());
    t_.db.close();
    QDir dir(t_.dir.path());
    dir.removeRecursively();
    QVERIFY(t_.open());
}

void TstExerciseStore::createGetUpdate()
{
    ExerciseStore store(&t_.db);
    Exercise e = makeExercise("Push-up", muscleBit(MuscleGroup::Chest) | muscleBit(MuscleGroup::Arms));
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    QVERIFY(e.key.startsWith("u-"));
    QVERIFY(!e.starter);

    Exercise read;
    QVERIFY(store.get(e.key, &read));
    QCOMPARE(read.name, QStringLiteral("Push-up"));
    QCOMPARE(read.muscles, muscleBit(MuscleGroup::Chest) | muscleBit(MuscleGroup::Arms));
    QCOMPARE(read.instructions, e.instructions);
    QCOMPARE(read.tips, e.tips);

    read.name = "Wide push-up";
    read.muscles = muscleBit(MuscleGroup::Chest);
    read.instructions = "Hands wide.";
    read.tips = "";
    QVERIFY(store.update(read) == StoreStatus::Ok);
    Exercise again;
    QVERIFY(store.get(e.key, &again));
    QCOMPARE(again.name, QStringLiteral("Wide push-up"));
    QCOMPARE(again.muscles, muscleBit(MuscleGroup::Chest));
    QCOMPARE(again.instructions, QStringLiteral("Hands wide."));
    QVERIFY(again.tips.isEmpty());
    QVERIFY(integrityOk(t_.db));
}

void TstExerciseStore::keysAreUniqueAndGenerated()
{
    ExerciseStore store(&t_.db);
    QSet<QString> keys;
    for (int i = 0; i < 20; ++i) {
        Exercise e = makeExercise("Same name");
        e.key = "ignored";
        QVERIFY(store.create(&e) == StoreStatus::Ok);
        QVERIFY(e.key != "ignored");
        keys.insert(e.key);
    }
    QCOMPARE(keys.size(), 20);
}

void TstExerciseStore::listIsSortedWithoutCase()
{
    ExerciseStore store(&t_.db);
    const QStringList input = QStringList() << "banana" << "Apple" << "cherry" << "Banana split" << "apricot";
    for (const QString &name : input) {
        Exercise e = makeExercise(name);
        QVERIFY(store.create(&e) == StoreStatus::Ok);
    }
    QStringList names;
    for (const Exercise &e : store.list())
        names << e.name;
    QCOMPARE(names, QStringList() << "Apple" << "apricot" << "banana" << "Banana split" << "cherry");
}

void TstExerciseStore::nameLimits()
{
    ExerciseStore store(&t_.db);
    QList<ExerciseProblem> problems;

    Exercise one = makeExercise("a");
    QVERIFY(store.create(&one, &problems) == StoreStatus::Ok);
    Exercise edge = makeExercise(QString(80, 'n'));
    QVERIFY(store.create(&edge, &problems) == StoreStatus::Ok);

    Exercise empty = makeExercise("");
    QVERIFY(store.create(&empty, &problems) == StoreStatus::Invalid);
    QVERIFY(problems.contains(ExerciseProblem::NameEmpty));
    Exercise blank = makeExercise("   ");
    QVERIFY(store.create(&blank, &problems) == StoreStatus::Invalid);
    QVERIFY(problems.contains(ExerciseProblem::NameEmpty));
    Exercise tooLong = makeExercise(QString(81, 'n'));
    QVERIFY(store.create(&tooLong, &problems) == StoreStatus::Invalid);
    QVERIFY(problems.contains(ExerciseProblem::NameTooLong));
}

void TstExerciseStore::textLimits()
{
    ExerciseStore store(&t_.db);
    QList<ExerciseProblem> problems;

    Exercise ok = makeExercise("Text");
    ok.instructions = QString(4000, 'i');
    ok.tips = QString(2000, 't');
    QVERIFY(store.create(&ok, &problems) == StoreStatus::Ok);

    Exercise longInstructions = makeExercise("Text");
    longInstructions.instructions = QString(4001, 'i');
    QVERIFY(store.create(&longInstructions, &problems) == StoreStatus::Invalid);
    QVERIFY(problems.contains(ExerciseProblem::InstructionsTooLong));

    Exercise longTips = makeExercise("Text");
    longTips.tips = QString(2001, 't');
    QVERIFY(store.create(&longTips, &problems) == StoreStatus::Invalid);
    QVERIFY(problems.contains(ExerciseProblem::TipsTooLong));
}

void TstExerciseStore::muscleRules()
{
    ExerciseStore store(&t_.db);
    QList<ExerciseProblem> problems;

    Exercise none = makeExercise("None", 0);
    QVERIFY(store.create(&none, &problems) == StoreStatus::Invalid);
    QVERIFY(problems.contains(ExerciseProblem::NoMuscles));

    Exercise all = makeExercise("All", allMuscles());
    QVERIFY(store.create(&all, &problems) == StoreStatus::Ok);
    Exercise last = makeExercise("Last", muscleBit(MuscleGroup::Calves));
    QVERIFY(store.create(&last, &problems) == StoreStatus::Ok);

    Exercise beyond = makeExercise("Beyond", allMuscles() + 1);
    QVERIFY(store.create(&beyond, &problems) == StoreStatus::Invalid);
    QVERIFY(problems.contains(ExerciseProblem::UnknownMuscles));
    Exercise high = makeExercise("High", MuscleSet(1) << 31);
    QVERIFY(store.create(&high, &problems) == StoreStatus::Invalid);
    QVERIFY(problems.contains(ExerciseProblem::UnknownMuscles));
    Exercise mixed = makeExercise("Mixed", muscleBit(MuscleGroup::Core) | (MuscleSet(1) << 25));
    QVERIFY(store.create(&mixed, &problems) == StoreStatus::Invalid);
}

void TstExerciseStore::newGroupBitsRoundTrip()
{
    ExerciseStore store(&t_.db);
    const MuscleSet all18 = activeMuscles();
    Exercise wide = makeExercise("Wide", all18);
    QVERIFY(store.create(&wide) == StoreStatus::Ok);
    Exercise read;
    QVERIFY(store.get(wide.key, &read));
    QCOMPARE(read.muscles, all18);

    Exercise top = makeExercise("Top", muscleBit(MuscleGroup::Shins) | muscleBit(MuscleGroup::Forearms)
                                           | muscleBit(MuscleGroup::LowerAbdomen));
    QVERIFY(store.create(&top) == StoreStatus::Ok);
    QVERIFY(store.get(top.key, &read));
    QCOMPARE(read.muscles, top.muscles);
    top.muscles = muscleBit(MuscleGroup::Obliques) | muscleBit(MuscleGroup::MidBack);
    QVERIFY(store.update(top) == StoreStatus::Ok);
    QVERIFY(store.get(top.key, &read));
    QCOMPARE(read.muscles, top.muscles);
    QVERIFY(integrityOk(t_.db));
}

void TstExerciseStore::highBitsAreReadBackWhole()
{
    // A set written by a newer version may use positions above 31; reading must not cut it.
    const MuscleSet wide = (MuscleSet(1) << 62) | (MuscleSet(1) << 40) | muscleBit(MuscleGroup::Core);
    QVERIFY(t_.db.run("INSERT INTO exercise (key, name, muscles) VALUES ('wide', 'Wide', ?)", QVariantList() << wide));
    ExerciseStore store(&t_.db);
    Exercise read;
    QVERIFY(store.get("wide", &read));
    QCOMPARE(read.muscles, wide);
    QCOMPARE(store.list().first().muscles, wide);
    // A new write cannot use a position outside the table.
    Exercise outside = makeExercise("Outside", MuscleSet(1) << 40);
    QList<ExerciseProblem> problems;
    QVERIFY(store.create(&outside, &problems) == StoreStatus::Invalid);
    QVERIFY(problems.contains(ExerciseProblem::UnknownMuscles));
}

void TstExerciseStore::invalidWritesChangeNothing()
{
    ExerciseStore store(&t_.db);
    Exercise e = makeExercise("Original");
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    Exercise bad = e;
    bad.name = "";
    QVERIFY(store.update(bad) == StoreStatus::Invalid);
    bad = e;
    bad.muscles = 0;
    QVERIFY(store.update(bad) == StoreStatus::Invalid);
    Exercise read;
    QVERIFY(store.get(e.key, &read));
    QCOMPARE(read.name, QStringLiteral("Original"));
    QCOMPARE(read.muscles, e.muscles);
    QCOMPARE(countRows(t_.db, "exercise"), 1);
}

void TstExerciseStore::nullTextIsStoredAsEmpty()
{
    ExerciseStore store(&t_.db);
    Exercise e;
    e.name = "No texts";
    e.muscles = muscleBit(MuscleGroup::Core);
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    Exercise read;
    QVERIFY(store.get(e.key, &read));
    QVERIFY(read.instructions.isEmpty());
    QVERIFY(read.tips.isEmpty());
}

void TstExerciseStore::unknownKeyIsNotFound()
{
    ExerciseStore store(&t_.db);
    Exercise e = makeExercise("Ghost");
    e.key = "u-missing";
    QVERIFY(store.update(e) == StoreStatus::NotFound);
    QVERIFY(store.remove("u-missing") == StoreStatus::NotFound);
    Exercise read;
    QVERIFY(!store.get("u-missing", &read));
    QVERIFY(store.workoutsUsing("u-missing").isEmpty());
}

void TstExerciseStore::workoutsUsingAndRemoval()
{
    ExerciseStore exercises(&t_.db);
    WorkoutStore workouts(&t_.db);
    const QString a = addExercise(t_.db, "A");
    const QString b = addExercise(t_.db, "B");

    // W1: block 1 has A, a rest and B; block 2 has only A. W2 uses B. W3 uses only A.
    Workout w1 = workout(QList<Block>()
        << block(QList<Step>() << work(a, 30) << rest(10) << work(b, 30))
        << block(QList<Step>() << work(a, 20)));
    w1.name = "W1";
    Workout w2 = workout(QList<Block>() << block(QList<Step>() << work(b, 30)));
    w2.name = "w2";
    Workout w3 = workout(QList<Block>() << block(QList<Step>() << work(a, 30), 3, 5));
    w3.name = "W3";
    int id1 = 0, id2 = 0, id3 = 0;
    QVERIFY(workouts.create(w1, &id1) == StoreStatus::Ok);
    QVERIFY(workouts.create(w2, &id2) == StoreStatus::Ok);
    QVERIFY(workouts.create(w3, &id3) == StoreStatus::Ok);

    QCOMPARE(exercises.workoutsUsing(a), QStringList() << "W1" << "W3");
    QCOMPARE(exercises.workoutsUsing(b), QStringList() << "W1" << "w2");

    QVERIFY(exercises.remove(a) == StoreStatus::Ok);
    QVERIFY(exercises.workoutsUsing(a).isEmpty());
    Exercise gone;
    QVERIFY(!exercises.get(a, &gone));

    Workout read;
    QVERIFY(workouts.get(id1, &read));
    QCOMPARE(read.blocks.size(), 1);
    QCOMPARE(read.blocks.first().steps.size(), 2);
    QVERIFY(read.blocks.first().steps.at(0).type == StepType::Rest);
    QCOMPARE(read.blocks.first().steps.at(1).exerciseKey, b);
    QVERIFY(validate(read).isEmpty());

    QVERIFY(workouts.get(id2, &read));
    QCOMPARE(read.blocks.size(), 1);

    // W3 is left without blocks: it stays and fails validation until edited.
    QVERIFY(workouts.get(id3, &read));
    QVERIFY(read.blocks.isEmpty());
    QVERIFY(hasProblem(validate(read), Problem::NoBlocks));
    QCOMPARE(workouts.list().size(), 3);
    QVERIFY(integrityOk(t_.db));
}

void TstExerciseStore::removeUnusedExercise()
{
    ExerciseStore store(&t_.db);
    const QString key = addExercise(t_.db, "Lonely");
    QVERIFY(store.remove(key) == StoreStatus::Ok);
    QCOMPARE(countRows(t_.db, "exercise"), 0);
}

static Exercise starter(const QString &key, const QString &name)
{
    Exercise e = makeExercise(name, muscleBit(MuscleGroup::Core));
    e.key = key;
    e.starterId = key.mid(8);
    return e;
}

void TstExerciseStore::starterIsOfferedOnce()
{
    ExerciseStore store(&t_.db);
    bool inserted = false;
    QVERIFY(store.offerStarter(starter("starter-plank", "Plank"), &inserted) == StoreStatus::Ok);
    QVERIFY(inserted);
    Exercise read;
    QVERIFY(store.get("starter-plank", &read));
    QVERIFY(read.starter);
    QCOMPARE(read.name, QStringLiteral("Plank"));

    QVERIFY(store.offerStarter(starter("starter-plank", "Plank v2"), &inserted) == StoreStatus::Ok);
    QVERIFY(!inserted);
    QVERIFY(store.get("starter-plank", &read));
    QCOMPARE(read.name, QStringLiteral("Plank"));
    QCOMPARE(countRows(t_.db, "exercise"), 1);
    QCOMPARE(countRows(t_.db, "starter_offered"), 1);

    QVERIFY(store.offerStarter(starter("starter-bridge", "Bridge"), &inserted) == StoreStatus::Ok);
    QVERIFY(inserted);
    QCOMPARE(countRows(t_.db, "exercise"), 2);
}

void TstExerciseStore::starterSurvivesDeletion()
{
    ExerciseStore store(&t_.db);
    bool inserted = false;
    QVERIFY(store.offerStarter(starter("starter-plank", "Plank"), &inserted) == StoreStatus::Ok);
    QVERIFY(store.remove("starter-plank") == StoreStatus::Ok);
    QVERIFY(store.offerStarter(starter("starter-plank", "Plank"), &inserted) == StoreStatus::Ok);
    QVERIFY(!inserted);
    Exercise read;
    QVERIFY(!store.get("starter-plank", &read));
}

void TstExerciseStore::starterSurvivesEdit()
{
    ExerciseStore store(&t_.db);
    bool inserted = false;
    QVERIFY(store.offerStarter(starter("starter-plank", "Plank"), &inserted) == StoreStatus::Ok);
    Exercise edited;
    QVERIFY(store.get("starter-plank", &edited));
    edited.name = "My plank";
    edited.instructions = "Mine.";
    QVERIFY(store.update(edited) == StoreStatus::Ok);

    QVERIFY(store.offerStarter(starter("starter-plank", "Plank"), &inserted) == StoreStatus::Ok);
    QVERIFY(!inserted);
    Exercise read;
    QVERIFY(store.get("starter-plank", &read));
    QCOMPARE(read.name, QStringLiteral("My plank"));
    QCOMPARE(read.instructions, QStringLiteral("Mine."));
}

void TstExerciseStore::starterFlagKeptOnEdit()
{
    ExerciseStore store(&t_.db);
    QVERIFY(store.offerStarter(starter("starter-plank", "Plank")) == StoreStatus::Ok);
    Exercise e;
    QVERIFY(store.get("starter-plank", &e));
    e.starter = false;
    e.name = "Renamed";
    QVERIFY(store.update(e) == StoreStatus::Ok);
    QVERIFY(store.get("starter-plank", &e));
    QVERIFY(e.starter);

    Exercise user = makeExercise("User");
    user.starter = true;
    QVERIFY(store.create(&user) == StoreStatus::Ok);
    QVERIFY(!user.starter);
    QVERIFY(store.get(user.key, &e));
    QVERIFY(!e.starter);
}

void TstExerciseStore::invalidStarterIsRejected()
{
    ExerciseStore store(&t_.db);
    bool inserted = true;
    QVERIFY(store.offerStarter(starter("", "No key"), &inserted) == StoreStatus::Invalid);
    QVERIFY(!inserted);
    Exercise bad = starter("starter-bad", "");
    QVERIFY(store.offerStarter(bad, &inserted) == StoreStatus::Invalid);
    QCOMPARE(countRows(t_.db, "exercise"), 0);
    QCOMPARE(countRows(t_.db, "starter_offered"), 0);
}

static Exercise full(const QString &name)
{
    Exercise e = makeExercise(name, muscleBit(MuscleGroup::Back) | muscleBit(MuscleGroup::Arms));
    e.icon = "pull-up";
    e.colour = "teal";
    e.language = "pt-br";
    return e;
}

void TstExerciseStore::colourTable()
{
    const QStringList keys = Colours::keys();
    QCOMPARE(keys, QStringList() << "red" << "orange" << "yellow" << "green" << "teal"
                                 << "blue" << "purple" << "pink" << "brown" << "grey");
    for (const QString &key : keys)
        QVERIFY(Colours::isKnown(key));
    QVERIFY(!Colours::isKnown(""));
    QVERIFY(!Colours::isKnown("Red"));
    QVERIFY(!Colours::isKnown("magenta"));
    QVERIFY(!Colours::isKnown(" red"));
}

void TstExerciseStore::iconKeyRules()
{
    ExerciseStore store(&t_.db);
    QList<ExerciseProblem> problems;
    const QStringList good = QStringList() << "" << "a" << "push-up" << "x1-2-3" << QString(40, 'a');
    for (const QString &icon : good) {
        Exercise e = makeExercise("Icon");
        e.icon = icon;
        QVERIFY2(store.create(&e, &problems) == StoreStatus::Ok, qPrintable(icon));
    }
    const QStringList bad = QStringList() << QString(41, 'a') << "Push-up" << "push up" << "push_up"
                                          << "push.up" << QString(QChar(0x00fc)) << " " << "a/b";
    for (const QString &icon : bad) {
        Exercise e = makeExercise("Icon");
        e.icon = icon;
        QVERIFY2(store.create(&e, &problems) == StoreStatus::Invalid, qPrintable(icon));
        QVERIFY(problems.contains(ExerciseProblem::IconInvalid));
    }
    QCOMPARE(countRows(t_.db, "exercise"), good.size());
}

void TstExerciseStore::languageDefaultsToEnglish()
{
    ExerciseStore store(&t_.db);
    Exercise e = makeExercise("Plain");
    QCOMPARE(e.language, QStringLiteral("en"));
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    Exercise back;
    QVERIFY(store.get(e.key, &back));
    QCOMPARE(back.language, QStringLiteral("en"));
    // A row written without the column gets the default.
    QVERIFY(t_.db.run("INSERT INTO exercise (key, name, muscles) VALUES ('raw', 'Raw', 4)"));
    QVERIFY(store.get("raw", &back));
    QCOMPARE(back.language, QStringLiteral("en"));
}

void TstExerciseStore::languageTagRules()
{
    ExerciseStore store(&t_.db);
    QList<ExerciseProblem> problems;
    const QStringList good = QStringList() << "en" << "pt-br" << "zh-hant-tw" << "x1" << "ab-12-cd"
                                           << QString(35, 'a');
    for (const QString &tag : good) {
        Exercise e = makeExercise("Lang");
        e.language = tag;
        QVERIFY2(store.create(&e, &problems) == StoreStatus::Ok, qPrintable(tag));
        QVERIFY(ExerciseStore::isValidLanguage(tag));
    }
    const QStringList bad = QStringList() << "" << "e" << QString(36, 'a') << "EN" << "pt_BR" << "1a" << "-a"
                                          << "en us" << "en." << QString(QChar(0x00e9)) + "n" << "a\n";
    for (const QString &tag : bad) {
        Exercise e = makeExercise("Lang");
        e.language = tag;
        QVERIFY2(store.create(&e, &problems) == StoreStatus::Invalid, qPrintable(tag));
        QVERIFY(problems.contains(ExerciseProblem::LanguageInvalid));
        QVERIFY(!ExerciseStore::isValidLanguage(tag));
    }
    QCOMPARE(countRows(t_.db, "exercise"), good.size());

    // An invalid tag on update changes nothing.
    Exercise e = makeExercise("Keep");
    e.language = "fr";
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    e.language = "FR";
    QVERIFY(store.update(e, &problems) == StoreStatus::Invalid);
    Exercise back;
    QVERIFY(store.get(e.key, &back));
    QCOMPARE(back.language, QStringLiteral("fr"));
}

void TstExerciseStore::languageRoundTripsAndUpdates()
{
    ExerciseStore store(&t_.db);
    Exercise e = makeExercise("Lang");
    e.language = "it";
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    Exercise back;
    QVERIFY(store.get(e.key, &back));
    QCOMPARE(back.language, QStringLiteral("it"));
    e.language = "es-419";
    QVERIFY(store.update(e) == StoreStatus::Ok);
    QVERIFY(store.get(e.key, &back));
    QCOMPARE(back.language, QStringLiteral("es-419"));
    bool found = false;
    for (const Exercise &x : store.list())
        found = found || (x.key == e.key && x.language == QLatin1String("es-419"));
    QVERIFY(found);
    // A well-formed tag that no list knows is kept.
    e.language = "qq-unknown";
    QVERIFY(store.update(e) == StoreStatus::Ok);
    QVERIFY(store.get(e.key, &back));
    QCOMPARE(back.language, QStringLiteral("qq-unknown"));
}

void TstExerciseStore::cloneCopiesTheLanguage()
{
    ExerciseStore store(&t_.db);
    Exercise e = makeExercise("Lang");
    e.language = "ja";
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    QString key;
    QVERIFY(store.clone(e.key, &key) == StoreStatus::Ok);
    Exercise copy;
    QVERIFY(store.get(key, &copy));
    QCOMPARE(copy.language, QStringLiteral("ja"));
}

void TstExerciseStore::describeChangeListsTheLanguage()
{
    ExerciseStore store(&t_.db);
    Exercise e = makeExercise("Lang");
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    Exercise edited = e;
    edited.language = "fr";
    edited.name = "Lang 2";
    ExerciseChange change;
    QVERIFY(store.describeChange(edited, &change) == StoreStatus::Ok);
    QCOMPARE(change.fields.size(), 2);
    QVERIFY(change.fields.contains(ExerciseField::Language));
    QVERIFY(change.fields.contains(ExerciseField::Name));
}

void TstExerciseStore::colourRules()
{
    ExerciseStore store(&t_.db);
    QList<ExerciseProblem> problems;
    for (const QString &colour : Colours::keys()) {
        Exercise e = makeExercise("Colour");
        e.colour = colour;
        QVERIFY(store.create(&e, &problems) == StoreStatus::Ok);
    }
    Exercise none = makeExercise("Colour");
    QVERIFY(store.create(&none, &problems) == StoreStatus::Ok);

    const QStringList bad = QStringList() << "magenta" << "Red" << "red " << "#ff0000" << "default";
    for (const QString &colour : bad) {
        Exercise e = makeExercise("Colour");
        e.colour = colour;
        QVERIFY2(store.create(&e, &problems) == StoreStatus::Invalid, qPrintable(colour));
        QVERIFY(problems.contains(ExerciseProblem::ColourUnknown));
    }
    Exercise edited = none;
    edited.colour = "magenta";
    QVERIFY(store.update(edited) == StoreStatus::Invalid);
    Exercise read;
    QVERIFY(store.get(none.key, &read));
    QVERIFY(read.colour.isEmpty());
}

void TstExerciseStore::fieldsRoundTrip()
{
    ExerciseStore store(&t_.db);
    Exercise e = full("Pull-up");
    QVERIFY(store.create(&e) == StoreStatus::Ok);

    Exercise read;
    QVERIFY(store.get(e.key, &read));
    QCOMPARE(read.icon, QStringLiteral("pull-up"));
    QCOMPARE(read.colour, QStringLiteral("teal"));
    const QList<Exercise> all = store.list();
    QCOMPARE(all.size(), 1);
    QCOMPARE(all.first().icon, QStringLiteral("pull-up"));
    QCOMPARE(all.first().colour, QStringLiteral("teal"));

    read.icon = "chin-up";
    read.colour = "orange";
    QVERIFY(store.update(read) == StoreStatus::Ok);
    Exercise again;
    QVERIFY(store.get(e.key, &again));
    QCOMPARE(again.icon, QStringLiteral("chin-up"));
    QCOMPARE(again.colour, QStringLiteral("orange"));

    again.icon.clear();
    again.colour.clear();
    QVERIFY(store.update(again) == StoreStatus::Ok);
    QVERIFY(store.get(e.key, &read));
    QVERIFY(read.icon.isEmpty());
    QVERIFY(read.colour.isEmpty());

    Exercise plain = makeExercise("Plain");
    QVERIFY(store.create(&plain) == StoreStatus::Ok);
    QVERIFY(store.get(plain.key, &read));
    QVERIFY(read.icon.isEmpty());
    QVERIFY(read.colour.isEmpty());
    QVERIFY(integrityOk(t_.db));
}

void TstExerciseStore::unknownButValidIconIsKept()
{
    // The store does not know the bundled picture set: a key from a newer version survives.
    ExerciseStore store(&t_.db);
    Exercise e = makeExercise("Future");
    e.icon = "icon-from-a-newer-version";
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    e.name = "Future renamed";
    QVERIFY(store.update(e) == StoreStatus::Ok);
    Exercise read;
    QVERIFY(store.get(e.key, &read));
    QCOMPARE(read.icon, QStringLiteral("icon-from-a-newer-version"));
}

void TstExerciseStore::starterCarriesTheFields()
{
    ExerciseStore store(&t_.db);
    Exercise s = full("Starter pull-up");
    s.key = "starter-pull-up";
    s.starterId = "pull-up";
    bool inserted = false;
    QVERIFY(store.offerStarter(s, &inserted) == StoreStatus::Ok);
    QVERIFY(inserted);
    Exercise read;
    QVERIFY(store.get("starter-pull-up", &read));
    QVERIFY(read.starter);
    QCOMPARE(read.icon, QStringLiteral("pull-up"));
    QCOMPARE(read.colour, QStringLiteral("teal"));

    read.icon = "mine";
    read.colour = "pink";
    QVERIFY(store.update(read) == StoreStatus::Ok);
    QVERIFY(store.offerStarter(s, &inserted) == StoreStatus::Ok);
    QVERIFY(!inserted);
    QVERIFY(store.get("starter-pull-up", &read));
    QVERIFY(read.starter);
    QCOMPARE(read.icon, QStringLiteral("mine"));
    QCOMPARE(read.colour, QStringLiteral("pink"));

    Exercise badStarter = full("Bad");
    badStarter.key = "starter-bad";
    badStarter.starterId = "bad";
    badStarter.colour = "magenta";
    QVERIFY(store.offerStarter(badStarter, &inserted) == StoreStatus::Invalid);
    badStarter.colour = "red";
    badStarter.icon = "Bad Icon";
    QVERIFY(store.offerStarter(badStarter, &inserted) == StoreStatus::Invalid);
    QCOMPARE(countRows(t_.db, "starter_offered"), 1);
}

void TstExerciseStore::namesNeedNotBeUnique()
{
    ExerciseStore store(&t_.db);
    Exercise a = makeExercise("Same");
    Exercise b = makeExercise("Same");
    Exercise c = makeExercise("same");
    QVERIFY(store.create(&a) == StoreStatus::Ok);
    QVERIFY(store.create(&b) == StoreStatus::Ok);
    QVERIFY(store.create(&c) == StoreStatus::Ok);
    QVERIFY(a.key != b.key);

    Exercise other = makeExercise("Other");
    QVERIFY(store.create(&other) == StoreStatus::Ok);
    other.name = "Same";
    QVERIFY(store.update(other) == StoreStatus::Ok);
    other.name = "SAME";
    QVERIFY(store.update(other) == StoreStatus::Ok);
    QCOMPARE(store.list().size(), 4);
    QVERIFY(integrityOk(t_.db));
}

void TstExerciseStore::cloneCopiesEverythingButIdentity()
{
    ExerciseStore store(&t_.db);
    Exercise e = full("Pull-up");
    QVERIFY(store.create(&e) == StoreStatus::Ok);

    QString key;
    QVERIFY(store.clone(e.key, &key) == StoreStatus::Ok);
    QVERIFY(!key.isEmpty());
    QVERIFY(key != e.key);
    QVERIFY(key.startsWith("u-"));
    Exercise copy;
    QVERIFY(store.get(key, &copy));
    QCOMPARE(copy.name, QStringLiteral("Pull-up (copy)"));
    QCOMPARE(copy.muscles, e.muscles);
    QCOMPARE(copy.instructions, e.instructions);
    QCOMPARE(copy.tips, e.tips);
    QCOMPARE(copy.icon, e.icon);
    QCOMPARE(copy.colour, e.colour);
    QCOMPARE(copy.language, e.language);
    QVERIFY(!copy.starter);
    QCOMPARE(countRows(t_.db, "exercise"), 2);

    // A starter entry gives a user entry.
    Exercise s = full("Starter");
    s.key = "starter-x";
    s.starterId = "x";
    QVERIFY(store.offerStarter(s) == StoreStatus::Ok);
    QVERIFY(store.clone("starter-x", &key) == StoreStatus::Ok);
    QVERIFY(store.get(key, &copy));
    QVERIFY(!copy.starter);
    Exercise original;
    QVERIFY(store.get("starter-x", &original));
    QVERIFY(original.starter);
    QVERIFY(store.clone(e.key) == StoreStatus::Ok);
    QVERIFY(integrityOk(t_.db));
}

void TstExerciseStore::cloneNameSuffixesInSequence()
{
    ExerciseStore store(&t_.db);
    Exercise e = full("Squat");
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    QStringList names;
    QString first;
    for (int i = 0; i < 4; ++i) {
        QString key;
        QVERIFY(store.clone(e.key, &key) == StoreStatus::Ok);
        if (i == 0)
            first = key;
        Exercise copy;
        QVERIFY(store.get(key, &copy));
        names << copy.name;
    }
    QCOMPARE(names, QStringList() << "Squat (copy)" << "Squat (copy 2)" << "Squat (copy 3)" << "Squat (copy 4)");

    // A clone of a clone extends its own name; the name check ignores case.
    QString key;
    QVERIFY(store.clone(first, &key) == StoreStatus::Ok);
    Exercise copy;
    QVERIFY(store.get(key, &copy));
    QCOMPARE(copy.name, QStringLiteral("Squat (copy) (copy)"));

    Exercise lower = makeExercise("lunge (COPY)");
    QVERIFY(store.create(&lower) == StoreStatus::Ok);
    Exercise lunge = makeExercise("Lunge");
    QVERIFY(store.create(&lunge) == StoreStatus::Ok);
    QVERIFY(store.clone(lunge.key, &key) == StoreStatus::Ok);
    QVERIFY(store.get(key, &copy));
    QCOMPARE(copy.name, QStringLiteral("Lunge (copy 2)"));
}

void TstExerciseStore::cloneKeepsNamesWithinTheLimit()
{
    ExerciseStore store(&t_.db);
    Exercise e = makeExercise(QString(80, 'x'));
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    for (int i = 0; i < 12; ++i) {
        QString key;
        QVERIFY(store.clone(e.key, &key) == StoreStatus::Ok);
        Exercise copy;
        QVERIFY(store.get(key, &copy));
        QVERIFY(copy.name.length() <= 80);
        QVERIFY(copy.name.startsWith("xxx"));
    }
    QStringList names;
    for (const Exercise &x : store.list())
        names << x.name.toLower();
    names.removeDuplicates();
    QCOMPARE(names.size(), 13);
}

void TstExerciseStore::cloneOfUnknownKeyIsNotFound()
{
    ExerciseStore store(&t_.db);
    QString key = "unchanged";
    QVERIFY(store.clone("u-missing", &key) == StoreStatus::NotFound);
    QCOMPARE(key, QStringLiteral("unchanged"));
    QCOMPARE(countRows(t_.db, "exercise"), 0);
}

void TstExerciseStore::cloneIsIndependent()
{
    ExerciseStore store(&t_.db);
    Exercise e = full("Row");
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    QString key;
    QVERIFY(store.clone(e.key, &key) == StoreStatus::Ok);

    Exercise original, copy;
    QVERIFY(store.get(e.key, &original));
    original.name = "Row changed";
    original.muscles = muscleBit(MuscleGroup::Core);
    original.icon = "other";
    original.colour = "red";
    QVERIFY(store.update(original) == StoreStatus::Ok);
    QVERIFY(store.get(key, &copy));
    QCOMPARE(copy.name, QStringLiteral("Row (copy)"));
    QCOMPARE(copy.muscles, e.muscles);
    QCOMPARE(copy.icon, e.icon);
    QCOMPARE(copy.colour, e.colour);

    copy.instructions = "Changed copy.";
    QVERIFY(store.update(copy) == StoreStatus::Ok);
    QVERIFY(store.get(e.key, &original));
    QCOMPARE(original.instructions, e.instructions);

    QVERIFY(store.remove(e.key) == StoreStatus::Ok);
    QVERIFY(store.get(key, &copy));
    QCOMPARE(copy.instructions, QStringLiteral("Changed copy."));
    QVERIFY(integrityOk(t_.db));
}

void TstExerciseStore::cloneLeavesWorkoutsAlone()
{
    ExerciseStore store(&t_.db);
    WorkoutStore workouts(&t_.db);
    const QString a = addExercise(t_.db, "A");
    Workout w = workout(QList<Block>() << block(QList<Step>() << work(a, 30) << rest(10) << work(a, 20), 2, 5));
    w.name = "Uses A";
    int id = 0;
    QVERIFY(workouts.create(w, &id) == StoreStatus::Ok);

    QString key;
    QVERIFY(store.clone(a, &key) == StoreStatus::Ok);
    QCOMPARE(store.workoutsUsing(a), QStringList() << "Uses A");
    QVERIFY(store.workoutsUsing(key).isEmpty());
    Workout read;
    QVERIFY(workouts.get(id, &read));
    QVERIFY(sameWorkout(read, w));
    QCOMPARE(countRows(t_.db, "step"), 3);
    QCOMPARE(countRows(t_.db, "block"), 1);
}

void TstExerciseStore::describeChangeWithoutChange()
{
    ExerciseStore store(&t_.db);
    Exercise e = full("Plank");
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    ExerciseChange change;
    change.fields << ExerciseField::Name;
    change.workouts << "stale";
    QVERIFY(store.describeChange(e, &change) == StoreStatus::Ok);
    QVERIFY(change.fields.isEmpty());
    QVERIFY(change.workouts.isEmpty());
}

void TstExerciseStore::describeChangeEachField()
{
    ExerciseStore store(&t_.db);
    Exercise e = full("Plank");
    QVERIFY(store.create(&e) == StoreStatus::Ok);

    struct Case { ExerciseField field; Exercise edited; };
    QList<Case> cases;
    Exercise x = e; x.name = "Plank 2"; cases.append({ ExerciseField::Name, x });
    x = e; x.muscles = muscleBit(MuscleGroup::Glutes); cases.append({ ExerciseField::Muscles, x });
    x = e; x.instructions = "Other"; cases.append({ ExerciseField::Instructions, x });
    x = e; x.tips = "Other"; cases.append({ ExerciseField::Tips, x });
    x = e; x.icon = ""; cases.append({ ExerciseField::Icon, x });
    x = e; x.colour = "red"; cases.append({ ExerciseField::Colour, x });
    x = e; x.language = "de"; cases.append({ ExerciseField::Language, x });
    for (const Case &c : cases) {
        ExerciseChange change;
        QVERIFY(store.describeChange(c.edited, &change) == StoreStatus::Ok);
        QCOMPARE(change.fields.size(), 1);
        QVERIFY(change.fields.first() == c.field);
    }
}

void TstExerciseStore::describeChangeSeveralFields()
{
    ExerciseStore store(&t_.db);
    Exercise e = full("Plank");
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    Exercise edited = e;
    edited.tips = "New tip";
    edited.name = "Plank (hard)";
    edited.colour = "";
    ExerciseChange change;
    QVERIFY(store.describeChange(edited, &change) == StoreStatus::Ok);
    QCOMPARE(change.fields.size(), 3);
    QVERIFY(change.fields.contains(ExerciseField::Name));
    QVERIFY(change.fields.contains(ExerciseField::Tips));
    QVERIFY(change.fields.contains(ExerciseField::Colour));
    QVERIFY(!change.fields.contains(ExerciseField::Muscles));
}

void TstExerciseStore::describeChangeListsWorkouts()
{
    ExerciseStore store(&t_.db);
    WorkoutStore workouts(&t_.db);
    const QString used = addExercise(t_.db, "Used");
    const QString unused = addExercise(t_.db, "Unused");
    const QStringList workoutNames = QStringList() << "Zeta" << "alpha" << "Mid";
    for (const QString &name : workoutNames) {
        Workout w = workout(QList<Block>() << block(QList<Step>() << work(used, 10)));
        w.name = name;
        QVERIFY(workouts.create(w) == StoreStatus::Ok);
    }

    Exercise e;
    QVERIFY(store.get(used, &e));
    e.name = "Used, edited";
    ExerciseChange change;
    QVERIFY(store.describeChange(e, &change) == StoreStatus::Ok);
    QCOMPARE(change.workouts, QStringList() << "alpha" << "Mid" << "Zeta");
    QCOMPARE(change.fields.size(), 1);

    QVERIFY(store.get(unused, &e));
    e.name = "Unused, edited";
    QVERIFY(store.describeChange(e, &change) == StoreStatus::Ok);
    QVERIFY(change.workouts.isEmpty());
    QCOMPARE(change.fields.size(), 1);
}

void TstExerciseStore::describeChangeUnknownKey()
{
    ExerciseStore store(&t_.db);
    Exercise e = makeExercise("Ghost");
    e.key = "u-missing";
    ExerciseChange change;
    QVERIFY(store.describeChange(e, &change) == StoreStatus::NotFound);
}

void TstExerciseStore::describeChangeWritesNothing()
{
    ExerciseStore store(&t_.db);
    Exercise e = full("Plank");
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    Exercise edited = e;
    edited.name = "Edited";
    edited.icon = "edited";
    ExerciseChange change;
    QVERIFY(store.describeChange(edited, &change) == StoreStatus::Ok);
    Exercise read;
    QVERIFY(store.get(e.key, &read));
    QCOMPARE(read.name, QStringLiteral("Plank"));
    QCOMPARE(read.icon, e.icon);
    QCOMPARE(countRows(t_.db, "exercise"), 1);
    QVERIFY(integrityOk(t_.db));
}

void TstExerciseStore::starterWithExistingKeyIsRecordedAndUnchanged()
{
    ExerciseStore store(&t_.db);
    // An entry with the key exists (for instance imported earlier), not as a starter entry.
    Exercise imported = makeExercise("Imported plank");
    imported.key = QStringLiteral("starter-plank");
    QVERIFY(store.createKeyed(imported) == StoreStatus::Ok);

    bool inserted = true;
    QVERIFY(store.offerStarter(starter("starter-plank", "Plank"), &inserted) == StoreStatus::Ok);
    QVERIFY(!inserted);
    Exercise read;
    QVERIFY(store.get("starter-plank", &read));
    QCOMPARE(read.name, QStringLiteral("Imported plank"));
    QVERIFY(!read.starter);
    QCOMPARE(countRows(t_.db, "exercise"), 1);
    QCOMPARE(countRows(t_.db, "starter_offered"), 1);

    // The key counts as offered: after a deletion it is not restored.
    QVERIFY(store.remove("starter-plank") == StoreStatus::Ok);
    QVERIFY(store.offerStarter(starter("starter-plank", "Plank"), &inserted) == StoreStatus::Ok);
    QVERIFY(!inserted);
    QCOMPARE(countRows(t_.db, "exercise"), 0);
}

int TstExerciseStore::metronome(const QString &name)
{
    return addMetronome(t_.db, name, Tempo::timePerRep(1500, 4));
}

void TstExerciseStore::newExerciseHasNoMetronome()
{
    ExerciseStore store(&t_.db);
    Exercise e = makeExercise("Plain");
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    Exercise read;
    QVERIFY(store.get(e.key, &read));
    QCOMPARE(read.metronomeId, 0);
    QVERIFY(store.tempoMap().isEmpty());
}

void TstExerciseStore::setAndClearTheMetronome()
{
    ExerciseStore store(&t_.db);
    const QString key = addExercise(t_.db, "Squat");
    const int m1 = metronome("One");
    const int m2 = metronome("Two");
    Exercise read;
    QVERIFY(store.setMetronome(key, m1) == StoreStatus::Ok);
    QVERIFY(store.get(key, &read));
    QCOMPARE(read.metronomeId, m1);
    // Linking another one replaces the link; the exercise stores a link, never a copy.
    QVERIFY(store.setMetronome(key, m2) == StoreStatus::Ok);
    QVERIFY(store.get(key, &read));
    QCOMPARE(read.metronomeId, m2);
    QVERIFY(store.clearMetronome(key) == StoreStatus::Ok);
    QVERIFY(store.get(key, &read));
    QCOMPARE(read.metronomeId, 0);
    // Clearing an exercise without a link is not an error, and the metronomes stay.
    QVERIFY(store.clearMetronome(key) == StoreStatus::Ok);
    QCOMPARE(countRows(t_.db, "metronome"), 2);
    QVERIFY(integrityOk(t_.db));
}

void TstExerciseStore::settingTheMetronomeChangesNothingElse()
{
    ExerciseStore store(&t_.db);
    Exercise e = makeExercise("Full", muscleBit(MuscleGroup::Glutes));
    e.icon = "squat";
    e.colour = "blue";
    e.language = "pt-br";
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    QVERIFY(store.setMetronome(e.key, metronome("M")) == StoreStatus::Ok);
    Exercise read;
    QVERIFY(store.get(e.key, &read));
    QCOMPARE(read.name, e.name);
    QCOMPARE(read.muscles, e.muscles);
    QCOMPARE(read.instructions, e.instructions);
    QCOMPARE(read.tips, e.tips);
    QCOMPARE(read.icon, e.icon);
    QCOMPARE(read.colour, e.colour);
    QCOMPARE(read.language, e.language);
    QCOMPARE(read.starter, e.starter);
}

void TstExerciseStore::setMetronomeRefusesAMissingMetronome()
{
    ExerciseStore store(&t_.db);
    const QString key = addExercise(t_.db, "Squat");
    const int m = metronome("M");
    QVERIFY(store.setMetronome(key, m) == StoreStatus::Ok);
    QVERIFY(store.setMetronome(key, 4242) == StoreStatus::Invalid);
    QVERIFY(store.setMetronome(key, 0) == StoreStatus::Invalid);
    Exercise read;
    QVERIFY(store.get(key, &read));
    QCOMPARE(read.metronomeId, m);
}

void TstExerciseStore::unknownExerciseIsNotFoundForTheLink()
{
    ExerciseStore store(&t_.db);
    QVERIFY(store.setMetronome("nobody", metronome("M")) == StoreStatus::NotFound);
    QVERIFY(store.clearMetronome("nobody") == StoreStatus::NotFound);
}

void TstExerciseStore::updateWritesTheLink()
{
    ExerciseStore store(&t_.db);
    const QString key = addExercise(t_.db, "Squat");
    const int m = metronome("M");
    Exercise e;
    QVERIFY(store.get(key, &e));
    e.metronomeId = m;
    e.name = "Squat 2";
    QVERIFY(store.update(e) == StoreStatus::Ok);
    Exercise read;
    QVERIFY(store.get(key, &read));
    QCOMPARE(read.metronomeId, m);
    QCOMPARE(read.name, QStringLiteral("Squat 2"));
    // An id that does not exist is stored as no link, without an error.
    e.metronomeId = 4242;
    QVERIFY(store.update(e) == StoreStatus::Ok);
    QVERIFY(store.get(key, &read));
    QCOMPARE(read.metronomeId, 0);
    QVERIFY(integrityOk(t_.db));
}

void TstExerciseStore::updateWithoutTheLinkClearsIt()
{
    // The link is part of the exercise as the screens edit it: detaching is an ordinary save.
    ExerciseStore store(&t_.db);
    const QString key = addExercise(t_.db, "Squat");
    QVERIFY(store.setMetronome(key, metronome("M")) == StoreStatus::Ok);
    Exercise e;
    QVERIFY(store.get(key, &e));
    e.metronomeId = 0;
    e.tips = "New tips";
    QVERIFY(store.update(e) == StoreStatus::Ok);
    Exercise read;
    QVERIFY(store.get(key, &read));
    QCOMPARE(read.metronomeId, 0);
    QCOMPARE(read.tips, QStringLiteral("New tips"));
    QCOMPARE(read.instructions, e.instructions);
}

void TstExerciseStore::createWithALinkStoresIt()
{
    ExerciseStore store(&t_.db);
    const int m = metronome("M");
    Exercise e = makeExercise("Linked");
    e.metronomeId = m;
    QVERIFY(store.create(&e) == StoreStatus::Ok);
    QCOMPARE(e.metronomeId, m);
    Exercise read;
    QVERIFY(store.get(e.key, &read));
    QCOMPARE(read.metronomeId, m);
    Exercise ghost = makeExercise("Ghost link");
    ghost.metronomeId = 4242;
    QVERIFY(store.create(&ghost) == StoreStatus::Ok);
    QCOMPARE(ghost.metronomeId, 0);
    QVERIFY(store.get(ghost.key, &read));
    QCOMPARE(read.metronomeId, 0);
}

void TstExerciseStore::aDamagedLinkReadsAsNoLinkAndIsClearedOnTheNextWrite()
{
    ExerciseStore store(&t_.db);
    const QString key = addExercise(t_.db, "Squat");
    // Damaged data: the column holds an id that no metronome has.
    QVERIFY(t_.db.run("PRAGMA foreign_keys = OFF"));
    QVERIFY(t_.db.run("UPDATE exercise SET metronome_id = 777 WHERE key = ?", QVariantList() << key));
    QVERIFY(t_.db.run("PRAGMA foreign_keys = ON"));

    Exercise read;
    QVERIFY(store.get(key, &read));
    QCOMPARE(read.metronomeId, 0);
    QCOMPARE(store.list().first().metronomeId, 0);
    QVERIFY(store.tempoMap().isEmpty());
    ExerciseChange change;
    QVERIFY(store.describeChange(read, &change) == StoreStatus::Ok);
    QVERIFY(change.fields.isEmpty());
    // The next write clears it.
    QVERIFY(store.update(read) == StoreStatus::Ok);
    QVERIFY(t_.db.run("PRAGMA foreign_keys = OFF"));
    QSqlQuery q;
    QVERIFY(t_.db.run("SELECT metronome_id FROM exercise WHERE key = ?", QVariantList() << key, &q) && q.next());
    QVERIFY(q.value(0).isNull());

    // The same through the link calls: clearing, and a valid link, replace the damaged value.
    QVERIFY(t_.db.run("UPDATE exercise SET metronome_id = 777 WHERE key = ?", QVariantList() << key));
    QVERIFY(store.clearMetronome(key) == StoreStatus::Ok);
    QVERIFY(t_.db.run("SELECT metronome_id FROM exercise WHERE key = ?", QVariantList() << key, &q) && q.next());
    QVERIFY(q.value(0).isNull());
    QVERIFY(t_.db.run("UPDATE exercise SET metronome_id = 777 WHERE key = ?", QVariantList() << key));
    const int m = metronome("M");
    QVERIFY(store.setMetronome(key, m) == StoreStatus::Ok);
    QVERIFY(store.get(key, &read));
    QCOMPARE(read.metronomeId, m);
    QVERIFY(t_.db.run("PRAGMA foreign_keys = ON"));
}

void TstExerciseStore::cloneKeepsTheLink()
{
    ExerciseStore store(&t_.db);
    const QString key = addExercise(t_.db, "Squat");
    const int m = metronome("M");
    QVERIFY(store.setMetronome(key, m) == StoreStatus::Ok);
    QString cloneKey;
    QVERIFY(store.clone(key, &cloneKey) == StoreStatus::Ok);
    Exercise copy;
    QVERIFY(store.get(cloneKey, &copy));
    QCOMPARE(copy.metronomeId, m);
    QCOMPARE(copy.name, QStringLiteral("Squat (copy)"));
    // Two exercises use the metronome now.
    QCOMPARE(MetronomeStore(&t_.db).exercisesUsing(m).size(), 2);
    // A clone of an exercise without a link has none.
    const QString plain = addExercise(t_.db, "Plain");
    QVERIFY(store.clone(plain, &cloneKey) == StoreStatus::Ok);
    QVERIFY(store.get(cloneKey, &copy));
    QCOMPARE(copy.metronomeId, 0);
}

void TstExerciseStore::cloneIsIndependentOfTheLink()
{
    ExerciseStore store(&t_.db);
    const QString key = addExercise(t_.db, "Squat");
    const int m = metronome("M");
    QVERIFY(store.setMetronome(key, m) == StoreStatus::Ok);
    QString cloneKey;
    QVERIFY(store.clone(key, &cloneKey) == StoreStatus::Ok);
    QVERIFY(store.clearMetronome(cloneKey) == StoreStatus::Ok);
    Exercise original, copy;
    QVERIFY(store.get(key, &original));
    QVERIFY(store.get(cloneKey, &copy));
    QCOMPARE(original.metronomeId, m);
    QCOMPARE(copy.metronomeId, 0);
}

void TstExerciseStore::describeChangeListsTheMetronome()
{
    ExerciseStore store(&t_.db);
    const QString key = addExercise(t_.db, "Squat");
    Exercise e;
    QVERIFY(store.get(key, &e));
    e.metronomeId = metronome("M");
    ExerciseChange change;
    QVERIFY(store.describeChange(e, &change) == StoreStatus::Ok);
    QVERIFY(change.fields == (QList<ExerciseField>() << ExerciseField::Metronome));
}

void TstExerciseStore::tempoMapHoldsTheExercisesWithAValidMetronome()
{
    ExerciseStore store(&t_.db);
    const QString a = addExercise(t_.db, "A");
    const QString b = addExercise(t_.db, "B");
    const QString c = addExercise(t_.db, "C");
    addExercise(t_.db, "D");
    const int mp = addMetronome(t_.db, "Phases", phasesOf({ 2, 3, 1 }, 6));
    const int mb = metronome("Bad");
    QVERIFY(store.setMetronome(a, mp) == StoreStatus::Ok);
    QVERIFY(store.setMetronome(b, mp) == StoreStatus::Ok);
    QVERIFY(store.setMetronome(c, mb) == StoreStatus::Ok);
    QVERIFY(t_.db.run("UPDATE metronome SET phases = 'garbage' WHERE id = ?", QVariantList() << mb));
    const ExerciseTempos map = store.tempoMap();
    QCOMPARE(map.size(), 2);
    QVERIFY(map.value(a) == phasesOf({ 2, 3, 1 }, 1));
    QVERIFY(map.value(b) == phasesOf({ 2, 3, 1 }, 1));
    QVERIFY(!map.contains(c));
}

void TstExerciseStore::deletingTheExerciseKeepsTheMetronome()
{
    ExerciseStore store(&t_.db);
    const QString key = addExercise(t_.db, "Squat");
    const int m = metronome("M");
    QVERIFY(store.setMetronome(key, m) == StoreStatus::Ok);
    QVERIFY(store.remove(key) == StoreStatus::Ok);
    SavedMetronome stored;
    QVERIFY(MetronomeStore(&t_.db).get(m, &stored));
    QVERIFY(integrityOk(t_.db));
}

QTEST_APPLESS_MAIN(TstExerciseStore)

#include "tst_exercisestore.moc"

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "exerciseform.h"
#include "musclegroups.h"
#include "testdb.h"

namespace {

Exercise sample(const QString &name, MuscleSet muscles)
{
    Exercise e;
    e.name = name;
    e.muscles = muscles;
    e.instructions = "Stand tall.";
    e.tips = "Keep the back straight.";
    e.colour = "blue";
    return e;
}

}

class TstExerciseForm : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void mapRoundTripsEveryField();
    void aMissingKeyReadsAsItsDefault();
    void unknownGroupKeysAreDropped();
    void retiredGroupKeysAreConverted();
    void nameIsTrimmed();
    void problemsHaveCodes();
    void codesAreListedOnceInOrder();
    void searchIgnoresCaseAndBlankSpace();
    void groupFilterNeedsTheGroup();
    void searchAndGroupCombine();
    void unknownGroupMatchesNothing();
    void editingTheTextsKeepsTheTempo();
    void detachingKeepsTheRest();
    void editedMapOfADeletedMetronomeStoresNoLink();

private:
    TestDb t_;
};

void TstExerciseForm::init()
{
    QVERIFY(t_.dir.isValid());
    t_.db.close();
    QDir(t_.dir.path()).removeRecursively();
    QVERIFY2(t_.open(), qPrintable(t_.error));
}

void TstExerciseForm::mapRoundTripsEveryField()
{
    Exercise e = sample("Squat", muscleBit(MuscleGroup::Quadriceps) | muscleBit(MuscleGroup::Glutes));
    e.key = "k1";
    e.icon = "squat";
    e.language = "de";
    e.starter = true;
    e.metronomeId = 7;
    const Exercise back = ExerciseForm::fromMap(ExerciseForm::toMap(e));
    QCOMPARE(back.key, e.key);
    QCOMPARE(back.name, e.name);
    QCOMPARE(back.muscles, e.muscles);
    QCOMPARE(back.instructions, e.instructions);
    QCOMPARE(back.tips, e.tips);
    QCOMPARE(back.icon, e.icon);
    QCOMPARE(back.colour, e.colour);
    QCOMPARE(back.language, e.language);
    QCOMPARE(back.starter, e.starter);
    QCOMPARE(back.metronomeId, 7);
}

void TstExerciseForm::aMissingKeyReadsAsItsDefault()
{
    const Exercise e = ExerciseForm::fromMap(QVariantMap());
    QVERIFY(e.name.isEmpty());
    QCOMPARE(e.muscles, MuscleSet(0));
    QCOMPARE(e.language, QStringLiteral("en"));
    QCOMPARE(e.metronomeId, 0);
    QVERIFY(!e.starter);
}

void TstExerciseForm::unknownGroupKeysAreDropped()
{
    QVariantMap m;
    m.insert("muscles", QStringList() << "biceps" << "wings");
    QCOMPARE(ExerciseForm::fromMap(m).muscles, muscleBit(MuscleGroup::Biceps));
}

void TstExerciseForm::retiredGroupKeysAreConverted()
{
    QVariantMap m;
    m.insert("muscles", QStringList() << "arms");
    const MuscleSet want = muscleBit(MuscleGroup::Biceps) | muscleBit(MuscleGroup::Triceps)
                           | muscleBit(MuscleGroup::Forearms);
    QCOMPARE(ExerciseForm::fromMap(m).muscles, want);
}

void TstExerciseForm::nameIsTrimmed()
{
    QVariantMap m;
    m.insert("name", "  Plank  ");
    QCOMPARE(ExerciseForm::fromMap(m).name, QStringLiteral("Plank"));
}

void TstExerciseForm::problemsHaveCodes()
{
    QCOMPARE(ExerciseForm::problemCode(ExerciseProblem::NameEmpty), QStringLiteral("name-empty"));
    QCOMPARE(ExerciseForm::problemCode(ExerciseProblem::NameTooLong), QStringLiteral("name-too-long"));
    QCOMPARE(ExerciseForm::problemCode(ExerciseProblem::NoMuscles), QStringLiteral("muscles-none"));
    QCOMPARE(ExerciseForm::problemCode(ExerciseProblem::InstructionsTooLong), QStringLiteral("instructions-too-long"));
    QCOMPARE(ExerciseForm::problemCode(ExerciseProblem::TipsTooLong), QStringLiteral("tips-too-long"));
    QCOMPARE(ExerciseForm::problemCode(ExerciseProblem::ColourUnknown), QStringLiteral("colour"));
    // What the store refuses is what the form reports.
    const QStringList codes = ExerciseForm::problemCodes(ExerciseStore::validate(ExerciseForm::fromMap(QVariantMap())));
    QVERIFY(codes.contains("name-empty"));
    QVERIFY(codes.contains("muscles-none"));
}

void TstExerciseForm::codesAreListedOnceInOrder()
{
    const QStringList codes = ExerciseForm::problemCodes(
        QList<ExerciseProblem>() << ExerciseProblem::NoMuscles << ExerciseProblem::NameEmpty << ExerciseProblem::NoMuscles);
    QCOMPARE(codes, QStringList() << "muscles-none" << "name-empty");
}

void TstExerciseForm::searchIgnoresCaseAndBlankSpace()
{
    const Exercise e = sample("Push-up", muscleBit(MuscleGroup::Triceps));
    QVERIFY(ExerciseForm::matches(e, "PUSH", QString()));
    QVERIFY(ExerciseForm::matches(e, "  up ", QString()));
    QVERIFY(ExerciseForm::matches(e, QString(), QString()));
    QVERIFY(ExerciseForm::matches(e, "   ", QString()));
    QVERIFY(!ExerciseForm::matches(e, "pull", QString()));
}

void TstExerciseForm::groupFilterNeedsTheGroup()
{
    const Exercise e = sample("Push-up", muscleBit(MuscleGroup::Triceps) | muscleBit(MuscleGroup::UpperChest));
    QVERIFY(ExerciseForm::matches(e, QString(), "triceps"));
    QVERIFY(ExerciseForm::matches(e, QString(), "upper-chest"));
    QVERIFY(!ExerciseForm::matches(e, QString(), "calves"));
}

void TstExerciseForm::searchAndGroupCombine()
{
    QList<Exercise> all;
    all << sample("Squat", muscleBit(MuscleGroup::Quadriceps))
        << sample("Squat jump", muscleBit(MuscleGroup::Calves))
        << sample("Lunge", muscleBit(MuscleGroup::Quadriceps));
    QCOMPARE(ExerciseForm::filter(all, "squat", QString()).size(), 2);
    QCOMPARE(ExerciseForm::filter(all, QString(), "quadriceps").size(), 2);
    const QList<Exercise> both = ExerciseForm::filter(all, "squat", "quadriceps");
    QCOMPARE(both.size(), 1);
    QCOMPARE(both.first().name, QStringLiteral("Squat"));
    QCOMPARE(ExerciseForm::filter(all, QString(), QString()).size(), 3);
}

void TstExerciseForm::unknownGroupMatchesNothing()
{
    QVERIFY(!ExerciseForm::matches(sample("A", muscleBit(MuscleGroup::Calves)), QString(), "wings"));
}

void TstExerciseForm::editingTheTextsKeepsTheTempo()
{
    ExerciseStore store(&t_.db);
    const int metronome = addMetronome(t_.db, "Slow", Tempo::timePerRep(3000, 5));
    Exercise e = sample("Squat", muscleBit(MuscleGroup::Quadriceps));
    e.metronomeId = metronome;
    QVERIFY(store.create(&e) == StoreStatus::Ok);

    // The screen loads the exercise as a map, changes the texts and saves what it shows.
    Exercise loaded;
    QVERIFY(store.get(e.key, &loaded));
    QVariantMap shown = ExerciseForm::toMap(loaded);
    shown.insert("instructions", "Sit back and down.");
    shown.insert("tips", "Knees over the toes.");
    shown.insert("name", "Back squat");
    QVERIFY(store.update(ExerciseForm::fromMap(shown)) == StoreStatus::Ok);

    Exercise read;
    QVERIFY(store.get(e.key, &read));
    QCOMPARE(read.metronomeId, metronome);
    QCOMPARE(read.instructions, QStringLiteral("Sit back and down."));
    QCOMPARE(read.name, QStringLiteral("Back squat"));
    QVERIFY(store.tempoMap().contains(e.key));
}

void TstExerciseForm::detachingKeepsTheRest()
{
    ExerciseStore store(&t_.db);
    MetronomeStore metronomes(&t_.db);
    const int metronome = addMetronome(t_.db, "Slow", Tempo::timePerRep(3000, 5));
    Exercise e = sample("Squat", muscleBit(MuscleGroup::Quadriceps));
    e.metronomeId = metronome;
    QVERIFY(store.create(&e) == StoreStatus::Ok);

    QVariantMap shown = ExerciseForm::toMap(e);
    shown.insert("metronomeId", 0);
    QVERIFY(store.update(ExerciseForm::fromMap(shown)) == StoreStatus::Ok);

    Exercise read;
    QVERIFY(store.get(e.key, &read));
    QCOMPARE(read.metronomeId, 0);
    QCOMPARE(read.name, QStringLiteral("Squat"));
    QCOMPARE(read.muscles, e.muscles);
    QCOMPARE(read.instructions, e.instructions);
    QCOMPARE(read.colour, QStringLiteral("blue"));
    // The metronome itself is untouched.
    SavedMetronome m;
    QVERIFY(metronomes.get(metronome, &m));
}

void TstExerciseForm::editedMapOfADeletedMetronomeStoresNoLink()
{
    ExerciseStore store(&t_.db);
    MetronomeStore metronomes(&t_.db);
    const int metronome = addMetronome(t_.db, "Slow", Tempo::timePerRep(3000, 5));
    Exercise e = sample("Squat", muscleBit(MuscleGroup::Quadriceps));
    e.metronomeId = metronome;
    QVERIFY(store.create(&e) == StoreStatus::Ok);

    // The editor was opened before the metronome was deleted elsewhere.
    const QVariantMap shown = ExerciseForm::toMap(e);
    QVERIFY(metronomes.remove(metronome) == StoreStatus::Ok);
    QVERIFY(store.update(ExerciseForm::fromMap(shown)) == StoreStatus::Ok);

    Exercise read;
    QVERIFY(store.get(e.key, &read));
    QCOMPARE(read.metronomeId, 0);
}

QTEST_APPLESS_MAIN(TstExerciseForm)
#include "tst_exerciseform.moc"

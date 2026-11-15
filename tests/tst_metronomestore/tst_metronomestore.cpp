// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "metronomestore.h"
#include "testdb.h"
#include "workoutlimits.h"

namespace {

// A rhythm: a valid pace of any form is turned into its rhythm (rhythmOf()); an invalid tempo is
// kept as it is, to test the refusals.
SavedMetronome make(const QString &name, const Tempo &tempo = Tempo::timePerRep(1500, 4))
{
    SavedMetronome m;
    m.name = name;
    m.tempo = tempo.isValid() ? rhythmOf(tempo) : tempo;
    return m;
}

bool has(const QList<MetronomeProblem> &list, MetronomeProblem::Code code)
{
    for (const MetronomeProblem &p : list) {
        if (p.code == code)
            return true;
    }
    return false;
}

}

class TstMetronomeStore : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void createReadsBackEveryRhythm();
    void keyIsGeneratedAndStable();
    void phaseNamesAreStoredAndLongNamesRefused();
    void givenKeyIsUsedOnce();
    void listIsSortedWithoutRegardToCase();
    void summaryCarriesTheFigures();
    void validationOfName();
    void validationOfTheRhythm();
    void severalProblemsAreReportedTogether();
    void invalidSaveWritesNothing();
    void updateReplacesTheValues();
    void updateOfAMissingMetronomeIsNotFound();

    void equalIdentityIsRefused();
    void sameNameIsRefusedWhateverTheRest();
    void caseAndBlankSpaceVariantsAreRefused();
    void emptyKeysEqualEachOther();
    void editingWithoutChangingTheIdentityIsAccepted();
    void changingIntoAClashIsRefused();
    void anEditKeepsWorkingWhenOldDataAlreadyClashes();

    void duplicateTakesTheFirstFreeSuffix();
    void duplicateGetsANewKey();
    void duplicateSkipsSuffixesThatClashOnTheName();

    void exercisesUsingAreSortedAndCounted();
    void updateTellsWhichExercisesAreAffected();
    void impactOfEditIsTheSameList();
    void saveAsCopyLeavesTheOriginalAndItsExercises();
    void removingAUsedMetronomeClearsTheLinksAndKeepsTheExercises();
    void removingAnUnusedMetronomeReportsNothing();
    void removingAMissingMetronomeIsNotFound();
    void aFailingRemoveChangesNothing();
    void theDatabaseClearsTheLinkEvenOnARawDelete();
    void anUnreadableRhythmIsListedWithoutFigures();

    void problemsOfReportsWithoutWriting();
    void problemsOfSkipsTheCompareForAnUnchangedIdentity();

private:
    // A new exercise linked to the metronome; returns its key.
    QString link(const QString &exerciseName, int metronomeId);
    TestDb t_;
    QString ex_;
};

void TstMetronomeStore::init()
{
    t_.db.close();
    QDir(t_.dir.path()).removeRecursively();
    QVERIFY(t_.open());
    ex_ = addExercise(t_.db, "Squat");
}

QString TstMetronomeStore::link(const QString &exerciseName, int metronomeId)
{
    const QString key = addExercise(t_.db, exerciseName);
    return ExerciseStore(&t_.db).setMetronome(key, metronomeId) == StoreStatus::Ok ? key : QString();
}

void TstMetronomeStore::createReadsBackEveryRhythm()
{
    MetronomeStore store(&t_.db);
    const Tempo rhythms[] = { phasesOf({ 1 }, 1), phasesOf({ 2, 3, 1 }, 1), phasesOf({ 20 }, 1),
                              phasesOf({ 1, 2, 3, 4, 5, 6, 7, 8 }, 1), phasesOf({ 20, 20, 20, 20, 20, 20 }, 1) };
    int n = 0;
    for (const Tempo &tempo : rhythms) {
        SavedMetronome m = make(QStringLiteral("M%1").arg(++n), tempo);
        m.createdAt = 1700000000 + n;
        int id = 0;
        QList<MetronomeProblem> problems;
        QVERIFY2(store.create(m, &id, &problems) == StoreStatus::Ok, qPrintable(tempo.toText()));
        QVERIFY(problems.isEmpty());
        SavedMetronome read;
        QVERIFY(store.get(id, &read));
        QCOMPARE(read.id, id);
        QCOMPARE(read.name, m.name);
        QVERIFY(read.tempo == tempo);
        QCOMPARE(read.createdAt, m.createdAt);
    }
    // Defaults: empty keys, the creation time is set when none is given.
    int id = 0;
    QVERIFY(store.create(make("Plain"), &id) == StoreStatus::Ok);
    SavedMetronome plain;
    QVERIFY(store.get(id, &plain));
    QVERIFY(plain.createdAt > 1600000000);
    QVERIFY(integrityOk(t_.db));
}

void TstMetronomeStore::phaseNamesAreStoredAndLongNamesRefused()
{
    MetronomeStore store(&t_.db);
    const QString emoji = QString::fromUtf8("\xf0\x9f\x98\x80");
    const Tempo named = Tempo::phases({ 2, 3, 1 }, 1, QStringList() << "Down;1" << "" << emoji);
    SavedMetronome m = make(QStringLiteral("Names"), named);
    int id = 0;
    QList<MetronomeProblem> problems;
    QVERIFY(store.create(m, &id, &problems) == StoreStatus::Ok);
    SavedMetronome read;
    QVERIFY(store.get(id, &read));
    QVERIFY(read.tempo == named);
    QCOMPARE(read.tempo.phaseName(1), QStringLiteral("Down;1"));
    QCOMPARE(read.tempo.phaseName(3), emoji);

    // A rhythm without names reads back with empty names.
    SavedMetronome old = make(QStringLiteral("Old"), phasesOf({ 2, 3, 1 }, 1));
    int oldId = 0;
    QVERIFY(store.create(old, &oldId, &problems) == StoreStatus::Ok);
    QVERIFY(store.get(oldId, &read));
    QCOMPARE(read.tempo.phaseNameList(), QStringList() << "" << "" << "");

    // A name of 13 characters is a problem of the pace and nothing is written.
    const int before = store.list().size();
    SavedMetronome bad = make(QStringLiteral("Long"), Tempo::phases({ 2 }, 5, QStringList() << QString(13, QLatin1Char('x'))));
    int badId = 0;
    problems.clear();
    QVERIFY(store.create(bad, &badId, &problems) == StoreStatus::Invalid);
    QCOMPARE(problems.size(), 1);
    QVERIFY(problems.first().code == MetronomeProblem::TempoInvalid);
    QVERIFY(problems.first().tempoProblem == TempoProblem::PhaseNameTooLong);
    QCOMPARE(store.list().size(), before);
}

void TstMetronomeStore::keyIsGeneratedAndStable()
{
    MetronomeStore store(&t_.db);
    int a = 0, b = 0;
    QVERIFY(store.create(make("A"), &a) == StoreStatus::Ok);
    QVERIFY(store.create(make("B"), &b) == StoreStatus::Ok);
    SavedMetronome ma, mb;
    QVERIFY(store.get(a, &ma));
    QVERIFY(store.get(b, &mb));
    QVERIFY(ma.key.startsWith(QLatin1String("m-")));
    QVERIFY(ma.key != mb.key);
    QVERIFY(ma.key.length() <= 64);

    // An edit keeps the key and the creation time.
    ma.name = QStringLiteral("A renamed");
    ma.tempo = rhythmOf(Tempo::timePerRep(2000, 3));
    ma.key = QStringLiteral("ignored");
    ma.createdAt = 5;
    QVERIFY(store.update(a, ma) == StoreStatus::Ok);
    SavedMetronome again;
    QVERIFY(store.get(a, &again));
    QVERIFY(again.key.startsWith(QLatin1String("m-")));
    QVERIFY(again.key != QLatin1String("ignored"));
    QVERIFY(again.createdAt > 5);
    QVERIFY(store.getByKey(again.key, &mb));
    QCOMPARE(mb.id, a);
    QVERIFY(!store.getByKey(QStringLiteral("nothing"), &mb));
}

void TstMetronomeStore::givenKeyIsUsedOnce()
{
    MetronomeStore store(&t_.db);
    SavedMetronome m = make("Imported");
    m.key = QStringLiteral("from-a-file");
    int id = 0;
    QVERIFY(store.create(m, &id) == StoreStatus::Ok);
    SavedMetronome read;
    QVERIFY(store.get(id, &read));
    QCOMPARE(read.key, QStringLiteral("from-a-file"));
    SavedMetronome other = make("Other");
    other.key = QStringLiteral("from-a-file");
    QVERIFY(store.create(other) == StoreStatus::Invalid);
    QCOMPARE(countRows(t_.db, "metronome"), 1);
}

void TstMetronomeStore::listIsSortedWithoutRegardToCase()
{
    MetronomeStore store(&t_.db);
    QVERIFY(store.create(make("banana")) == StoreStatus::Ok);
    QVERIFY(store.create(make("Apple")) == StoreStatus::Ok);
    QVERIFY(store.create(make("cherry")) == StoreStatus::Ok);
    QStringList names;
    for (const MetronomeSummary &s : store.list())
        names << s.name;
    QCOMPARE(names, QStringList() << "Apple" << "banana" << "cherry");
}

void TstMetronomeStore::summaryCarriesTheFigures()
{
    MetronomeStore store(&t_.db);
    int ida = 0, idb = 0, idc = 0;
    QVERIFY(store.create(make("Even", phasesOf({ 3, 3, 4 }, 1)), &ida) == StoreStatus::Ok);
    SavedMetronome named = make("Phases", phasesOf({ 2, 4 }, 1));
    named.tempo = Tempo::phases({ 2, 4 }, 1, QStringList() << "Down" << "Up");
    QVERIFY(store.create(named, &idb) == StoreStatus::Ok);
    QVERIFY(store.create(make("Slow", Tempo::timePerRep(2500, 3)), &idc) == StoreStatus::Ok);
    QVERIFY(!link("E1", ida).isEmpty());
    QVERIFY(!link("E2", ida).isEmpty());
    QVERIFY(!link("E3", idb).isEmpty());

    const QList<MetronomeSummary> list = store.list();
    QCOMPARE(list.size(), 3);
    QCOMPARE(list.at(0).name, QStringLiteral("Even"));
    QCOMPARE(list.at(0).phaseCount, 3);
    QCOMPARE(list.at(0).phaseUnits, QList<int>() << 3 << 3 << 4);
    QCOMPARE(list.at(0).repetitionMs, qint64(5000));
    QCOMPARE(list.at(0).exerciseCount, 2);
    QCOMPARE(list.at(1).name, QStringLiteral("Phases"));
    QCOMPARE(list.at(1).repetitionMs, qint64(3000));
    QCOMPARE(list.at(1).exerciseCount, 1);
    QCOMPARE(list.at(1).phaseCount, 2);
    QCOMPARE(list.at(1).phaseUnits, QList<int>() << 2 << 4);
    QCOMPARE(list.at(1).phaseNames, QStringList() << "Down" << "Up");
    QCOMPARE(list.at(2).name, QStringLiteral("Slow"));
    QCOMPARE(list.at(2).phaseCount, 1);
    QCOMPARE(list.at(2).repetitionMs, qint64(2500));
    QCOMPARE(list.at(2).exerciseCount, 0);
    QVERIFY(!list.at(0).key.isEmpty());
}

void TstMetronomeStore::validationOfName()
{
    QVERIFY(MetronomeStore::validate(make("Fine")).isEmpty());
    QVERIFY(has(MetronomeStore::validate(make("")), MetronomeProblem::NameEmpty));
    QVERIFY(has(MetronomeStore::validate(make("  \t ")), MetronomeProblem::NameEmpty));
    QVERIFY(MetronomeStore::validate(make(QString(Limits::maxNameChars, QLatin1Char('x')))).isEmpty());
    QVERIFY(has(MetronomeStore::validate(make(QString(Limits::maxNameChars + 1, QLatin1Char('x')))), MetronomeProblem::NameTooLong));
    QVERIFY(MetronomeStore::validate(make("A", Tempo::timePerRep(1000, 3))).isEmpty());
}

void TstMetronomeStore::validationOfTheRhythm()
{
    struct Case { Tempo tempo; TempoProblem reason; };
    const Case cases[] = {
        { Tempo(), TempoProblem::RepsOutOfRange },
        { phasesOf({}, 1), TempoProblem::PhaseCountOutOfRange },
        { phasesOf({ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 1), TempoProblem::PhaseCountOutOfRange },
        { phasesOf({ 2, 0, 3 }, 1), TempoProblem::PhaseOutOfRange },
        { phasesOf({ 21 }, 1), TempoProblem::PhaseOutOfRange },
        { phasesOf({ 20, 20, 20, 20, 20, 20, 20 }, 1), TempoProblem::RepTooLong },
    };
    for (const Case &c : cases) {
        const QList<MetronomeProblem> found = MetronomeStore::validate(make("A", c.tempo));
        QCOMPARE(found.size(), 1);
        QVERIFY(found.first().code == MetronomeProblem::TempoInvalid);
        QVERIFY(found.first().tempoProblem == c.reason);
    }
    // The limits themselves are accepted: one phase of 0.5 s, and 6 phases of 10 s (60 s).
    QVERIFY(MetronomeStore::validate(make("A", phasesOf({ 1 }, 1))).isEmpty());
    QVERIFY(MetronomeStore::validate(make("A", phasesOf({ 20, 20, 20, 20, 20, 20 }, 1))).isEmpty());
    // A rhythm is the phases of one repetition: other forms and other repetition counts are refused.
    SavedMetronome m = make("A");
    m.tempo = Tempo::timePerRep(1500, 4);
    QVERIFY(has(MetronomeStore::validate(m), MetronomeProblem::TempoInvalid));
    m.tempo = phasesOf({ 2, 2 }, 5);
    QVERIFY(has(MetronomeStore::validate(m), MetronomeProblem::TempoInvalid));
}

void TstMetronomeStore::severalProblemsAreReportedTogether()
{
    SavedMetronome m = make("", Tempo::timePerRep(100, 5));
    const QList<MetronomeProblem> found = MetronomeStore::validate(m);
    QVERIFY(has(found, MetronomeProblem::NameEmpty));
    QVERIFY(has(found, MetronomeProblem::TempoInvalid));
    QCOMPARE(found.size(), 2);
}

void TstMetronomeStore::invalidSaveWritesNothing()
{
    MetronomeStore store(&t_.db);
    QList<MetronomeProblem> problems;
    QVERIFY(store.create(make("A", Tempo::timePerRep(100, 5)), nullptr, &problems) == StoreStatus::Invalid);
    QVERIFY(has(problems, MetronomeProblem::TempoInvalid));
    QCOMPARE(countRows(t_.db, "metronome"), 0);
    int id = 0;
    QVERIFY(store.create(make("Kept"), &id) == StoreStatus::Ok);
    SavedMetronome bad = make("Kept", Tempo::timePerRep(100, 5));
    QVERIFY(store.update(id, bad, &problems) == StoreStatus::Invalid);
    SavedMetronome read;
    QVERIFY(store.get(id, &read));
    QVERIFY(read.tempo == rhythmOf(Tempo::timePerRep(1500, 4)));
}

void TstMetronomeStore::updateReplacesTheValues()
{
    MetronomeStore store(&t_.db);
    int id = 0;
    QVERIFY(store.create(make("Old", Tempo::timePerRep(1000, 3)), &id) == StoreStatus::Ok);
    SavedMetronome m = make("New", phasesOf({ 2, 2, 4 }, 1));
    QVERIFY(store.update(id, m) == StoreStatus::Ok);
    SavedMetronome read;
    QVERIFY(store.get(id, &read));
    QCOMPARE(read.name, QStringLiteral("New"));
    QVERIFY(read.tempo == phasesOf({ 2, 2, 4 }, 1));
    QCOMPARE(countRows(t_.db, "metronome"), 1);
}

void TstMetronomeStore::updateOfAMissingMetronomeIsNotFound()
{
    MetronomeStore store(&t_.db);
    QVERIFY(store.update(42, make("X")) == StoreStatus::NotFound);
    QVERIFY(store.remove(42) == StoreStatus::NotFound);
    QVERIFY(store.duplicate(42) == StoreStatus::NotFound);
    SavedMetronome m;
    QVERIFY(!store.get(42, &m));
}

void TstMetronomeStore::equalIdentityIsRefused()
{
    MetronomeStore store(&t_.db);
    QVERIFY(store.create(make("Pace", Tempo::timePerRep(1000, 3))) == StoreStatus::Ok);
    QList<MetronomeProblem> problems;
    // The pace does not matter for the identity.
    QVERIFY(store.create(make("Pace", Tempo::timePerRep(2000, 9)), nullptr, &problems) == StoreStatus::Invalid);
    QVERIFY(has(problems, MetronomeProblem::IdentityTaken));
    QCOMPARE(countRows(t_.db, "metronome"), 1);
}

void TstMetronomeStore::sameNameIsRefusedWhateverTheRest()
{
    MetronomeStore store(&t_.db);
    QVERIFY(store.create(make("Pace", Tempo::timePerRep(1000, 3))) == StoreStatus::Ok);
    QVERIFY(store.create(make("Pace", Tempo::timePerRep(2000, 9))) == StoreStatus::Invalid);
    QVERIFY(store.create(make("pace", Tempo::timePerRep(1000, 3))) == StoreStatus::Invalid);
    QCOMPARE(countRows(t_.db, "metronome"), 1);
}

void TstMetronomeStore::caseAndBlankSpaceVariantsAreRefused()
{
    MetronomeStore store(&t_.db);
    QVERIFY(store.create(make("Pace")) == StoreStatus::Ok);
    QVERIFY(store.create(make("PACE")) == StoreStatus::Invalid);
    QVERIFY(store.create(make("  pace\t")) == StoreStatus::Invalid);
    // Blank space inside counts.
    QVERIFY(store.create(make("Pa ce")) == StoreStatus::Ok);
    QCOMPARE(countRows(t_.db, "metronome"), 2);
}

void TstMetronomeStore::emptyKeysEqualEachOther()
{
    MetronomeStore store(&t_.db);
    QVERIFY(store.create(make("Pace", Tempo::timePerRep(1000, 3))) == StoreStatus::Ok);
    QVERIFY(store.create(make("Pace", Tempo::timePerRep(1000, 3))) == StoreStatus::Invalid);
}

void TstMetronomeStore::editingWithoutChangingTheIdentityIsAccepted()
{
    MetronomeStore store(&t_.db);
    int id = 0;
    QVERIFY(store.create(make("Pace", Tempo::timePerRep(1000, 3)), &id) == StoreStatus::Ok);
    SavedMetronome m = make("Pace", Tempo::timePerRep(3000, 8));
    QVERIFY(store.update(id, m) == StoreStatus::Ok);
    // A change of the letter case alone is a change of the stored text and is compared by identity,
    // which is the same: it is accepted.
    m.name = QStringLiteral("PACE");
    QVERIFY(store.update(id, m) == StoreStatus::Ok);
}

void TstMetronomeStore::changingIntoAClashIsRefused()
{
    MetronomeStore store(&t_.db);
    int a = 0, b = 0;
    QVERIFY(store.create(make("One", Tempo::timePerRep(1000, 3)), &a) == StoreStatus::Ok);
    QVERIFY(store.create(make("Two", Tempo::timePerRep(1000, 3)), &b) == StoreStatus::Ok);
    QList<MetronomeProblem> problems;
    QVERIFY(store.update(b, make("one", Tempo::timePerRep(1000, 3)), &problems) == StoreStatus::Invalid);
    QVERIFY(has(problems, MetronomeProblem::IdentityTaken));
    // Changing the name resolves it.
    QVERIFY(store.update(b, make("Three", Tempo::timePerRep(1000, 3))) == StoreStatus::Ok);
    SavedMetronome read;
    QVERIFY(store.get(a, &read));
    QCOMPARE(read.name, QStringLiteral("One"));
}

void TstMetronomeStore::anEditKeepsWorkingWhenOldDataAlreadyClashes()
{
    MetronomeStore store(&t_.db);
    int a = 0, b = 0;
    QVERIFY(store.create(make("One"), &a) == StoreStatus::Ok);
    QVERIFY(store.create(make("Two"), &b) == StoreStatus::Ok);
    // Data written before the rule, or by hand, with equal identities.
    QVERIFY(t_.db.run("UPDATE metronome SET name = 'One' WHERE id = ?", QVariantList() << b));
    SavedMetronome m;
    QVERIFY(store.get(b, &m));
    m.tempo = rhythmOf(Tempo::timePerRep(2000, 3));
    QVERIFY(store.update(b, m) == StoreStatus::Ok);
    // The clash itself is refused for any other record.
    QVERIFY(store.create(make("one")) == StoreStatus::Invalid);
}

void TstMetronomeStore::duplicateTakesTheFirstFreeSuffix()
{
    MetronomeStore store(&t_.db);
    int id = 0, copy1 = 0, copy2 = 0, copy3 = 0;
    QVERIFY(store.create(make("Pace", phasesOf({ 2, 3, 1 }, 1)), &id) == StoreStatus::Ok);
    QVERIFY(store.duplicate(id, &copy1) == StoreStatus::Ok);
    QVERIFY(store.duplicate(id, &copy2) == StoreStatus::Ok);
    QVERIFY(store.duplicate(copy1, &copy3) == StoreStatus::Ok);
    SavedMetronome m;
    QVERIFY(store.get(copy1, &m));
    QCOMPARE(m.name, QStringLiteral("Pace (copy)"));
    QVERIFY(m.tempo == phasesOf({ 2, 3, 1 }, 1));
    QVERIFY(store.get(copy2, &m));
    QCOMPARE(m.name, QStringLiteral("Pace (copy 2)"));
    QVERIFY(store.get(copy3, &m));
    QCOMPARE(m.name, QStringLiteral("Pace (copy) (copy)"));
}

void TstMetronomeStore::duplicateGetsANewKey()
{
    MetronomeStore store(&t_.db);
    SavedMetronome original = make("Pace", Tempo::repsAndTotal(7, 20000));
    int id = 0, copyId = 0;
    QVERIFY(store.create(original, &id) == StoreStatus::Ok);
    QVERIFY(store.duplicate(id, &copyId) == StoreStatus::Ok);
    SavedMetronome a, b;
    QVERIFY(store.get(id, &a));
    QVERIFY(store.get(copyId, &b));
    QVERIFY(a.key != b.key);
    QVERIFY(b.tempo == a.tempo);
    // The copy is independent: a change of one does not reach the other.
    b.tempo = rhythmOf(Tempo::timePerRep(1000, 2));
    QVERIFY(store.update(copyId, b) == StoreStatus::Ok);
    QVERIFY(store.get(id, &a));
    QVERIFY(a.tempo == rhythmOf(Tempo::repsAndTotal(7, 20000)));
}

void TstMetronomeStore::duplicateSkipsSuffixesThatClashOnTheName()
{
    MetronomeStore store(&t_.db);
    int id = 0;
    QVERIFY(store.create(make("Pace", Tempo::timePerRep(1000, 3)), &id) == StoreStatus::Ok);
    // "Pace (copy)" exists: the copy takes the next free suffix.
    QVERIFY(store.create(make("Pace (copy)", Tempo::timePerRep(1000, 3))) == StoreStatus::Ok);
    int copyId = 0;
    QVERIFY(store.duplicate(id, &copyId) == StoreStatus::Ok);
    SavedMetronome m;
    QVERIFY(store.get(copyId, &m));
    QCOMPARE(m.name, QStringLiteral("Pace (copy 2)"));
    // A long name is cut to make room for the suffix.
    const QString longName(Limits::maxNameChars, QLatin1Char('n'));
    int longId = 0;
    QVERIFY(store.create(make(longName), &longId) == StoreStatus::Ok);
    QVERIFY(store.duplicate(longId, &copyId) == StoreStatus::Ok);
    QVERIFY(store.get(copyId, &m));
    QCOMPARE(m.name.length(), Limits::maxNameChars);
    QVERIFY(m.name.endsWith(QLatin1String(" (copy)")));
}

void TstMetronomeStore::exercisesUsingAreSortedAndCounted()
{
    MetronomeStore store(&t_.db);
    int id = 0, other = 0;
    QVERIFY(store.create(make("Shared"), &id) == StoreStatus::Ok);
    QVERIFY(store.create(make("Alone"), &other) == StoreStatus::Ok);
    const QString zulu = link("zulu", id);
    const QString alpha = link("Alpha", id);
    link("Not using", other);
    const QList<ExerciseRef> refs = store.exercisesUsing(id);
    QCOMPARE(refs.size(), 2);
    QCOMPARE(refs.at(0).name, QStringLiteral("Alpha"));
    QCOMPARE(refs.at(0).key, alpha);
    QCOMPARE(refs.at(1).name, QStringLiteral("zulu"));
    QCOMPARE(refs.at(1).key, zulu);
    QCOMPARE(store.exercisesUsing(other).size(), 1);
    QVERIFY(store.exercisesUsing(9999).isEmpty());
    // An exercise without a metronome belongs to no list.
    QCOMPARE(store.exercisesUsing(0).size(), 0);
}

void TstMetronomeStore::updateTellsWhichExercisesAreAffected()
{
    MetronomeStore store(&t_.db);
    int id = 0;
    QVERIFY(store.create(make("Shared"), &id) == StoreStatus::Ok);
    link("B exercise", id);
    link("A exercise", id);
    SavedMetronome m;
    QVERIFY(store.get(id, &m));
    m.tempo = rhythmOf(Tempo::timePerRep(2000, 6));
    QList<ExerciseRef> affected;
    QVERIFY(store.update(id, m, nullptr, &affected) == StoreStatus::Ok);
    QStringList names;
    for (const ExerciseRef &r : affected)
        names << r.name;
    QCOMPARE(names, QStringList() << "A exercise" << "B exercise");
    // An invalid change reports nothing as affected and changes nothing.
    affected.clear();
    m.tempo = Tempo::timePerRep(5, 6);
    QVERIFY(store.update(id, m, nullptr, &affected) == StoreStatus::Invalid);
    QVERIFY(affected.isEmpty());
}

void TstMetronomeStore::impactOfEditIsTheSameList()
{
    MetronomeStore store(&t_.db);
    int id = 0, unused = 0;
    QVERIFY(store.create(make("Shared"), &id) == StoreStatus::Ok);
    QVERIFY(store.create(make("Unused"), &unused) == StoreStatus::Ok);
    link("One", id);
    link("Two", id);
    const QList<ExerciseRef> impact = store.impactOfEdit(id);
    QCOMPARE(impact.size(), 2);
    QCOMPARE(impact.at(0).key, store.exercisesUsing(id).at(0).key);
    QCOMPARE(impact.at(1).name, QStringLiteral("Two"));
    QVERIFY(store.impactOfEdit(unused).isEmpty());
}

void TstMetronomeStore::saveAsCopyLeavesTheOriginalAndItsExercises()
{
    MetronomeStore store(&t_.db);
    ExerciseStore exercises(&t_.db);
    int id = 0;
    QVERIFY(store.create(make("Shared", Tempo::timePerRep(1500, 4)), &id) == StoreStatus::Ok);
    const QString key = link("Uses it", id);
    SavedMetronome changed;
    QVERIFY(store.get(id, &changed));
    changed.tempo = phasesOf({ 4, 2 }, 1);
    int copyId = 0;
    QVERIFY(store.saveAsCopy(changed, &copyId) == StoreStatus::Ok);
    QVERIFY(copyId != id);

    SavedMetronome original, copy;
    QVERIFY(store.get(id, &original));
    QVERIFY(store.get(copyId, &copy));
    QVERIFY(original.tempo == rhythmOf(Tempo::timePerRep(1500, 4)));
    QVERIFY(copy.tempo == phasesOf({ 4, 2 }, 1));
    QCOMPARE(copy.name, QStringLiteral("Shared (copy)"));
    QVERIFY(copy.key != original.key);
    // Nothing is relinked: the exercise still uses the original, the copy is used by none.
    Exercise read;
    QVERIFY(exercises.get(key, &read));
    QCOMPARE(read.metronomeId, id);
    QCOMPARE(store.exercisesUsing(id).size(), 1);
    QVERIFY(store.exercisesUsing(copyId).isEmpty());

    // A changed name that is free is kept; invalid values are refused as for any save.
    changed.name = QStringLiteral("Fresh name");
    QVERIFY(store.saveAsCopy(changed, &copyId) == StoreStatus::Ok);
    QVERIFY(store.get(copyId, &copy));
    QCOMPARE(copy.name, QStringLiteral("Fresh name"));
    changed.tempo = phasesOf({ 21 }, 1);
    QList<MetronomeProblem> problems;
    QVERIFY(store.saveAsCopy(changed, nullptr, &problems) == StoreStatus::Invalid);
    QVERIFY(has(problems, MetronomeProblem::TempoInvalid));
    QCOMPARE(countRows(t_.db, "metronome"), 3);
}

void TstMetronomeStore::removingAUsedMetronomeClearsTheLinksAndKeepsTheExercises()
{
    MetronomeStore store(&t_.db);
    ExerciseStore exercises(&t_.db);
    int id = 0, other = 0;
    QVERIFY(store.create(make("Used"), &id) == StoreStatus::Ok);
    QVERIFY(store.create(make("Other"), &other) == StoreStatus::Ok);
    const QString second = link("Second", id);
    const QString first = link("First", id);
    const QString bystander = link("Bystander", other);
    Exercise before;
    QVERIFY(exercises.get(first, &before));

    QList<ExerciseRef> cleared;
    QVERIFY(store.remove(id, &cleared) == StoreStatus::Ok);
    QCOMPARE(cleared.size(), 2);
    QCOMPARE(cleared.at(0).name, QStringLiteral("First"));
    QCOMPARE(cleared.at(0).key, first);
    QCOMPARE(cleared.at(1).name, QStringLiteral("Second"));
    QCOMPARE(cleared.at(1).key, second);

    // The metronome is gone, the exercises are intact and no link is left, in the data as in the read.
    SavedMetronome m;
    QVERIFY(!store.get(id, &m));
    Exercise after;
    QVERIFY(exercises.get(first, &after));
    QCOMPARE(after.metronomeId, 0);
    QCOMPARE(after.name, before.name);
    QCOMPARE(after.muscles, before.muscles);
    QCOMPARE(after.instructions, before.instructions);
    QCOMPARE(after.tips, before.tips);
    QVERIFY(exercises.get(second, &after));
    QCOMPARE(after.metronomeId, 0);
    QSqlQuery q;
    QVERIFY(t_.db.run("SELECT COUNT(*) FROM exercise WHERE metronome_id IS NOT NULL", QVariantList(), &q) && q.next());
    QCOMPARE(q.value(0).toInt(), 1);
    // The exercise of another metronome is untouched.
    QVERIFY(exercises.get(bystander, &after));
    QCOMPARE(after.metronomeId, other);
    QVERIFY(store.get(other, &m));
    QVERIFY(integrityOk(t_.db));
}

void TstMetronomeStore::removingAnUnusedMetronomeReportsNothing()
{
    MetronomeStore store(&t_.db);
    int id = 0, keep = 0;
    QVERIFY(store.create(make("Unused"), &id) == StoreStatus::Ok);
    QVERIFY(store.create(make("Kept"), &keep) == StoreStatus::Ok);
    QList<ExerciseRef> cleared;
    QVERIFY(store.remove(id, &cleared) == StoreStatus::Ok);
    QVERIFY(cleared.isEmpty());
    SavedMetronome m;
    QVERIFY(!store.get(id, &m));
    QVERIFY(store.get(keep, &m));
    QCOMPARE(countRows(t_.db, "metronome"), 1);
}

void TstMetronomeStore::removingAMissingMetronomeIsNotFound()
{
    MetronomeStore store(&t_.db);
    QList<ExerciseRef> cleared;
    QVERIFY(store.remove(4242, &cleared) == StoreStatus::NotFound);
    QVERIFY(cleared.isEmpty());
}

void TstMetronomeStore::aFailingRemoveChangesNothing()
{
    MetronomeStore store(&t_.db);
    ExerciseStore exercises(&t_.db);
    int id = 0;
    QVERIFY(store.create(make("Used"), &id) == StoreStatus::Ok);
    const QString a = link("A", id);
    const QString b = link("B", id);
    // The delete of the metronome fails after the links have been cleared.
    QVERIFY(t_.db.run("CREATE TRIGGER refuse_delete BEFORE DELETE ON metronome "
                      "BEGIN SELECT RAISE(ABORT, 'refused'); END"));
    QList<ExerciseRef> cleared;
    QVERIFY(store.remove(id, &cleared) == StoreStatus::Failed);
    QVERIFY(cleared.isEmpty());
    SavedMetronome m;
    QVERIFY(store.get(id, &m));
    Exercise e;
    QVERIFY(exercises.get(a, &e));
    QCOMPARE(e.metronomeId, id);
    QVERIFY(exercises.get(b, &e));
    QCOMPARE(e.metronomeId, id);
    QCOMPARE(store.exercisesUsing(id).size(), 2);
    QVERIFY(integrityOk(t_.db));
    // Once the cause is gone the same call works.
    QVERIFY(t_.db.run("DROP TRIGGER refuse_delete"));
    QVERIFY(store.remove(id, &cleared) == StoreStatus::Ok);
    QCOMPARE(cleared.size(), 2);
}

void TstMetronomeStore::theDatabaseClearsTheLinkEvenOnARawDelete()
{
    // The foreign key is on for every connection, so a delete that bypasses the store leaves no
    // dangling link either.
    MetronomeStore store(&t_.db);
    ExerciseStore exercises(&t_.db);
    int id = 0;
    QVERIFY(store.create(make("Used"), &id) == StoreStatus::Ok);
    const QString key = link("A", id);
    QSqlQuery q;
    QVERIFY(t_.db.run("PRAGMA foreign_keys", QVariantList(), &q) && q.next());
    QCOMPARE(q.value(0).toInt(), 1);
    QVERIFY(t_.db.run("DELETE FROM metronome WHERE id = ?", QVariantList() << id));
    QVERIFY(t_.db.run("SELECT metronome_id FROM exercise WHERE key = ?", QVariantList() << key, &q) && q.next());
    QVERIFY(q.value(0).isNull());
    QVERIFY(integrityOk(t_.db));
}

void TstMetronomeStore::anUnreadableRhythmIsListedWithoutFigures()
{
    MetronomeStore store(&t_.db);
    int c = 0;
    QVERIFY(store.create(make("C"), &c) == StoreStatus::Ok);
    QVERIFY(t_.db.run("UPDATE metronome SET phases = 'garbage' WHERE id = ?", QVariantList() << c));
    // An unreadable rhythm is listed with no figures and fails validation instead of crashing.
    bool found = false;
    for (const MetronomeSummary &s : store.list()) {
        if (s.id == c) {
            found = true;
            QCOMPARE(s.repetitionMs, qint64(0));
        }
    }
    QVERIFY(found);
}

void TstMetronomeStore::problemsOfReportsWithoutWriting()
{
    MetronomeStore store(&t_.db);
    const int id = addMetronome(t_.db, "Pulse", Tempo::timePerRep(1500, 4));
    QVERIFY(id > 0);
    const int before = countRows(t_.db, "metronome");

    bool failed = true;
    QList<MetronomeProblem> found = store.problemsOf(make("pulse ", Tempo::timePerRep(1000, 3)), 0, &failed);
    QVERIFY(!failed);
    QVERIFY(has(found, MetronomeProblem::IdentityTaken));

    found = store.problemsOf(make("  ", Tempo::timePerRep(1000, 3)));
    QVERIFY(has(found, MetronomeProblem::NameEmpty));
    QVERIFY(!has(found, MetronomeProblem::IdentityTaken));
    QCOMPARE(countRows(t_.db, "metronome"), before);
}

void TstMetronomeStore::problemsOfSkipsTheCompareForAnUnchangedIdentity()
{
    MetronomeStore store(&t_.db);
    const int a = addMetronome(t_.db, "Pulse", Tempo::timePerRep(1500, 4));
    QVERIFY(a > 0);
    // The metronome being edited does not clash with itself.
    QVERIFY(store.problemsOf(make("PULSE", Tempo::timePerRep(1000, 3)), a).isEmpty());
    // Another new one with the same identity does.
    QVERIFY(has(store.problemsOf(make("PULSE", Tempo::timePerRep(1000, 3)), 0), MetronomeProblem::IdentityTaken));
    // Renaming it into another's identity does too.
    const int b = addMetronome(t_.db, "Other", Tempo::timePerRep(1500, 4));
    QVERIFY(b > 0);
    QVERIFY(has(store.problemsOf(make("pulse", Tempo::timePerRep(1000, 3)), b), MetronomeProblem::IdentityTaken));
}

QTEST_APPLESS_MAIN(TstMetronomeStore)

#include "tst_metronomestore.moc"

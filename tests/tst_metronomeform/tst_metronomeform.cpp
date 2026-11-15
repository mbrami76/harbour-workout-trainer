// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "metronomeform.h"
#include "testsupport.h"

namespace {

QVariantMap phasesMap()
{
    QVariantMap m;
    m.insert("form", "phases");
    m.insert("reps", 8);
    m.insert("phases", QVariantList() << 2 << 3 << 1);
    m.insert("phaseNames", QVariantList() << "Down" << "" << "Up");
    m.insert("name", "  Squat pace ");
    return m;
}

}

class TstMetronomeForm : public QObject
{
    Q_OBJECT

private slots:
    void phasesMapGivesTheTempoAndTheNames();
    void totalTimeAndTimePerRepMapsGiveTheirTempos();
    void aMissingKeyReadsAsItsDefault();
    void fromMapTrimsTheNameAndKeepsTheRest();
    void toMapRoundTripsEveryRhythm();
    void toMapHasOnlyTheRhythmKeys();
    void fromMapIgnoresTheKeysOfAPaceRun();
    void tempoProblemsHaveCodes();
    void metronomeProblemsHaveCodes();
    void codesAreListedOnceInOrder();
};

void TstMetronomeForm::phasesMapGivesTheTempoAndTheNames()
{
    const Tempo t = MetronomeForm::tempoFromMap(phasesMap());
    QVERIFY(t.isValid());
    QVERIFY(t.form() == TempoForm::Phases);
    QCOMPARE(t.reps(), 8);
    QCOMPARE(t.phaseUnitList(), QList<int>() << 2 << 3 << 1);
    QCOMPARE(t.phaseName(1), QStringLiteral("Down"));
    QCOMPARE(t.phaseName(2), QString());
    QCOMPARE(t.phaseName(3), QStringLiteral("Up"));
}

void TstMetronomeForm::totalTimeAndTimePerRepMapsGiveTheirTempos()
{
    QVariantMap total;
    total.insert("form", "reps-total");
    total.insert("reps", 7);
    total.insert("totalSec", 20);
    QVERIFY(MetronomeForm::tempoFromMap(total) == Tempo::repsAndTotal(7, 20000));

    QVariantMap perRep;
    perRep.insert("form", "time-per-rep");
    perRep.insert("reps", 4);
    perRep.insert("repHalfSec", 3);
    QVERIFY(MetronomeForm::tempoFromMap(perRep) == Tempo::timePerRep(1500, 4));
}

void TstMetronomeForm::aMissingKeyReadsAsItsDefault()
{
    const SavedMetronome m = MetronomeForm::fromMap(QVariantMap());
    QCOMPARE(m.id, 0);
    QVERIFY(m.name.isEmpty());
    QVERIFY(!m.tempo.isValid());
    QCOMPARE(m.tempo.problem(), TempoProblem::PhaseCountOutOfRange);
}

void TstMetronomeForm::fromMapTrimsTheNameAndKeepsTheRest()
{
    QVariantMap map = phasesMap();
    map.insert("id", 12);
    map.insert("key", "m-abc");
    const SavedMetronome m = MetronomeForm::fromMap(map);
    QCOMPARE(m.id, 12);
    QCOMPARE(m.key, QStringLiteral("m-abc"));
    QCOMPARE(m.name, QStringLiteral("Squat pace"));
    QVERIFY(m.tempo.isValid());
    QCOMPARE(m.tempo.reps(), 1);
}

void TstMetronomeForm::toMapRoundTripsEveryRhythm()
{
    const Tempo rhythms[] = { phasesOf({ 1 }, 1), phasesOf({ 2, 3, 1 }, 1),
                              Tempo::phases(QList<int>() << 2 << 4, 1, QStringList() << "In" << "Out") };
    for (const Tempo &tempo : rhythms) {
        SavedMetronome m;
        m.id = 3;
        m.key = "m-1";
        m.name = "Name";
        m.tempo = tempo;
        const SavedMetronome back = MetronomeForm::fromMap(MetronomeForm::toMap(m));
        QVERIFY(back.tempo == tempo);
        QCOMPARE(back.id, 3);
        QCOMPARE(back.key, m.key);
        QCOMPARE(back.name, m.name);
    }
}

void TstMetronomeForm::toMapHasOnlyTheRhythmKeys()
{
    SavedMetronome m;
    m.name = "N";
    m.tempo = phasesOf({ 3, 4 }, 1);
    const QVariantMap map = MetronomeForm::toMap(m);
    QCOMPARE(map.keys().size(), 5);
    QVERIFY(!map.contains("icon"));
    QVERIFY(!map.contains("colour"));
    QCOMPARE(map.value("phases").toList().size(), 2);
    QVERIFY(map.value("phaseNames").toList().isEmpty() || map.value("phaseNames").toList().size() == 2);
    QVERIFY(!map.contains("form"));
    QVERIFY(!map.contains("reps"));
    QVERIFY(!map.contains("sets"));
    QVERIFY(!map.contains("restSec"));
}

void TstMetronomeForm::fromMapIgnoresTheKeysOfAPaceRun()
{
    // The keys of a quick mode do not belong to a rhythm: the rhythm has one repetition.
    const SavedMetronome m = MetronomeForm::fromMap(phasesMap());
    QCOMPARE(m.tempo.reps(), 1);
    QVERIFY(m.tempo.form() == TempoForm::Phases);
}

void TstMetronomeForm::tempoProblemsHaveCodes()
{
    QCOMPARE(MetronomeForm::tempoProblemCode(TempoProblem::None), QString());
    QCOMPARE(MetronomeForm::tempoProblemCode(TempoProblem::RepsOutOfRange), QStringLiteral("reps"));
    QCOMPARE(MetronomeForm::tempoProblemCode(TempoProblem::RepTooShort), QStringLiteral("rep-short"));
    QCOMPARE(MetronomeForm::tempoProblemCode(TempoProblem::RepTooLong), QStringLiteral("rep-long"));
    QCOMPARE(MetronomeForm::tempoProblemCode(TempoProblem::PhaseCountOutOfRange), QStringLiteral("phase-count"));
    QCOMPARE(MetronomeForm::tempoProblemCode(TempoProblem::PhaseOutOfRange), QStringLiteral("phase-length"));
    QCOMPARE(MetronomeForm::tempoProblemCode(TempoProblem::PhaseNameTooLong), QStringLiteral("phase-name"));
}

void TstMetronomeForm::metronomeProblemsHaveCodes()
{
    struct Case { MetronomeProblem::Code code; const char *text; };
    const Case cases[] = {
        { MetronomeProblem::NameEmpty, "name-empty" }, { MetronomeProblem::NameTooLong, "name-too-long" },
        { MetronomeProblem::IdentityTaken, "identity" } };
    for (const Case &c : cases) {
        MetronomeProblem p;
        p.code = c.code;
        QCOMPARE(MetronomeForm::problemCode(p), QString::fromLatin1(c.text));
    }
    MetronomeProblem tempo;
    tempo.code = MetronomeProblem::TempoInvalid;
    tempo.tempoProblem = TempoProblem::RepTooShort;
    QCOMPARE(MetronomeForm::problemCode(tempo), QStringLiteral("rep-short"));
}

void TstMetronomeForm::codesAreListedOnceInOrder()
{
    QList<MetronomeProblem> list;
    MetronomeProblem p;
    p.code = MetronomeProblem::IdentityTaken;
    list.append(p);
    p.code = MetronomeProblem::NameEmpty;
    list.append(p);
    p.code = MetronomeProblem::IdentityTaken;
    list.append(p);
    QCOMPARE(MetronomeForm::problemCodes(list), QStringList() << "identity" << "name-empty");
}

QTEST_APPLESS_MAIN(TstMetronomeForm)

#include "tst_metronomeform.moc"

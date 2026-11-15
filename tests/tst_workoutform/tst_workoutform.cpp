// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "workoutform.h"

namespace {

Step timed(const QString &key, int sec)
{
    Step s;
    s.exerciseKey = key;
    s.durationSec = sec;
    return s;
}

Step counted(StepType type, const QString &key, int reps)
{
    Step s;
    s.type = type;
    s.exerciseKey = key;
    s.reps = reps;
    return s;
}

}

class TstWorkoutForm : public QObject
{
    Q_OBJECT

private slots:
    void stepMapCarriesEveryField();
    void blockMapCarriesTheCounts();
    void headerMapCarriesTheIdentity();
    void everyProblemHasADistinctCode();
    void problemListKeepsOrderAndPlaces();
    void uncountedStepsAreRepetitionStepsWithoutATempo();
    void aStepCountsOnceWhateverTheRounds();
};

void TstWorkoutForm::stepMapCarriesEveryField()
{
    Step s = counted(StepType::Cooldown, "key", 8);
    s.startOnTap = true;
    const QVariantMap m = WorkoutForm::stepToMap(s);
    QCOMPARE(m.value("type").toString(), QStringLiteral("cooldown"));
    QCOMPARE(m.value("exerciseKey").toString(), QStringLiteral("key"));
    QCOMPARE(m.value("reps").toInt(), 8);
    QCOMPARE(m.value("durationSec").toInt(), 0);
    QVERIFY(m.value("startOnTap").toBool());
    QVERIFY(!m.value("timed").toBool());
    QVERIFY(WorkoutForm::stepToMap(timed("a", 5)).value("timed").toBool());
}

void TstWorkoutForm::blockMapCarriesTheCounts()
{
    Block b;
    b.rounds = 3;
    b.restBetweenRoundsSec = 20;
    b.steps << timed("a", 5) << timed("b", 5);
    const QVariantMap m = WorkoutForm::blockToMap(b);
    QCOMPARE(m.value("rounds").toInt(), 3);
    QCOMPARE(m.value("restBetweenRoundsSec").toInt(), 20);
    QCOMPARE(m.value("stepCount").toInt(), 2);
}

void TstWorkoutForm::headerMapCarriesTheIdentity()
{
    Workout w;
    w.id = 7;
    w.name = QStringLiteral("Legs");
    w.icon = QStringLiteral("bike");
    w.colour = QStringLiteral("teal");
    w.restBetweenBlocksSec = 30;
    Block b;
    b.steps << timed("a", 5) << timed("b", 5);
    w.blocks << b << b;
    const QVariantMap m = WorkoutForm::headerToMap(w);
    QCOMPARE(m.value("id").toInt(), 7);
    QCOMPARE(m.value("name").toString(), QStringLiteral("Legs"));
    QCOMPARE(m.value("icon").toString(), QStringLiteral("bike"));
    QCOMPARE(m.value("colour").toString(), QStringLiteral("teal"));
    QCOMPARE(m.value("restBetweenBlocksSec").toInt(), 30);
    QCOMPARE(m.value("blockCount").toInt(), 2);
    QCOMPARE(m.value("stepCount").toInt(), 4);
}

void TstWorkoutForm::everyProblemHasADistinctCode()
{
    QSet<QString> seen;
    for (int c = Problem::NoBlocks; c <= Problem::DurationWithRhythm; ++c) {
        const QString code = WorkoutForm::problemCode(Problem::Code(c));
        QVERIFY2(code != QLatin1String("failed"), qPrintable(QString::number(c)));
        QVERIFY2(!seen.contains(code), qPrintable(code));
        seen.insert(code);
    }
}

void TstWorkoutForm::problemListKeepsOrderAndPlaces()
{
    Problem first;
    first.code = Problem::EmptyBlock;
    first.blockIndex = 2;
    Problem second;
    second.code = Problem::DurationOutOfRange;
    second.blockIndex = 0;
    second.stepIndex = 4;
    const QVariantList list = WorkoutForm::problemList(QList<Problem>() << first << second);
    QCOMPARE(list.size(), 2);
    QCOMPARE(list.at(0).toMap().value("code").toString(), QStringLiteral("empty-block"));
    QCOMPARE(list.at(0).toMap().value("block").toInt(), 2);
    QCOMPARE(list.at(0).toMap().value("step").toInt(), -1);
    QCOMPARE(list.at(1).toMap().value("code").toString(), QStringLiteral("duration"));
    QCOMPARE(list.at(1).toMap().value("step").toInt(), 4);
}

void TstWorkoutForm::uncountedStepsAreRepetitionStepsWithoutATempo()
{
    Workout w;
    Block b;
    b.steps << timed("a", 30)                               // timed: counted
            << counted(StepType::Work, "b", 10)             // no tempo: left out
            << counted(StepType::Warmup, "c", 10)           // tempo: counted
            << counted(StepType::Cooldown, "d", 10);        // no tempo: left out
    w.blocks << b;
    ExerciseTempos tempos;
    tempos.insert("c", Tempo::timePerRep(2000, 10));
    const QList<WorkoutForm::StepRef> found = WorkoutForm::uncountedSteps(w, tempos);
    QCOMPARE(found.size(), 2);
    QCOMPARE(found.at(0).step, 1);
    QCOMPARE(found.at(1).step, 3);
}

void TstWorkoutForm::aStepCountsOnceWhateverTheRounds()
{
    Workout w;
    Block b;
    b.rounds = 5;
    b.steps << counted(StepType::Work, "b", 10);
    w.blocks << b;
    QCOMPARE(WorkoutForm::uncountedSteps(w, ExerciseTempos()).size(), 1);
}

QTEST_APPLESS_MAIN(TstWorkoutForm)
#include "tst_workoutform.moc"

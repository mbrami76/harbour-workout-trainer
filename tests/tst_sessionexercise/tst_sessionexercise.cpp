// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "engine.h"
#include "fakeclock.h"
#include "sessionexercise.h"
#include "testsupport.h"

// The exercise file of a session (R-TMR-12): which exercise the button refers to in each state.
namespace {

QList<PlanStep> planOf(const QList<Step> &steps)
{
    return expandPlan(workout(QList<Block>() << block(steps, 1)), tempoSnapshot());
}

QString keyAt(const QList<PlanStep> &plan, int index)
{
    const PlanStep *next = index + 1 < plan.size() ? &plan.at(index + 1) : nullptr;
    return SessionExercise::fileKey(&plan.at(index), next);
}

}

class TstSessionExercise : public QObject
{
    Q_OBJECT

private slots:
    void duringAStepWithAnExerciseItIsThatExercise();
    void duringARestItIsTheNextExercise();
    void aRestWithNothingAfterItHasNoFile();
    void aRestBeforeAStepWithoutAnExerciseHasNoFile();
    void aStepWithoutAnExerciseHasNoFile();
    void theLeadInBeforeTheFirstStepOffersItsExercise();
    void theEngineStatesDoNotChangeIt();
};

void TstSessionExercise::duringAStepWithAnExerciseItIsThatExercise()
{
    const QList<PlanStep> plan = planOf(QList<Step>() << work("squat", 30) << rest(10) << work("plank", 30));
    QCOMPARE(keyAt(plan, 0), QStringLiteral("squat"));
    QCOMPARE(keyAt(plan, 2), QStringLiteral("plank"));
    // A repetition step, with or without a tempo, is a step with an exercise.
    const QList<PlanStep> reps = planOf(QList<Step>() << repStep("squat", 10));
    QCOMPARE(keyAt(reps, 0), QStringLiteral("squat"));
}

void TstSessionExercise::duringARestItIsTheNextExercise()
{
    const QList<PlanStep> plan = planOf(QList<Step>() << work("squat", 30) << rest(10) << work("plank", 30));
    QCOMPARE(keyAt(plan, 1), QStringLiteral("plank"));
}

void TstSessionExercise::aRestWithNothingAfterItHasNoFile()
{
    const QList<PlanStep> plan = planOf(QList<Step>() << work("squat", 30) << rest(10));
    QVERIFY(keyAt(plan, 1).isEmpty());
}

void TstSessionExercise::aRestBeforeAStepWithoutAnExerciseHasNoFile()
{
    const QList<PlanStep> plan = planOf(QList<Step>() << work("squat", 30) << rest(10) << work("", 30));
    QVERIFY(keyAt(plan, 1).isEmpty());
}

void TstSessionExercise::aStepWithoutAnExerciseHasNoFile()
{
    const QList<PlanStep> plan = planOf(QList<Step>() << work("", 30) << work("squat", 30));
    QVERIFY(keyAt(plan, 0).isEmpty());
    QVERIFY(SessionExercise::fileKey(nullptr, nullptr).isEmpty());
}

void TstSessionExercise::theLeadInBeforeTheFirstStepOffersItsExercise()
{
    // During the lead-in the session is at its first step.
    const QList<PlanStep> plan = planOf(QList<Step>() << work("squat", 30) << work("plank", 30));
    FakeClock clock;
    EngineConfig config;
    config.leadInSec = 3;
    SessionEngine engine(&clock, plan, config);
    engine.start();
    QVERIFY(engine.state() == SessionState::LeadIn);
    QCOMPARE(keyAt(engine.plan(), engine.stepIndex()), QStringLiteral("squat"));
    // A first step that is a rest offers the exercise after it.
    const QList<PlanStep> restFirst = planOf(QList<Step>() << rest(10) << work("plank", 30));
    SessionEngine second(&clock, restFirst, config);
    second.start();
    QCOMPARE(keyAt(second.plan(), second.stepIndex()), QStringLiteral("plank"));
}

void TstSessionExercise::theEngineStatesDoNotChangeIt()
{
    // Running, paused and waiting for a tap are all at the same step: the same exercise.
    QList<Step> steps;
    steps << work("squat", 30) << onTap(work("plank", 30));
    const QList<PlanStep> plan = planOf(steps);
    FakeClock clock;
    EngineConfig config;
    config.leadInSec = 0;
    SessionEngine engine(&clock, plan, config);
    engine.start();
    QCOMPARE(keyAt(engine.plan(), engine.stepIndex()), QStringLiteral("squat"));
    engine.pause();
    QVERIFY(engine.state() == SessionState::Paused);
    QCOMPARE(keyAt(engine.plan(), engine.stepIndex()), QStringLiteral("squat"));
    engine.resume();
    clock.set(30000);
    engine.update();
    QVERIFY(engine.state() == SessionState::WaitingToStart);
    QCOMPARE(keyAt(engine.plan(), engine.stepIndex()), QStringLiteral("plank"));
}

QTEST_APPLESS_MAIN(TstSessionExercise)
#include "tst_sessionexercise.moc"

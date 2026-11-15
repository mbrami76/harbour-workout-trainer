// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "engine.h"
#include "fakeclock.h"
#include "sessioncontrols.h"
#include "testsupport.h"

using namespace SessionControls;

// What the three buttons do in each state of the session (R-UI-2, R-TMR-3, R-TMR-4, R-TMR-11, R-WRK-11),
// and what the engine does for a tap and a long press on skip.
namespace {

PlanStep timedStep() { PlanStep s; s.type = StepType::Work; s.durationSec = 30; return s; }
PlanStep repsStep() { PlanStep s; s.type = StepType::Work; s.reps = 10; return s; }
PlanStep pacedStep() { PlanStep s = repsStep(); s.hasTempo = true; return s; }
PlanStep restStep() { PlanStep s; s.type = StepType::Rest; s.durationSec = 20; return s; }

}

class TstSessionControls : public QObject
{
    Q_OBJECT

private slots:
    void aTimedStepPausesAndSkips();
    void aRepetitionStepWithoutATempoHasDone();
    void aPacedStepEndsEarly();
    void theLeadInAndARestPauseAndSkip();
    void whilePausedPlayResumes();
    void aStepThatWaitsForATapIsStartedByPlay();
    void stopAndSkipHoldWorkWhereverTheSessionIsOn();
    void nothingWorksOutsideASession();
    void onlyStopWorksWhilePreparing();
    void aTapOnSkipGoesToTheNextStepInEveryState();
    void aLongPressOnSkipGoesToTheNextBlockInEveryState();
};

void TstSessionControls::aTimedStepPausesAndSkips()
{
    const PlanStep step = timedStep();
    const Controls c = forState(SessionState::Running, &step);
    QVERIFY(c.play == Play::Pause);
    QVERIFY(c.skipTap == Skip::NextStep);
}

void TstSessionControls::aRepetitionStepWithoutATempoHasDone()
{
    const PlanStep step = repsStep();
    const Controls c = forState(SessionState::WaitingForReps, &step);
    QVERIFY(c.play == Play::Pause);
    QVERIFY(c.skipTap == Skip::Done);
    QVERIFY(c.skipHold);
}

void TstSessionControls::aPacedStepEndsEarly()
{
    const PlanStep step = pacedStep();
    const Controls c = forState(SessionState::Running, &step);
    QVERIFY(c.skipTap == Skip::EndSet);
    QVERIFY(c.skipHold);
}

void TstSessionControls::theLeadInAndARestPauseAndSkip()
{
    const PlanStep step = restStep();
    for (SessionState s : { SessionState::LeadIn, SessionState::WaitingForVoice, SessionState::Running }) {
        const Controls c = forState(s, &step);
        QVERIFY(c.play == Play::Pause);
        QVERIFY(c.skipTap == Skip::NextStep);
    }
}

void TstSessionControls::whilePausedPlayResumes()
{
    // The step it paused in does not change what the buttons do, also a paced one.
    const PlanStep paced = pacedStep();
    const PlanStep reps = repsStep();
    for (const PlanStep *step : { &paced, &reps }) {
        const Controls c = forState(SessionState::Paused, step);
        QVERIFY(c.play == Play::Resume);
        QVERIFY(c.skipTap == Skip::NextStep);
        QVERIFY(c.skipHold);
        QVERIFY(c.stopHold);
    }
}

void TstSessionControls::aStepThatWaitsForATapIsStartedByPlay()
{
    const PlanStep step = timedStep();
    const Controls c = forState(SessionState::WaitingToStart, &step);
    QVERIFY(c.play == Play::Start);
    QVERIFY(c.skipTap == Skip::NextStep);
    QVERIFY(c.skipHold);
}

void TstSessionControls::stopAndSkipHoldWorkWhereverTheSessionIsOn()
{
    const PlanStep step = timedStep();
    for (SessionState s : { SessionState::LeadIn, SessionState::WaitingForVoice, SessionState::Running,
                            SessionState::WaitingForReps, SessionState::Paused, SessionState::WaitingToStart }) {
        const Controls c = forState(s, &step);
        QVERIFY(c.stopHold);
        QVERIFY(c.skipHold);
    }
}

void TstSessionControls::nothingWorksOutsideASession()
{
    for (SessionState s : { SessionState::Ready, SessionState::Finished, SessionState::Stopped }) {
        const Controls c = forState(s, nullptr);
        QVERIFY(c.play == Play::None);
        QVERIFY(c.skipTap == Skip::None);
        QVERIFY(!c.skipHold);
        QVERIFY(!c.stopHold);
    }
}

void TstSessionControls::onlyStopWorksWhilePreparing()
{
    const Controls c = whilePreparing();
    QVERIFY(c.play == Play::None);
    QVERIFY(c.skipTap == Skip::None);
    QVERIFY(!c.skipHold);
    QVERIFY(c.stopHold);
}

namespace {

// Two blocks of two timed steps each.
QList<PlanStep> plan()
{
    QList<Block> blocks;
    blocks << block(QList<Step>() << work("a", 10) << work("b", 10), 1);
    blocks << block(QList<Step>() << work("c", 10) << work("d", 10), 1);
    return expandPlan(workout(blocks));
}

}

void TstSessionControls::aTapOnSkipGoesToTheNextStepInEveryState()
{
    for (int variant = 0; variant < 3; ++variant) {
        FakeClock clock;
        EngineConfig config;
        config.leadInSec = variant == 0 ? 3 : 0;
        SessionEngine engine(&clock, plan(), config);
        engine.start();
        if (variant == 2) {
            clock.set(1000);
            engine.pause();
        }
        engine.next();
        QCOMPARE(engine.stepIndex(), 1);
        QCOMPARE(engine.currentStep().blockIndex, 0);
    }
}

void TstSessionControls::aLongPressOnSkipGoesToTheNextBlockInEveryState()
{
    for (int variant = 0; variant < 3; ++variant) {
        FakeClock clock;
        EngineConfig config;
        config.leadInSec = variant == 0 ? 3 : 0;
        SessionEngine engine(&clock, plan(), config);
        engine.start();
        if (variant == 2) {
            clock.set(1000);
            engine.pause();
        }
        engine.nextBlock();
        QCOMPARE(engine.stepIndex(), 2);
        QCOMPARE(engine.currentStep().blockIndex, 1);
    }
}

QTEST_APPLESS_MAIN(TstSessionControls)
#include "tst_sessioncontrols.moc"

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include <functional>
#include <random>

#include "engine.h"
#include "fakeclock.h"
#include "plan.h"
#include "testsupport.h"

typedef QList<qint64> TimeList;

class TstEngine : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void fullWorkoutWithoutLeadIn();
    void fullWorkoutWithLeadIn();
    void readOnlyState();
    void repetitionStep();
    void ticksAreEmittedOnce();
    void shortStepSkipsTicks();
    void customTickList();
    void leadInTicks();
    void roundAndBlockChange();
    void pauseInTimedStep();
    void pauseInLeadIn();
    void pauseInRepetitionStep();
    void pauseWithoutLeadIn();
    void pauseInRestStep();
    void pauseOverLongTime();
    void pauseAndResumeOutOfContext();
    void pauseDuringResumeLeadIn();
    void nextStep();
    void nextThroughLeadIn();
    void nextWhilePaused();
    void nextBlockSkipsTheRestOfTheBlock();
    void nextBlockInTheLastBlockEnds();
    void nextBlockInRestsLeadInAndPause();
    void nextBlockWhileWaitingForTap();
    void elapsedTimeCountsActiveTimeOnly();
    void stopEndsSession();
    void finalStateIgnoresCalls();
    void doneOutsideWaitingStep();
    void lateUpdateCrossesBoundaries();
    void lateUpdateAcrossLeadIns();
    void emptyPlanFinishes();
    void leadInIsClamped();
    void longWorkoutWithIrregularUpdates();

private:
    void make(const QList<Step> &steps, int leadInSec, int rounds = 1, int restBetweenRounds = 0);
    void make(const QList<PlanStep> &plan, const EngineConfig &config);
    void take(const QList<EngineEvent> &events) { log_ += events; }
    void at(qint64 ms) { clock_.set(ms); take(engine_->update()); }
    TimeList times(EventType type) const;
    QList<int> indices(EventType type) const;
    TimeList tickTimes(bool inLeadIn) const;
    QList<int> tickValues(bool inLeadIn) const;
    bool is(SessionState state) const { return engine_->state() == state; }

    FakeClock clock_;
    QScopedPointer<SessionEngine> engine_;
    QList<EngineEvent> log_;
};

void TstEngine::init()
{
    clock_.set(0);
    log_.clear();
    engine_.reset();
}

void TstEngine::make(const QList<Step> &steps, int leadInSec, int rounds, int restBetweenRounds)
{
    EngineConfig config;
    config.leadInSec = leadInSec;
    make(expandPlan(workout(QList<Block>() << block(steps, rounds, restBetweenRounds))), config);
}

void TstEngine::make(const QList<PlanStep> &plan, const EngineConfig &config)
{
    engine_.reset(new SessionEngine(&clock_, plan, config));
}

TimeList TstEngine::times(EventType type) const
{
    TimeList list;
    for (const EngineEvent &e : log_) {
        if (e.type == type)
            list.append(e.atMs);
    }
    return list;
}

QList<int> TstEngine::indices(EventType type) const
{
    QList<int> list;
    for (const EngineEvent &e : log_) {
        if (e.type == type)
            list.append(e.stepIndex);
    }
    return list;
}

TimeList TstEngine::tickTimes(bool inLeadIn) const
{
    TimeList list;
    for (const EngineEvent &e : log_) {
        if (e.type == EventType::Tick && e.inLeadIn == inLeadIn)
            list.append(e.atMs);
    }
    return list;
}

QList<int> TstEngine::tickValues(bool inLeadIn) const
{
    QList<int> list;
    for (const EngineEvent &e : log_) {
        if (e.type == EventType::Tick && e.inLeadIn == inLeadIn)
            list.append(e.remainingSec);
    }
    return list;
}

void TstEngine::fullWorkoutWithoutLeadIn()
{
    make(QList<Step>() << work("a", 5) << rest(3) << work("b", 4), 0);
    QVERIFY(is(SessionState::Ready));
    take(engine_->start());
    for (qint64 t = 1000; t <= 12000; t += 1000)
        at(t);

    QCOMPARE(times(EventType::StepStarted), TimeList() << 0 << 5000 << 8000);
    QCOMPARE(times(EventType::Finished), TimeList() << 12000);
    QVERIFY(is(SessionState::Finished));
    QCOMPARE(engine_->records().size(), 3);
    QCOMPARE(engine_->records().at(0).activeMs, qint64(5000));
    QCOMPARE(engine_->records().at(1).activeMs, qint64(3000));
    QCOMPARE(engine_->records().at(2).activeMs, qint64(4000));
}

void TstEngine::fullWorkoutWithLeadIn()
{
    // The rest step has no lead-in: a 3 s, rest 3 s, b 3 s countdown precedes a and b only.
    make(QList<Step>() << work("a", 5) << rest(3) << work("b", 4), 3);
    take(engine_->start());
    QVERIFY(is(SessionState::LeadIn));
    at(1000);
    QCOMPARE(engine_->leadInRemainingMs(), qint64(2000));
    QCOMPARE(engine_->stepRemainingMs(), qint64(5000));
    for (qint64 t = 2000; t <= 18000; t += 1000)
        at(t);

    QCOMPARE(times(EventType::StepStarted), TimeList() << 3000 << 8000 << 14000);
    QCOMPARE(times(EventType::Finished), TimeList() << 18000);
    QCOMPARE(engine_->records().at(0).activeMs, qint64(5000));
    QCOMPARE(engine_->records().at(2).activeMs, qint64(4000));
}

void TstEngine::readOnlyState()
{
    make(QList<Step>() << work("a", 10) << work("b", 10), 0);
    QCOMPARE(engine_->stepCount(), 2);
    take(engine_->start());
    QCOMPARE(engine_->stepIndex(), 0);
    QCOMPARE(engine_->currentStep().exerciseKey, QStringLiteral("a"));
    clock_.set(2500);
    QCOMPARE(engine_->stepRemainingMs(), qint64(7500));
    QCOMPARE(engine_->currentStepActiveMs(), qint64(2500));
    at(10000);
    QCOMPARE(engine_->stepIndex(), 1);
    QCOMPARE(engine_->leadInRemainingMs(), qint64(0));
}

void TstEngine::repetitionStep()
{
    make(QList<Step>() << work("a", 4) << repStep("b", 10) << rest(2), 0);
    take(engine_->start());
    at(4000);
    QVERIFY(is(SessionState::WaitingForReps));
    QCOMPARE(engine_->stepIndex(), 1);
    QCOMPARE(engine_->stepRemainingMs(), qint64(0));

    at(9000);
    QVERIFY(is(SessionState::WaitingForReps));
    QVERIFY(engine_->update().isEmpty());
    take(engine_->done());
    QVERIFY(is(SessionState::Running));
    QCOMPARE(engine_->stepIndex(), 2);
    QCOMPARE(times(EventType::StepStarted), TimeList() << 0 << 4000 << 9000);
    // No ticks for a repetition step.
    for (const EngineEvent &e : log_) {
        if (e.type == EventType::Tick)
            QVERIFY(e.stepIndex != 1);
    }
    at(11000);
    QVERIFY(is(SessionState::Finished));
    QCOMPARE(engine_->records().at(1).activeMs, qint64(5000));
}

void TstEngine::ticksAreEmittedOnce()
{
    make(QList<Step>() << work("a", 20), 0);
    take(engine_->start());
    for (qint64 t = 250; t <= 20000; t += 250) {
        at(t);
        at(t);
    }
    QCOMPARE(tickValues(false), QList<int>() << 10 << 5 << 3 << 2 << 1);
    QCOMPARE(tickTimes(false), TimeList() << 10000 << 15000 << 17000 << 18000 << 19000);
}

void TstEngine::shortStepSkipsTicks()
{
    make(QList<Step>() << work("a", 4), 0);
    take(engine_->start());
    at(4000);
    QCOMPARE(tickValues(false), QList<int>() << 3 << 2 << 1);

    log_.clear();
    clock_.set(0);
    make(QList<Step>() << work("a", 3), 0);
    take(engine_->start());
    at(3000);
    QCOMPARE(tickValues(false), QList<int>() << 2 << 1);
    QCOMPARE(tickTimes(false), TimeList() << 1000 << 2000);

    log_.clear();
    clock_.set(0);
    make(QList<Step>() << work("a", 1), 0);
    take(engine_->start());
    at(1000);
    QVERIFY(tickValues(false).isEmpty());
}

void TstEngine::customTickList()
{
    EngineConfig config;
    config.leadInSec = 0;
    config.tickSeconds = QList<int>() << 1 << 7 << 30 << 7 << 0 << -2;
    make(expandPlan(workout(QList<Block>() << block(QList<Step>() << work("a", 40)))), config);
    take(engine_->start());
    at(40000);
    QCOMPARE(tickValues(false), QList<int>() << 30 << 7 << 1);
    QCOMPARE(tickTimes(false), TimeList() << 10000 << 33000 << 39000);

    // An empty list means no ticks at all.
    log_.clear();
    clock_.set(0);
    config.tickSeconds.clear();
    make(expandPlan(workout(QList<Block>() << block(QList<Step>() << work("a", 40)))), config);
    take(engine_->start());
    at(40000);
    QVERIFY(times(EventType::Tick).isEmpty());
}

void TstEngine::leadInTicks()
{
    make(QList<Step>() << work("a", 20), 10);
    take(engine_->start());
    at(10000);
    at(30000);
    // Lead-in: 10 s is not below the lead-in length and is skipped.
    QCOMPARE(tickValues(true), QList<int>() << 5 << 3 << 2 << 1);
    QCOMPARE(tickTimes(true), TimeList() << 5000 << 7000 << 8000 << 9000);
    QCOMPARE(tickTimes(false), TimeList() << 20000 << 25000 << 27000 << 28000 << 29000);
    QCOMPARE(times(EventType::StepStarted), TimeList() << 10000);

    log_.clear();
    clock_.set(0);
    make(QList<Step>() << work("a", 20), 3);
    take(engine_->start());
    at(3000);
    QCOMPARE(tickValues(true), QList<int>() << 2 << 1);
}

void TstEngine::roundAndBlockChange()
{
    make(QList<Step>() << work("a", 3), 0, 2);
    take(engine_->start());
    QVERIFY(times(EventType::RoundChanged).isEmpty());
    at(6000);
    QCOMPARE(times(EventType::RoundChanged), TimeList() << 3000);

    // The change is reported before the start of the step that opens the round.
    int changed = -1, started = -1;
    for (int i = 0; i < log_.size(); ++i) {
        if (log_.at(i).type == EventType::RoundChanged)
            changed = i;
        if (log_.at(i).type == EventType::StepStarted && log_.at(i).atMs == 3000)
            started = i;
    }
    QVERIFY(changed >= 0 && started > changed);

    // A rest between rounds keeps the round it follows: the change comes with the next work step.
    log_.clear();
    clock_.set(0);
    make(QList<Step>() << work("a", 3), 0, 2, 2);
    take(engine_->start());
    at(8000);
    QCOMPARE(times(EventType::StepStarted), TimeList() << 0 << 3000 << 5000);
    QCOMPARE(times(EventType::RoundChanged), TimeList() << 5000);

    // A new block is a change as well.
    log_.clear();
    clock_.set(0);
    EngineConfig config;
    config.leadInSec = 0;
    make(expandPlan(workout(QList<Block>()
        << block(QList<Step>() << work("a", 2))
        << block(QList<Step>() << work("b", 2)))), config);
    take(engine_->start());
    at(4000);
    QCOMPARE(times(EventType::RoundChanged), TimeList() << 2000);
}

void TstEngine::pauseInTimedStep()
{
    make(QList<Step>() << work("a", 10), 3);
    take(engine_->start());
    at(5000);
    QCOMPARE(engine_->stepRemainingMs(), qint64(8000));
    take(engine_->pause());
    QVERIFY(is(SessionState::Paused));
    QCOMPARE(engine_->stepRemainingMs(), qint64(8000));

    at(100000);
    QVERIFY(is(SessionState::Paused));
    QCOMPARE(engine_->stepRemainingMs(), qint64(8000));

    take(engine_->resume());
    QVERIFY(is(SessionState::LeadIn));
    QCOMPARE(engine_->leadInRemainingMs(), qint64(3000));
    QCOMPARE(engine_->stepRemainingMs(), qint64(8000));
    at(103000);
    QVERIFY(is(SessionState::Running));
    QCOMPARE(engine_->stepRemainingMs(), qint64(8000));
    at(111000);

    QVERIFY(is(SessionState::Finished));
    QCOMPARE(times(EventType::StepStarted), TimeList() << 3000);
    QCOMPARE(times(EventType::Finished), TimeList() << 111000);
    QCOMPARE(engine_->records().at(0).activeMs, qint64(10000));
    // Ticks after the resume come from the restored remaining time and do not repeat.
    QCOMPARE(tickTimes(false), TimeList() << 106000 << 108000 << 109000 << 110000);
}

void TstEngine::pauseInLeadIn()
{
    make(QList<Step>() << work("a", 10), 3);
    take(engine_->start());
    at(1000);
    take(engine_->pause());
    QVERIFY(is(SessionState::Paused));
    at(50000);
    QVERIFY(times(EventType::StepStarted).isEmpty());

    take(engine_->resume());
    QVERIFY(is(SessionState::LeadIn));
    QCOMPARE(engine_->leadInRemainingMs(), qint64(3000));
    at(53000);
    QCOMPARE(times(EventType::StepStarted), TimeList() << 53000);
    at(63000);
    QCOMPARE(times(EventType::Finished), TimeList() << 63000);
    QCOMPARE(engine_->records().at(0).activeMs, qint64(10000));
}

void TstEngine::pauseInRepetitionStep()
{
    make(QList<Step>() << repStep("a", 10) << work("b", 5), 3);
    take(engine_->start());
    at(3000);
    QVERIFY(is(SessionState::WaitingForReps));
    at(4000);
    take(engine_->pause());
    QVERIFY(is(SessionState::Paused));
    QVERIFY(engine_->done().isEmpty());
    QVERIFY(is(SessionState::Paused));

    at(10000);
    take(engine_->resume());
    QVERIFY(is(SessionState::LeadIn));
    at(13000);
    QVERIFY(is(SessionState::WaitingForReps));
    at(14000);
    take(engine_->done());
    QCOMPARE(engine_->records().at(0).activeMs, qint64(2000));
    QCOMPARE(times(EventType::StepStarted), TimeList() << 3000);
    at(17000);
    QCOMPARE(times(EventType::StepStarted), TimeList() << 3000 << 17000);
}

void TstEngine::pauseWithoutLeadIn()
{
    make(QList<Step>() << work("a", 10), 0);
    take(engine_->start());
    at(4000);
    take(engine_->pause());
    QCOMPARE(engine_->stepRemainingMs(), qint64(6000));
    at(9000);
    take(engine_->resume());
    QVERIFY(is(SessionState::Running));
    QCOMPARE(engine_->stepRemainingMs(), qint64(6000));
    at(15000);
    QCOMPARE(times(EventType::Finished), TimeList() << 15000);
}

void TstEngine::pauseInRestStep()
{
    // a: 3 s lead-in, 10 s from 3000 to 13000; rest 20 s from 13000 to 33000.
    const QList<Step> steps = QList<Step>() << work("a", 10) << rest(20);

    // More than 5 s left: resumes at once.
    make(steps, 3);
    take(engine_->start());
    at(20000);
    take(engine_->pause());
    at(30000);
    take(engine_->resume());
    QVERIFY(is(SessionState::Running));
    QCOMPARE(engine_->stepRemainingMs(), qint64(13000));
    at(43000);
    QCOMPARE(times(EventType::Finished), TimeList() << 43000);

    // Exactly 5 s left: lead-in, then the remaining time continues.
    log_.clear();
    clock_.set(0);
    make(steps, 3);
    take(engine_->start());
    at(28000);
    take(engine_->pause());
    at(40000);
    take(engine_->resume());
    QVERIFY(is(SessionState::LeadIn));
    QCOMPARE(engine_->leadInRemainingMs(), qint64(3000));
    QCOMPARE(engine_->stepRemainingMs(), qint64(5000));
    at(43000);
    QVERIFY(is(SessionState::Running));
    at(48000);
    QCOMPARE(times(EventType::Finished), TimeList() << 48000);
    QCOMPARE(times(EventType::StepStarted), TimeList() << 3000 << 13000);

    // One millisecond more than 5 s left: no lead-in.
    log_.clear();
    clock_.set(0);
    make(steps, 3);
    take(engine_->start());
    clock_.set(27999);
    take(engine_->pause());
    clock_.set(40000);
    take(engine_->resume());
    QVERIFY(is(SessionState::Running));

    // Without a configured lead-in there is none for a rest either.
    log_.clear();
    clock_.set(0);
    make(steps, 0);
    take(engine_->start());
    at(28000);
    take(engine_->pause());
    take(engine_->resume());
    QVERIFY(is(SessionState::Running));
}

void TstEngine::pauseOverLongTime()
{
    make(QList<Step>() << work("a", 10), 0);
    take(engine_->start());
    at(1000);
    take(engine_->pause());
    const qint64 tenHours = qint64(10) * 3600 * 1000;
    at(tenHours);
    QVERIFY(is(SessionState::Paused));
    take(engine_->resume());
    at(tenHours + 9000);
    QCOMPARE(times(EventType::Finished), TimeList() << tenHours + 9000);
    QCOMPARE(engine_->records().at(0).activeMs, qint64(10000));
}

void TstEngine::pauseAndResumeOutOfContext()
{
    make(QList<Step>() << work("a", 10), 0);
    QVERIFY(engine_->pause().isEmpty());
    QVERIFY(engine_->resume().isEmpty());
    QVERIFY(is(SessionState::Ready));

    take(engine_->start());
    at(4000);
    QVERIFY(engine_->resume().isEmpty());
    QVERIFY(is(SessionState::Running));
    QCOMPARE(engine_->stepRemainingMs(), qint64(6000));

    take(engine_->pause());
    clock_.set(7000);
    QVERIFY(engine_->pause().isEmpty());
    QVERIFY(is(SessionState::Paused));
    QCOMPARE(engine_->stepRemainingMs(), qint64(6000));
}

void TstEngine::pauseDuringResumeLeadIn()
{
    make(QList<Step>() << work("a", 10), 3);
    take(engine_->start());
    at(5000);
    take(engine_->pause());
    at(6000);
    take(engine_->resume());
    at(7000);
    take(engine_->pause());
    QVERIFY(is(SessionState::Paused));
    QCOMPARE(engine_->stepRemainingMs(), qint64(8000));
    at(20000);
    take(engine_->resume());
    at(23000);
    QVERIFY(is(SessionState::Running));
    QCOMPARE(engine_->stepRemainingMs(), qint64(8000));
    QCOMPARE(times(EventType::StepStarted), TimeList() << 3000);
}

void TstEngine::nextStep()
{
    make(QList<Step>() << work("a", 10) << work("b", 10) << work("c", 10), 0);
    take(engine_->start());
    at(2000);
    take(engine_->next());
    QCOMPARE(engine_->stepIndex(), 1);
    QCOMPARE(engine_->stepRemainingMs(), qint64(10000));
    at(3000);
    take(engine_->next());
    QCOMPARE(engine_->stepIndex(), 2);
    at(4000);
    take(engine_->next());
    QVERIFY(is(SessionState::Finished));
    QCOMPARE(times(EventType::Finished), TimeList() << 4000);
    QCOMPARE(indices(EventType::StepStarted), QList<int>() << 0 << 1 << 2);

    QCOMPARE(engine_->records().at(0).activeMs, qint64(2000));
    QCOMPARE(engine_->records().at(1).activeMs, qint64(1000));
}

void TstEngine::nextThroughLeadIn()
{
    make(QList<Step>() << work("a", 10) << rest(5) << work("b", 10), 3);
    take(engine_->start());
    at(3000);
    at(5000);
    take(engine_->next());
    // The rest step has no lead-in.
    QVERIFY(is(SessionState::Running));
    QCOMPARE(engine_->stepIndex(), 1);
    take(engine_->next());
    QVERIFY(is(SessionState::LeadIn));
    QCOMPARE(engine_->stepIndex(), 2);
    QCOMPARE(engine_->leadInRemainingMs(), qint64(3000));
}

void TstEngine::nextWhilePaused()
{
    make(QList<Step>() << work("a", 10) << work("b", 10), 3);
    take(engine_->start());
    at(5000);
    take(engine_->pause());
    take(engine_->next());
    QVERIFY(is(SessionState::Paused));
    QCOMPARE(engine_->stepIndex(), 1);
    QCOMPARE(engine_->stepRemainingMs(), qint64(10000));
    at(9000);
    take(engine_->resume());
    at(12000);
    QVERIFY(is(SessionState::Running));
    QCOMPARE(times(EventType::StepStarted), TimeList() << 3000 << 12000);
    QCOMPARE(engine_->stepRemainingMs(), qint64(10000));
}

// Three blocks: a, b (two rounds with a rest between them) and c.
static QList<PlanStep> threeBlocks()
{
    QList<Block> blocks;
    blocks << block(QList<Step>() << work("a", 10), 1);
    blocks << block(QList<Step>() << work("b", 10) << work("c", 10), 2, 5);
    blocks << block(QList<Step>() << work("d", 10), 1);
    return expandPlan(workout(blocks));
}

void TstEngine::nextBlockSkipsTheRestOfTheBlock()
{
    EngineConfig config;
    config.leadInSec = 0;
    make(threeBlocks(), config);
    take(engine_->start());
    at(2000);
    // From the only step of block 0 to the first step of block 1.
    take(engine_->nextBlock());
    QCOMPARE(engine_->currentStep().blockIndex, 1);
    QCOMPARE(engine_->currentStep().roundIndex, 0);
    QCOMPARE(engine_->currentStep().stepIndexInBlock, 0);
    QVERIFY(is(SessionState::Running));
    QCOMPARE(engine_->stepRemainingMs(), qint64(10000));
    at(3000);
    // From inside the block, rounds and rest included, to block 2.
    take(engine_->nextBlock());
    QCOMPARE(engine_->currentStep().blockIndex, 2);
    QCOMPARE(engine_->currentStep().stepIndexInBlock, 0);
    QCOMPARE(engine_->records().at(1).activeMs, qint64(1000));
}

void TstEngine::nextBlockInTheLastBlockEnds()
{
    EngineConfig config;
    config.leadInSec = 0;
    make(threeBlocks(), config);
    take(engine_->start());
    take(engine_->nextBlock());
    take(engine_->nextBlock());
    QCOMPARE(engine_->currentStep().blockIndex, 2);
    at(4000);
    take(engine_->nextBlock());
    QVERIFY(is(SessionState::Finished));
    QCOMPARE(times(EventType::Finished), TimeList() << 4000);
    QCOMPARE(engine_->records().last().activeMs, qint64(4000));
}

void TstEngine::nextBlockInRestsLeadInAndPause()
{
    EngineConfig config;
    config.leadInSec = 3;
    make(threeBlocks(), config);
    take(engine_->start());
    // In the lead-in of the first step: the next block begins with its own lead-in.
    take(engine_->nextBlock());
    QVERIFY(is(SessionState::LeadIn));
    QCOMPARE(engine_->currentStep().blockIndex, 1);
    at(3000);
    at(13000);
    // Step b of round 0 is over; the rest between the rounds is running.
    at(23000);
    QVERIFY(engine_->currentStep().insertedRest || engine_->currentStep().roundIndex == 0);
    take(engine_->pause());
    take(engine_->nextBlock());
    // Paused: the next block is selected and the session stays paused until it is resumed.
    QVERIFY(is(SessionState::Paused));
    QCOMPARE(engine_->currentStep().blockIndex, 2);
    take(engine_->resume());
    at(30000);
    QVERIFY(is(SessionState::Running));
    QCOMPARE(engine_->currentStep().blockIndex, 2);
}

void TstEngine::nextBlockWhileWaitingForTap()
{
    QList<Step> second;
    Step waits = work("b", 10);
    waits.startOnTap = true;
    second << waits;
    QList<Block> blocks;
    blocks << block(QList<Step>() << work("a", 10), 1) << block(second, 1);
    EngineConfig config;
    config.leadInSec = 0;
    make(expandPlan(workout(blocks)), config);
    take(engine_->start());
    take(engine_->nextBlock());
    // The next block begins with a step that waits for the tap.
    QVERIFY(is(SessionState::WaitingToStart));
    QCOMPARE(engine_->currentStep().blockIndex, 1);
    take(engine_->nextBlock());
    QVERIFY(is(SessionState::Finished));
}

void TstEngine::elapsedTimeCountsActiveTimeOnly()
{
    make(QList<Step>() << work("a", 10) << work("b", 10), 3);
    QCOMPARE(engine_->elapsedActiveMs(), qint64(0));
    take(engine_->start());
    at(2000);
    // The lead-in does not count.
    QCOMPARE(engine_->elapsedActiveMs(), qint64(0));
    at(8000);
    QCOMPARE(engine_->elapsedActiveMs(), qint64(5000));
    take(engine_->pause());
    at(30000);
    QCOMPARE(engine_->elapsedActiveMs(), qint64(5000));
    take(engine_->resume());
    // The resume has its own lead-in; the step goes on at 33 s.
    at(33000);
    at(36000);
    QCOMPARE(engine_->elapsedActiveMs(), qint64(8000));
    // The time of the step left is added to that of the next one.
    take(engine_->next());
    at(37000);
    QCOMPARE(engine_->elapsedActiveMs(), qint64(8000));
}

void TstEngine::stopEndsSession()
{
    make(QList<Step>() << work("a", 10), 0);
    QVERIFY(engine_->stop().isEmpty());
    QVERIFY(is(SessionState::Ready));

    take(engine_->start());
    at(4000);
    take(engine_->stop());
    QVERIFY(is(SessionState::Stopped));
    QCOMPARE(times(EventType::Stopped), TimeList() << 4000);
    QCOMPARE(engine_->records().at(0).activeMs, qint64(4000));

    // Stop while paused.
    log_.clear();
    clock_.set(0);
    make(QList<Step>() << work("a", 10), 0);
    take(engine_->start());
    at(4000);
    take(engine_->pause());
    at(9000);
    take(engine_->stop());
    QVERIFY(is(SessionState::Stopped));
    QCOMPARE(times(EventType::Stopped), TimeList() << 9000);
    QCOMPARE(engine_->records().at(0).activeMs, qint64(4000));
}

void TstEngine::finalStateIgnoresCalls()
{
    make(QList<Step>() << work("a", 2), 0);
    take(engine_->start());
    at(2000);
    QVERIFY(is(SessionState::Finished));
    const int count = log_.size();
    QVERIFY(engine_->update().isEmpty());
    QVERIFY(engine_->start().isEmpty());
    QVERIFY(engine_->pause().isEmpty());
    QVERIFY(engine_->resume().isEmpty());
    QVERIFY(engine_->next().isEmpty());
    QVERIFY(engine_->nextBlock().isEmpty());
    QVERIFY(engine_->done().isEmpty());
    QVERIFY(engine_->stop().isEmpty());
    QVERIFY(is(SessionState::Finished));
    QCOMPARE(log_.size(), count);

    log_.clear();
    clock_.set(0);
    make(QList<Step>() << work("a", 20), 0);
    take(engine_->start());
    take(engine_->stop());
    QVERIFY(engine_->update().isEmpty());
    QVERIFY(engine_->start().isEmpty());
    QVERIFY(engine_->pause().isEmpty());
    QVERIFY(engine_->resume().isEmpty());
    QVERIFY(engine_->next().isEmpty());
    QVERIFY(engine_->nextBlock().isEmpty());
    QVERIFY(engine_->done().isEmpty());
    QVERIFY(engine_->stop().isEmpty());
    clock_.set(100000);
    QVERIFY(engine_->update().isEmpty());
    QVERIFY(is(SessionState::Stopped));
}

void TstEngine::doneOutsideWaitingStep()
{
    make(QList<Step>() << work("a", 10) << repStep("b", 5), 3);
    QVERIFY(engine_->done().isEmpty());
    take(engine_->start());
    QVERIFY(engine_->done().isEmpty());
    QVERIFY(is(SessionState::LeadIn));
    at(3000);
    QVERIFY(engine_->done().isEmpty());
    QVERIFY(is(SessionState::Running));
    QCOMPARE(engine_->stepIndex(), 0);
    // Lead-in of the repetition step.
    at(13000);
    QVERIFY(is(SessionState::LeadIn));
    QVERIFY(engine_->done().isEmpty());
    QCOMPARE(engine_->stepIndex(), 1);
}

void TstEngine::lateUpdateCrossesBoundaries()
{
    make(QList<Step>() << work("a", 2) << work("b", 2) << work("c", 2), 0);
    take(engine_->start());
    clock_.set(5500);
    const QList<EngineEvent> events = engine_->update();
    take(events);

    QCOMPARE(times(EventType::StepStarted), TimeList() << 0 << 2000 << 4000);
    QVERIFY(is(SessionState::Running));
    QCOMPARE(engine_->stepIndex(), 2);
    QCOMPARE(engine_->stepRemainingMs(), qint64(500));
    qint64 last = 0;
    for (const EngineEvent &e : events) {
        QVERIFY(e.atMs >= last);
        last = e.atMs;
    }
    // Ticks of the crossed steps are reported too: remaining 1 s of a, b and c.
    QCOMPARE(tickTimes(false), TimeList() << 1000 << 3000 << 5000);

    clock_.set(9000);
    take(engine_->update());
    QCOMPARE(times(EventType::Finished), TimeList() << 6000);
}

void TstEngine::lateUpdateAcrossLeadIns()
{
    make(QList<Step>() << work("a", 2) << work("b", 2), 3);
    take(engine_->start());
    clock_.set(20000);
    take(engine_->update());
    QCOMPARE(times(EventType::StepStarted), TimeList() << 3000 << 8000);
    QCOMPARE(times(EventType::Finished), TimeList() << 10000);
    QCOMPARE(tickTimes(true), TimeList() << 1000 << 2000 << 6000 << 7000);
}

void TstEngine::emptyPlanFinishes()
{
    make(QList<PlanStep>(), EngineConfig());
    take(engine_->start());
    QVERIFY(is(SessionState::Finished));
    QCOMPARE(times(EventType::Finished), TimeList() << 0);
}

void TstEngine::leadInIsClamped()
{
    EngineConfig config;
    config.leadInSec = 99;
    make(expandPlan(workout(QList<Block>() << block(QList<Step>() << work("a", 5)))), config);
    take(engine_->start());
    QCOMPARE(engine_->leadInRemainingMs(), qint64(10000));

    config.leadInSec = -4;
    make(expandPlan(workout(QList<Block>() << block(QList<Step>() << work("a", 5)))), config);
    log_.clear();
    take(engine_->start());
    QVERIFY(is(SessionState::Running));
}

void TstEngine::longWorkoutWithIrregularUpdates()
{
    // 50 rounds of lead-in 3 s + work 57 s + rest 12 s = 3600 s.
    const QList<Step> steps = QList<Step>() << work("a", 57) << rest(12);
    const QList<PlanStep> plan = expandPlan(workout(QList<Block>() << block(steps, 50)));
    QCOMPARE(plan.size(), 100);

    TimeList expected;
    qint64 cursor = 0;
    for (const PlanStep &step : plan) {
        if (step.type != StepType::Rest)
            cursor += 3000;
        expected.append(cursor);
        cursor += qint64(step.durationSec) * 1000;
    }
    QCOMPARE(cursor, qint64(3600) * 1000);

    make(plan, EngineConfig());
    std::mt19937 random(12345);
    std::uniform_int_distribution<int> gap(50, 700);

    take(engine_->start());
    qint64 now = 0;
    qint64 worstDelivery = 0;
    int calls = 0;
    while (!is(SessionState::Finished) && now < qint64(4000) * 1000) {
        now += gap(random);
        clock_.set(now);
        const QList<EngineEvent> events = engine_->update();
        ++calls;
        for (const EngineEvent &e : events)
            worstDelivery = qMax(worstDelivery, now - e.atMs);
        take(events);
    }

    QVERIFY(is(SessionState::Finished));
    QVERIFY(calls > 5000);
    QVERIFY(worstDelivery <= 700);
    const TimeList started = times(EventType::StepStarted);
    QCOMPARE(started.size(), expected.size());
    qint64 worstDeviation = 0;
    for (int i = 0; i < started.size(); ++i)
        worstDeviation = qMax(worstDeviation, qAbs(started.at(i) - expected.at(i)));
    QVERIFY(worstDeviation <= 100);
    QCOMPARE(times(EventType::Finished), TimeList() << qint64(3600) * 1000);

    qint64 active = 0;
    for (const StepRecord &record : engine_->records())
        active += record.activeMs;
    QCOMPARE(active, qint64(3600 - 50 * 3) * 1000);
}

QTEST_APPLESS_MAIN(TstEngine)

#include "tst_engine.moc"

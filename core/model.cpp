// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "model.h"

#include "workoutlimits.h"
#include "plan.h"

namespace {

bool outside(int value, int low, int high)
{
    return value < low || value > high;
}

void add(QList<Problem> &list, Problem::Code code, int blockIndex, int stepIndex)
{
    Problem problem;
    problem.code = code;
    problem.blockIndex = blockIndex;
    problem.stepIndex = stepIndex;
    list.append(problem);
}

}

QString stepTypeName(StepType type)
{
    switch (type) {
    case StepType::Work: return QStringLiteral("work");
    case StepType::Rest: return QStringLiteral("rest");
    case StepType::Warmup: return QStringLiteral("warmup");
    case StepType::Cooldown: return QStringLiteral("cooldown");
    }
    return QString();
}

bool stepTypeFromName(const QString &name, StepType *type)
{
    static const StepType all[] = { StepType::Work, StepType::Rest, StepType::Warmup, StepType::Cooldown };
    for (StepType candidate : all) {
        if (stepTypeName(candidate) == name) {
            *type = candidate;
            return true;
        }
    }
    return false;
}

Tempo Step::paceTempo() const
{
    if (pace == PaceKind::Total)
        return Tempo::repsAndTotal(reps, paceMs);
    if (pace == PaceKind::PerRep)
        return Tempo::timePerRep(paceMs, reps);
    return Tempo();
}

QString paceKindName(PaceKind kind)
{
    switch (kind) {
    case PaceKind::None: return QString();
    case PaceKind::Total: return QStringLiteral("total");
    case PaceKind::PerRep: return QStringLiteral("per-rep");
    }
    return QString();
}

bool paceKindFromName(const QString &name, PaceKind *kind)
{
    static const PaceKind all[] = { PaceKind::None, PaceKind::Total, PaceKind::PerRep };
    for (PaceKind candidate : all) {
        if (paceKindName(candidate) == name) {
            *kind = candidate;
            return true;
        }
    }
    return false;
}

QList<Problem> validateStep(const Step &step, int blockIndex, int stepIndex)
{
    QList<Problem> found;

    if (step.type == StepType::Rest) {
        if (!step.exerciseKey.isEmpty())
            add(found, Problem::RestHasExercise, blockIndex, stepIndex);
        if (step.reps != 0)
            add(found, Problem::RestNotTimed, blockIndex, stepIndex);
    } else if (step.exerciseKey.isEmpty()) {
        add(found, Problem::MissingExercise, blockIndex, stepIndex);
    }

    if (step.durationSec != 0 && step.reps != 0) {
        add(found, Problem::BothDurationAndReps, blockIndex, stepIndex);
        return found;
    }
    if (step.durationSec == 0 && step.reps == 0) {
        add(found, Problem::MissingDurationOrReps, blockIndex, stepIndex);
        return found;
    }
    if (step.durationSec != 0 && outside(step.durationSec, Limits::minDurationSec, Limits::maxDurationSec))
        add(found, Problem::DurationOutOfRange, blockIndex, stepIndex);
    if (step.reps != 0 && outside(step.reps, Limits::minReps, Limits::maxReps))
        add(found, Problem::RepsOutOfRange, blockIndex, stepIndex);

    if (step.pace != PaceKind::None) {
        if (step.durationSec != 0) {
            add(found, Problem::PaceWithDuration, blockIndex, stepIndex);
        } else if (step.reps != 0 && !outside(step.reps, Limits::minReps, Limits::maxReps)) {
            const TempoProblem pace = step.paceTempo().problem();
            if (pace == TempoProblem::RepTooShort)
                add(found, Problem::PaceRepTooShort, blockIndex, stepIndex);
            else if (pace == TempoProblem::RepTooLong)
                add(found, Problem::PaceRepTooLong, blockIndex, stepIndex);
        }
    }
    return found;
}

QList<Problem> validateStepFor(const Step &step, bool hasRhythm, int blockIndex, int stepIndex)
{
    if (!hasRhythm || step.type == StepType::Rest)
        return validateStep(step, blockIndex, stepIndex);
    // The stored pace is ignored while the rhythm exists: check the step without it.
    Step plain = step;
    plain.pace = PaceKind::None;
    plain.paceMs = 0;
    QList<Problem> found = validateStep(plain, blockIndex, stepIndex);
    if (step.durationSec != 0)
        add(found, Problem::DurationWithRhythm, blockIndex, stepIndex);
    return found;
}

QList<Problem> validate(const Workout &workout, const ExerciseTempos &rhythms)
{
    QList<Problem> found;

    if (workout.blocks.isEmpty())
        add(found, Problem::NoBlocks, -1, -1);
    if (workout.blocks.size() > Limits::maxBlocksPerWorkout)
        add(found, Problem::TooManyBlocks, -1, -1);
    if (outside(workout.restBetweenBlocksSec, Limits::minRestSec, Limits::maxRestSec))
        add(found, Problem::RestBetweenBlocksOutOfRange, -1, -1);

    for (int b = 0; b < workout.blocks.size(); ++b) {
        const Block &block = workout.blocks.at(b);
        if (block.steps.isEmpty())
            add(found, Problem::EmptyBlock, b, -1);
        if (block.steps.size() > Limits::maxStepsPerBlock)
            add(found, Problem::TooManySteps, b, -1);
        if (outside(block.rounds, Limits::minRounds, Limits::maxRounds))
            add(found, Problem::RoundsOutOfRange, b, -1);
        if (outside(block.restBetweenRoundsSec, Limits::minRestSec, Limits::maxRestSec))
            add(found, Problem::RestBetweenRoundsOutOfRange, b, -1);
        for (int s = 0; s < block.steps.size(); ++s) {
            const Step &step = block.steps.at(s);
            const bool ignoredPace = step.pace != PaceKind::None && rhythms.value(step.exerciseKey).isValid();
            if (ignoredPace) {
                Step plain = step;
                plain.pace = PaceKind::None;
                plain.paceMs = 0;
                found += validateStep(plain, b, s);
            } else {
                found += validateStep(step, b, s);
            }
        }
    }
    return found;
}

PlannedTotal totalPlannedSeconds(const Workout &workout, const ExerciseTempos &tempos)
{
    PlannedTotal total;
    const QList<PlanStep> plan = expandPlan(workout, tempos);
    qint64 allMs = 0;
    for (const PlanStep &step : plan) {
        if (step.isTimed()) {
            total.secondsTimedOnly += step.durationSec;
            allMs += qint64(step.durationSec) * 1000;
        } else if (step.hasTempo) {
            total.hasTempoSteps = true;
            allMs += step.plannedMs();
        } else {
            total.hasRepSteps = true;
        }
    }
    total.seconds = int((allMs + 500) / 1000);
    return total;
}

void normalizeStartModes(Workout &workout)
{
    for (Block &block : workout.blocks) {
        if (block.steps.isEmpty())
            continue;
        block.steps.first().startOnTap = false;
        return;
    }
}

Workout quickWorkout(const QString &name, const QString &exerciseKey, int workSec, int restSec, int rounds)
{
    Step work;
    work.type = StepType::Work;
    work.exerciseKey = exerciseKey;
    work.durationSec = workSec;

    Block block;
    block.steps.append(work);
    block.rounds = rounds;
    block.restBetweenRoundsSec = restSec;

    Workout workout;
    workout.name = name;
    workout.blocks.append(block);
    return workout;
}

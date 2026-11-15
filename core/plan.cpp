// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "plan.h"

namespace {

PlanStep insertedRest(const PlanStep &after, int seconds)
{
    PlanStep rest;
    rest.type = StepType::Rest;
    rest.durationSec = seconds;
    rest.blockIndex = after.blockIndex;
    rest.blockCount = after.blockCount;
    rest.roundIndex = after.roundIndex;
    rest.roundCount = after.roundCount;
    rest.stepIndexInBlock = -1;
    rest.stepCountInBlock = after.stepCountInBlock;
    rest.insertedRest = true;
    return rest;
}

}

QList<PlanStep> expandPlan(const Workout &workout, const ExerciseTempos &tempos)
{
    QList<PlanStep> plan;
    const int blockCount = workout.blocks.size();

    for (int b = 0; b < blockCount; ++b) {
        const Block &block = workout.blocks.at(b);
        if (block.steps.isEmpty())
            continue;

        if (!plan.isEmpty() && workout.restBetweenBlocksSec > 0)
            plan.append(insertedRest(plan.last(), workout.restBetweenBlocksSec));

        for (int r = 0; r < block.rounds; ++r) {
            if (r > 0 && block.restBetweenRoundsSec > 0)
                plan.append(insertedRest(plan.last(), block.restBetweenRoundsSec));

            for (int s = 0; s < block.steps.size(); ++s) {
                const Step &step = block.steps.at(s);
                PlanStep entry;
                entry.type = step.type;
                entry.exerciseKey = step.exerciseKey;
                entry.durationSec = step.durationSec;
                entry.reps = step.reps;
                entry.startOnTap = step.startOnTap;
                if (step.type != StepType::Rest && step.reps != 0) {
                    const Tempo rhythm = tempos.value(step.exerciseKey).forReps(step.reps);
                    if (rhythm.isValid()) {
                        entry.hasTempo = true;
                        entry.tempo = rhythm;
                    } else if (step.pace != PaceKind::None && step.durationSec == 0) {
                        const Tempo timed = step.paceTempo();
                        if (timed.isValid()) {
                            entry.hasTempo = true;
                            entry.tempo = timed;
                            entry.pace = step.pace;
                        }
                    }
                }
                entry.blockIndex = b;
                entry.blockCount = blockCount;
                entry.roundIndex = r;
                entry.roundCount = block.rounds;
                entry.stepIndexInBlock = s;
                entry.stepCountInBlock = block.steps.size();
                plan.append(entry);
            }
        }
    }
    return plan;
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "workoutdraft.h"

#include "workoutlimits.h"

#include <algorithm>
#include <cstdlib>

namespace {

bool hasBlock(const Workout &w, int block)
{
    return block >= 0 && block < w.blocks.size();
}

bool hasStep(const Workout &w, int block, int step)
{
    return hasBlock(w, block) && step >= 0 && step < w.blocks.at(block).steps.size();
}

WorkoutDraft::Position done(Workout &w, int block, int step)
{
    normalizeStartModes(w);
    WorkoutDraft::Position p;
    p.block = block;
    p.step = step;
    return p;
}

}

WorkoutDraft::Position WorkoutDraft::addStep(Workout &w, int block)
{
    if (!hasBlock(w, block))
        return Position();
    Step step;
    step.durationSec = defaultDurationSec;
    w.blocks[block].steps.append(step);
    return done(w, block, w.blocks.at(block).steps.size() - 1);
}

WorkoutDraft::Position WorkoutDraft::addBlock(Workout &w)
{
    w.blocks.append(Block());
    return done(w, w.blocks.size() - 1, -1);
}

bool WorkoutDraft::removeStep(Workout &w, int block, int step)
{
    if (!hasStep(w, block, step))
        return false;
    w.blocks[block].steps.removeAt(step);
    normalizeStartModes(w);
    return true;
}

bool WorkoutDraft::removeBlock(Workout &w, int block)
{
    if (!hasBlock(w, block))
        return false;
    w.blocks.removeAt(block);
    normalizeStartModes(w);
    return true;
}

WorkoutDraft::Position WorkoutDraft::duplicateStep(Workout &w, int block, int step)
{
    if (!hasStep(w, block, step))
        return Position();
    w.blocks[block].steps.insert(step + 1, w.blocks.at(block).steps.at(step));
    return done(w, block, step + 1);
}

WorkoutDraft::Position WorkoutDraft::duplicateBlock(Workout &w, int block)
{
    if (!hasBlock(w, block))
        return Position();
    w.blocks.insert(block + 1, w.blocks.at(block));
    return done(w, block + 1, -1);
}

WorkoutDraft::Position WorkoutDraft::moveStep(Workout &w, int block, int step, int delta)
{
    if (!hasStep(w, block, step))
        return Position();
    QList<Step> &steps = w.blocks[block].steps;
    const int target = std::max(0, std::min(int(steps.size()) - 1, step + delta));
    if (target != step)
        steps.move(step, target);
    return done(w, block, target);
}

WorkoutDraft::Position WorkoutDraft::moveBlock(Workout &w, int block, int delta)
{
    if (!hasBlock(w, block))
        return Position();
    const int target = std::max(0, std::min(int(w.blocks.size()) - 1, block + delta));
    if (target != block)
        w.blocks.move(block, target);
    return done(w, target, -1);
}

WorkoutDraft::Position WorkoutDraft::moveStepAcross(Workout &w, int block, int step, int delta)
{
    if (!hasStep(w, block, step))
        return Position();
    int b = block;
    int s = step;
    for (int left = std::abs(delta); left > 0; --left) {
        if (delta < 0) {
            if (s > 0) {
                w.blocks[b].steps.move(s, s - 1);
                --s;
            } else if (b > 0) {
                const Step moved = w.blocks[b].steps.takeAt(s);
                --b;
                w.blocks[b].steps.append(moved);
                s = w.blocks.at(b).steps.size() - 1;
            } else {
                break;
            }
        } else {
            if (s < w.blocks.at(b).steps.size() - 1) {
                w.blocks[b].steps.move(s, s + 1);
                ++s;
            } else if (b < w.blocks.size() - 1) {
                const Step moved = w.blocks[b].steps.takeAt(s);
                ++b;
                w.blocks[b].steps.prepend(moved);
                s = 0;
            } else {
                break;
            }
        }
    }
    return done(w, b, s);
}

WorkoutDraft::Position WorkoutDraft::moveStepToBlock(Workout &w, int block, int step, int targetBlock)
{
    if (!hasStep(w, block, step) || !hasBlock(w, targetBlock))
        return Position();
    if (targetBlock == block)
        return done(w, block, step);
    const Step moved = w.blocks[block].steps.takeAt(step);
    w.blocks[targetBlock].steps.append(moved);
    return done(w, targetBlock, w.blocks.at(targetBlock).steps.size() - 1);
}

namespace {

int clampPaceMs(PaceKind kind, int reps, int paceMs)
{
    if (kind == PaceKind::Total)
        return qBound(reps * Limits::minRepMs, paceMs, reps * Limits::maxRepMs);
    return qBound(Limits::minRepMs, paceMs, Limits::maxRepMs);
}

}

void WorkoutDraft::setStepType(Step &step, StepType type)
{
    step.type = type;
    if (type != StepType::Rest)
        return;
    step.exerciseKey.clear();
    step.pace = PaceKind::None;
    step.paceMs = 0;
    if (!step.isTimed()) {
        step.reps = 0;
        step.durationSec = defaultDurationSec;
    }
}

void WorkoutDraft::setStepTimed(Step &step, bool timed)
{
    if (timed == step.isTimed() && (timed || step.reps > 0))
        return;
    step.pace = PaceKind::None;
    step.paceMs = 0;
    if (timed || step.type == StepType::Rest) {
        step.durationSec = defaultDurationSec;
        step.reps = 0;
    } else {
        step.durationSec = 0;
        step.reps = defaultReps;
    }
}

void WorkoutDraft::setStepReps(Step &step, int reps)
{
    step.reps = reps;
    step.durationSec = 0;
    if (step.pace == PaceKind::Total && reps >= Limits::minReps && reps <= Limits::maxReps)
        step.paceMs = clampPaceMs(PaceKind::Total, reps, step.paceMs);
}

void WorkoutDraft::setStepDuration(Step &step, int seconds)
{
    step.durationSec = seconds;
    step.reps = 0;
    step.pace = PaceKind::None;
    step.paceMs = 0;
}

void WorkoutDraft::setStepPace(Step &step, PaceKind kind, int paceMs)
{
    if (kind == PaceKind::None || step.isTimed() || step.reps < Limits::minReps || step.reps > Limits::maxReps) {
        step.pace = PaceKind::None;
        step.paceMs = 0;
        return;
    }
    if (paceMs <= 0)
        paceMs = kind == PaceKind::Total ? step.reps * defaultRepMs : defaultRepMs;
    step.pace = kind;
    step.paceMs = clampPaceMs(kind, step.reps, paceMs);
}

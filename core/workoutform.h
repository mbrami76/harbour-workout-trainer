// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef WORKOUTFORM_H
#define WORKOUTFORM_H

#include <QList>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "model.h"

// The parts of a workout as maps, the form the screens use, and the codes of what the core refuses
// (R-WRK-8, R-WRK-12). Maps are read only: the draft of a workout is edited with WorkoutDraft.
namespace WorkoutForm {

// {type, exerciseKey, durationSec, reps, startOnTap, timed, pace, paceMs}; type is the stored name
// of the step type ("work", "rest", "warmup", "cooldown"); pace is "none", "total" or "per-rep".
QVariantMap stepToMap(const Step &step);
// {rounds, restBetweenRoundsSec, stepCount}
QVariantMap blockToMap(const Block &block);
// {id, favorite, name, icon, colour, restBetweenBlocksSec, blockCount, stepCount}
QVariantMap headerToMap(const Workout &workout);

int stepCount(const Workout &workout);

// The reason a workout is refused, as a fixed code: "no-blocks", "too-many-blocks",
// "rest-between-blocks", "empty-block", "too-many-steps", "rounds", "rest-between-rounds",
// "exercise-missing", "rest-has-exercise", "rest-not-timed", "ending-missing", "ending-both",
// "duration", "reps", "name-empty", "name-too-long", "icon", "colour", "identity",
// "exercise-unknown", "pace-with-duration", "pace-short", "pace-long" or "duration-with-rhythm".
QString problemCode(Problem::Code code);
// [{code, block, step}] in order; block and step are -1 when the finding is not about them.
QVariantList problemList(const QList<Problem> &problems);

// A step of the definition by place.
struct StepRef {
    int block = 0;
    int step = 0;
};

// The steps that end by repetitions, are not rests, whose exercise has no rhythm in the snapshot
// and that have no valid timed pace: the ones the total planned time leaves out (R-WRK-9). Each step counts once, however
// many rounds its block has.
QList<StepRef> uncountedSteps(const Workout &workout, const ExerciseTempos &tempos);

}

#endif

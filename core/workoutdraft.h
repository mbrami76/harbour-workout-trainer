// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef WORKOUTDRAFT_H
#define WORKOUTDRAFT_H

#include "model.h"

// Edits of a workout that is being built (R-WRK-2 to R-WRK-6, R-WRK-11). Every function works on
// the workout in place, never refuses a value for a limit (the validation of the core reports
// those, R-WRK-8) and leaves the start mode of the first step of the first block cleared. An index
// that does not exist changes nothing and returns false or an invalid position.
namespace WorkoutDraft {

// Length and count of a step that is added or whose ending is switched.
const int defaultDurationSec = 30;
const int defaultReps = 10;
// The time of one repetition that a timed pace starts with (R-WRK-14).
const int defaultRepMs = 2000;

// A step by block and place in the block; step is -1 for a block. Valid positions are not negative.
struct Position {
    int block = -1;
    int step = -1;
    bool isValid() const { return block >= 0; }
};

// Appends a work step with no exercise and the default duration to the block; its position.
Position addStep(Workout &workout, int block);
// Appends an empty block of one round and no rest; its position.
Position addBlock(Workout &workout);
bool removeStep(Workout &workout, int block, int step);
bool removeBlock(Workout &workout, int block);
// A copy right after the original; the position of the copy.
Position duplicateStep(Workout &workout, int block, int step);
Position duplicateBlock(Workout &workout, int block);
// Moves by delta places inside the block or the workout, stopping at the ends; the new position.
// The position is the old one when nothing could move.
Position moveStep(Workout &workout, int block, int step, int delta);
Position moveBlock(Workout &workout, int block, int delta);
// Moves a step one place at a time, `delta` places in all (up for a negative, down for a positive
// number). At the first place of a block an up move takes the step to the end of the block before,
// and at the last place a down move takes it to the start of the block after (R-WRK-6); it stops
// at the first place of the first block and the last place of the last block. The new position;
// the old one when nothing could move; invalid for a step that does not exist. A block that
// ends up empty stays, and is reported by the validation (R-WRK-8).
Position moveStepAcross(Workout &workout, int block, int step, int delta);
// Moves the step to the end of another block, as the same step; the new position.
Position moveStepToBlock(Workout &workout, int block, int step, int targetBlock);

// A rest step has no exercise and ends by duration (R-WRK-3): the exercise is cleared and a
// repetition ending becomes the default duration. The other types keep what they have.
void setStepType(Step &step, StepType type);
// Switches between the two endings (R-WRK-4); the old value is dropped and the default of the new
// one is set. A rest stays timed. Does nothing if the step already ends that way.
void setStepTimed(Step &step, bool timed);
// Sets the repetitions of the step, which then ends by repetitions (the duration and any pace
// of a duration step are gone). A pace in a total time keeps its mean repetition inside the limits
// of 0.5 s to 60 s, so that the step is never left invalid by this change alone (R-WRK-14).
void setStepReps(Step &step, int reps);
// Sets the duration of the step, which then ends by duration and has no pace.
void setStepDuration(Step &step, int seconds);
// Sets the pace of a repetition step (R-WRK-14). A value of 0 takes the default of the kind: the
// time of one repetition defaultRepMs, or the repetitions times that for a total time, inside
// the limits. A value outside the limits is clamped, so that the step stays valid; the kind None
// clears the pace. A step that ends by duration gets none.
void setStepPace(Step &step, PaceKind kind, int paceMs = 0);

}

#endif

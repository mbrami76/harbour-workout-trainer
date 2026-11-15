// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef PLAN_H
#define PLAN_H

#include <QHash>

#include "model.h"

// One step of the flat plan. All indices are zero-based. An inserted rest (between rounds or
// between blocks) belongs to the block and round that precede it and has stepIndexInBlock -1.
struct PlanStep {
    StepType type = StepType::Work;
    QString exerciseKey;
    int durationSec = 0;
    int reps = 0;
    int blockIndex = 0;
    int blockCount = 0;
    int roundIndex = 0;
    int roundCount = 0;
    int stepIndexInBlock = 0;
    int stepCountInBlock = 0;
    bool insertedRest = false;
    bool startOnTap = false;
    // A paced repetition step (R-WRK-10, R-WRK-14): tempo is the pace of the repetitions, taken
    // from the rhythm of the exercise when the plan was built or, without a rhythm, from the pace
    // of the step; it covers the step's own repetitions (tempo.reps() == reps). pace tells which:
    // None for a rhythm (the phases are in tempo) and for a step that is not paced by time.
    bool hasTempo = false;
    Tempo tempo;
    PaceKind pace = PaceKind::None;

    bool isTimed() const { return durationSec > 0; }
    // Planned length in milliseconds: the duration of a timed step, the repetitions times the
    // length of one repetition for a tempo step, 0 for a repetition step without a tempo (it ends
    // when the user says so).
    qint64 plannedMs() const
    {
        return isTimed() ? qint64(durationSec) * 1000 : (hasTempo ? tempo.setDurationMs() : 0);
    }
};

// Expands the blocks by their rounds and inserts the rests (R-WRK-2, R-WRK-5). A rest of 0
// inserts nothing; no rest follows the last round or the last block.
//
// The plan is pure: the tempos of the exercises are passed in, by exercise key, as a snapshot
// taken by the caller (see WorkoutStore::plan()). A step that ends by repetitions and is not a
// rest takes the rhythm of its exercise and keeps its own repetitions (R-WRK-10); without a
// rhythm it takes the timed pace stored in the step, if any and valid (R-WRK-14). A rhythm wins
// over a stored pace. A step that ends by duration, a rest and a repetition step with neither a
// rhythm nor a valid pace stay plain.
QList<PlanStep> expandPlan(const Workout &workout, const ExerciseTempos &tempos = ExerciseTempos());

#endif

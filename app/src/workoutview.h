// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef WORKOUTVIEW_H
#define WORKOUTVIEW_H

#include <QHash>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "model.h"

class Services;

// What the screens show about the steps of a workout (R-WRK-9, R-WRK-10, R-EXR-10): the names of
// the exercises and the tempo each one gets from its metronome, read from the stores. The workout
// itself stays the one of the core; nothing here decides a rule.
namespace WorkoutView {

struct Lookups {
    QHash<QString, QString> exerciseNames;
    // The metronome name and the length of one repetition, by exercise key, for the exercises that
    // have a tempo.
    QHash<QString, QString> tempoNames;
    QHash<QString, qint64> tempoRepMs;
    ExerciseTempos tempos;

    QString exerciseName(const QString &key) const { return exerciseNames.value(key); }
    bool hasTempo(const QString &key) const { return tempos.contains(key); }
};

// Reads the exercises, the metronomes and the tempos from the stores now.
Lookups load(Services *services);

struct RowOptions {
    // Rows to add a step to each block, for the builder.
    bool addRows = false;
    // Rows for the rest between blocks, for the read-only view.
    bool blockRests = false;
    // The blocks shown as one line, by position: their steps and their add row are left out. A
    // view only, never stored (R-WRK-6).
    QList<bool> collapsed;
};

// The rows of a list of blocks and steps, flat, for a ListView:
//   {kind: "block", block, rounds, restBetweenRoundsSec, stepCount, collapsed, plannedSeconds,
//    plannedOpen (a step of the block ends on a tap and is not counted), problems}
//   {kind: "step", block, step, type, exerciseKey, exerciseName, timed, durationSec, reps,
//    startOnTap, startModeAllowed, canMoveUp, canMoveDown, tempoName, tempoRepMs, notCounted, problems}
//   {kind: "addstep", block}                       (RowOptions::addRows)
//   {kind: "blockrest", sec}                       (RowOptions::blockRests)
// problems is the list of codes that are about the block or the step (WorkoutForm::problemList()).
QVariantList rows(const Workout &workout, const Lookups &lookups, const QVariantList &problems,
                  const RowOptions &options);

// One step as its row.
QVariantMap stepRow(const Workout &workout, int block, int step, const Lookups &lookups,
                    const QVariantList &problems);

// The planned time: {seconds, hasTempoSteps, uncounted (number of steps left out), uncountedNames
// (the exercises of those steps, sorted, without repeats)}.
QVariantMap total(const Workout &workout, const Lookups &lookups);

}

#endif

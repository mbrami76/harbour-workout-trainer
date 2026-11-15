// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef TESTSUPPORT_H
#define TESTSUPPORT_H

#include <QHash>
#include <initializer_list>

#include "model.h"

// A tempo of the form with phases, from the lengths in half seconds.
inline Tempo phasesOf(std::initializer_list<int> units, int reps)
{
    return Tempo::phases(QList<int>(units), reps);
}

inline Step timedStep(StepType type, const QString &exercise, int seconds)
{
    Step step;
    step.type = type;
    step.exerciseKey = exercise;
    step.durationSec = seconds;
    return step;
}

inline Step repStep(const QString &exercise, int reps)
{
    Step step;
    step.type = StepType::Work;
    step.exerciseKey = exercise;
    step.reps = reps;
    return step;
}

// The tempos of the exercises that tempoStep() has used, by exercise key: the snapshot that the
// plan, the planned time and the engine take in tests that use no database. A later call for the
// same exercise replaces the tempo.
inline ExerciseTempos &tempoSnapshot()
{
    static ExerciseTempos snapshot;
    return snapshot;
}

// A repetition step of an exercise whose metronome has the given pace. The step has its own
// repetitions: reps, or those of the tempo when reps is 0.
inline Step tempoStep(const QString &exercise, const Tempo &tempo, int reps = 0)
{
    tempoSnapshot().insert(exercise, tempo);
    return repStep(exercise, reps > 0 ? reps : tempo.reps());
}

// A repetition step with its own pace (no rhythm): the repetitions in a total time, or a time per
// repetition, in milliseconds.
inline Step pacedStep(const QString &exercise, int reps, PaceKind pace, int paceMs)
{
    Step step = repStep(exercise, reps);
    step.pace = pace;
    step.paceMs = paceMs;
    return step;
}

inline Step onTap(Step step)
{
    step.startOnTap = true;
    return step;
}

inline Step work(const QString &exercise, int seconds)
{
    return timedStep(StepType::Work, exercise, seconds);
}

inline Step rest(int seconds)
{
    return timedStep(StepType::Rest, QString(), seconds);
}

inline Block block(const QList<Step> &steps, int rounds = 1, int restBetweenRoundsSec = 0)
{
    Block b;
    b.steps = steps;
    b.rounds = rounds;
    b.restBetweenRoundsSec = restBetweenRoundsSec;
    return b;
}

inline Workout workout(const QList<Block> &blocks, int restBetweenBlocksSec = 0)
{
    Workout w;
    w.name = QStringLiteral("Test workout");
    w.blocks = blocks;
    w.restBetweenBlocksSec = restBetweenBlocksSec;
    return w;
}

inline bool hasProblem(const QList<Problem> &list, Problem::Code code)
{
    for (const Problem &p : list) {
        if (p.code == code)
            return true;
    }
    return false;
}

#endif

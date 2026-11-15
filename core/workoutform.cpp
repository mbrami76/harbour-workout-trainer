// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "workoutform.h"

QVariantMap WorkoutForm::stepToMap(const Step &s)
{
    QVariantMap map;
    map.insert(QStringLiteral("type"), stepTypeName(s.type));
    map.insert(QStringLiteral("exerciseKey"), s.exerciseKey);
    map.insert(QStringLiteral("durationSec"), s.durationSec);
    map.insert(QStringLiteral("reps"), s.reps);
    map.insert(QStringLiteral("startOnTap"), s.startOnTap);
    map.insert(QStringLiteral("timed"), s.isTimed());
    // "none", "total" or "per-rep", and its value in milliseconds (0 for none).
    map.insert(QStringLiteral("pace"), s.pace == PaceKind::None ? QStringLiteral("none") : paceKindName(s.pace));
    map.insert(QStringLiteral("paceMs"), s.pace == PaceKind::None ? 0 : s.paceMs);
    return map;
}

QVariantMap WorkoutForm::blockToMap(const Block &b)
{
    QVariantMap map;
    map.insert(QStringLiteral("rounds"), b.rounds);
    map.insert(QStringLiteral("restBetweenRoundsSec"), b.restBetweenRoundsSec);
    map.insert(QStringLiteral("stepCount"), b.steps.size());
    return map;
}

int WorkoutForm::stepCount(const Workout &w)
{
    int count = 0;
    for (const Block &b : w.blocks)
        count += b.steps.size();
    return count;
}

QVariantMap WorkoutForm::headerToMap(const Workout &w)
{
    QVariantMap map;
    map.insert(QStringLiteral("id"), w.id);
    map.insert(QStringLiteral("favorite"), w.favorite);
    map.insert(QStringLiteral("name"), w.name);
    map.insert(QStringLiteral("icon"), w.icon);
    map.insert(QStringLiteral("colour"), w.colour);
    map.insert(QStringLiteral("restBetweenBlocksSec"), w.restBetweenBlocksSec);
    map.insert(QStringLiteral("blockCount"), w.blocks.size());
    map.insert(QStringLiteral("stepCount"), stepCount(w));
    return map;
}

QString WorkoutForm::problemCode(Problem::Code code)
{
    switch (code) {
    case Problem::NoBlocks: return QStringLiteral("no-blocks");
    case Problem::TooManyBlocks: return QStringLiteral("too-many-blocks");
    case Problem::RestBetweenBlocksOutOfRange: return QStringLiteral("rest-between-blocks");
    case Problem::EmptyBlock: return QStringLiteral("empty-block");
    case Problem::TooManySteps: return QStringLiteral("too-many-steps");
    case Problem::RoundsOutOfRange: return QStringLiteral("rounds");
    case Problem::RestBetweenRoundsOutOfRange: return QStringLiteral("rest-between-rounds");
    case Problem::MissingExercise: return QStringLiteral("exercise-missing");
    case Problem::RestHasExercise: return QStringLiteral("rest-has-exercise");
    case Problem::RestNotTimed: return QStringLiteral("rest-not-timed");
    case Problem::MissingDurationOrReps: return QStringLiteral("ending-missing");
    case Problem::BothDurationAndReps: return QStringLiteral("ending-both");
    case Problem::DurationOutOfRange: return QStringLiteral("duration");
    case Problem::RepsOutOfRange: return QStringLiteral("reps");
    case Problem::NameEmpty: return QStringLiteral("name-empty");
    case Problem::NameTooLong: return QStringLiteral("name-too-long");
    case Problem::IconInvalid: return QStringLiteral("icon");
    case Problem::ColourUnknown: return QStringLiteral("colour");
    case Problem::IdentityTaken: return QStringLiteral("identity");
    case Problem::UnknownExercise: return QStringLiteral("exercise-unknown");
    case Problem::PaceWithDuration: return QStringLiteral("pace-with-duration");
    case Problem::PaceRepTooShort: return QStringLiteral("pace-short");
    case Problem::PaceRepTooLong: return QStringLiteral("pace-long");
    case Problem::DurationWithRhythm: return QStringLiteral("duration-with-rhythm");
    }
    return QStringLiteral("failed");
}

QVariantList WorkoutForm::problemList(const QList<Problem> &problems)
{
    QVariantList list;
    for (const Problem &p : problems) {
        QVariantMap map;
        map.insert(QStringLiteral("code"), problemCode(p.code));
        map.insert(QStringLiteral("block"), p.blockIndex);
        map.insert(QStringLiteral("step"), p.stepIndex);
        list.append(map);
    }
    return list;
}

QList<WorkoutForm::StepRef> WorkoutForm::uncountedSteps(const Workout &w, const ExerciseTempos &tempos)
{
    QList<StepRef> found;
    for (int b = 0; b < w.blocks.size(); ++b) {
        const QList<Step> &steps = w.blocks.at(b).steps;
        for (int s = 0; s < steps.size(); ++s) {
            const Step &step = steps.at(s);
            if (step.isTimed() || step.type == StepType::Rest || tempos.contains(step.exerciseKey)
                || step.paceTempo().isValid())
                continue;
            StepRef ref;
            ref.block = b;
            ref.step = s;
            found.append(ref);
        }
    }
    return found;
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "workoutview.h"

#include <QSet>

#include "services.h"
#include "workoutform.h"

namespace {

QStringList codesFor(const QVariantList &problems, int block, int step)
{
    QStringList codes;
    for (const QVariant &v : problems) {
        const QVariantMap p = v.toMap();
        if (p.value(QStringLiteral("block")).toInt() == block && p.value(QStringLiteral("step")).toInt() == step)
            codes.append(p.value(QStringLiteral("code")).toString());
    }
    return codes;
}

}

WorkoutView::Lookups WorkoutView::load(Services *services)
{
    Lookups lookups;
    if (!services->isReady())
        return lookups;
    lookups.tempos = services->workouts()->tempos();
    QHash<int, MetronomeSummary> metronomes;
    for (const MetronomeSummary &m : services->metronomes()->list())
        metronomes.insert(m.id, m);
    for (const Exercise &e : services->exercises()->list()) {
        lookups.exerciseNames.insert(e.key, e.name);
        const auto found = metronomes.constFind(e.metronomeId);
        if (found != metronomes.constEnd() && lookups.tempos.contains(e.key)) {
            lookups.tempoNames.insert(e.key, found->name);
            lookups.tempoRepMs.insert(e.key, found->repetitionMs);
        }
    }
    return lookups;
}

QVariantMap WorkoutView::stepRow(const Workout &w, int block, int step, const Lookups &lookups,
                                 const QVariantList &problems)
{
    const Step &s = w.blocks.at(block).steps.at(step);
    QVariantMap map = WorkoutForm::stepToMap(s);
    map.insert(QStringLiteral("kind"), QStringLiteral("step"));
    map.insert(QStringLiteral("block"), block);
    map.insert(QStringLiteral("step"), step);
    map.insert(QStringLiteral("exerciseName"), lookups.exerciseName(s.exerciseKey));
    map.insert(QStringLiteral("startModeAllowed"), !(block == 0 && step == 0));
    // Whether the step has a place before or after it, across the blocks (R-WRK-6).
    map.insert(QStringLiteral("canMoveUp"), !(block == 0 && step == 0));
    map.insert(QStringLiteral("canMoveDown"),
               !(block == w.blocks.size() - 1 && step == w.blocks.at(block).steps.size() - 1));
    const bool repetitions = !s.isTimed() && s.type != StepType::Rest;
    // A rhythm wins over a stored pace, which is then kept but not in use (R-WRK-14).
    const bool rhythm = repetitions && lookups.hasTempo(s.exerciseKey);
    const bool timedPace = repetitions && !rhythm && s.paceTempo().isValid();
    map.insert(QStringLiteral("tempoName"), rhythm ? lookups.tempoNames.value(s.exerciseKey) : QString());
    map.insert(QStringLiteral("tempoRepMs"), rhythm ? lookups.tempoRepMs.value(s.exerciseKey) : qint64(0));
    map.insert(QStringLiteral("hasRhythm"), lookups.hasTempo(s.exerciseKey) && s.type != StepType::Rest);
    map.insert(QStringLiteral("rhythmUsed"), rhythm);
    map.insert(QStringLiteral("paceUsed"), timedPace);
    map.insert(QStringLiteral("paceIgnored"), rhythm && s.pace != PaceKind::None);
    // The time of one repetition when the step is paced, else 0, and the time the step takes.
    qint64 repMs = 0;
    qint64 plannedMs = 0;
    if (rhythm) {
        repMs = lookups.tempoRepMs.value(s.exerciseKey);
        plannedMs = repMs * s.reps;
    } else if (timedPace) {
        const Tempo pace = s.paceTempo();
        repMs = (pace.setDurationMs() + s.reps / 2) / s.reps;
        plannedMs = pace.setDurationMs();
    } else if (s.isTimed()) {
        plannedMs = qint64(s.durationSec) * 1000;
    }
    map.insert(QStringLiteral("repMs"), repMs);
    map.insert(QStringLiteral("plannedMs"), plannedMs);
    map.insert(QStringLiteral("notCounted"), repetitions && s.reps > 0 && !rhythm && !timedPace);
    map.insert(QStringLiteral("problems"), codesFor(problems, block, step));
    return map;
}

QVariantList WorkoutView::rows(const Workout &w, const Lookups &lookups, const QVariantList &problems,
                               const RowOptions &options)
{
    QVariantList list;
    for (int b = 0; b < w.blocks.size(); ++b) {
        QVariantMap block = WorkoutForm::blockToMap(w.blocks.at(b));
        block.insert(QStringLiteral("kind"), QStringLiteral("block"));
        block.insert(QStringLiteral("block"), b);
        block.insert(QStringLiteral("problems"), codesFor(problems, b, -1));
        const bool collapsed = b < options.collapsed.size() && options.collapsed.at(b);
        block.insert(QStringLiteral("collapsed"), collapsed);
        // The planned time of the block alone: its rounds and the rests between them.
        Workout alone;
        alone.blocks.append(w.blocks.at(b));
        const PlannedTotal planned = totalPlannedSeconds(alone, lookups.tempos);
        block.insert(QStringLiteral("plannedSeconds"), planned.seconds);
        block.insert(QStringLiteral("plannedOpen"), planned.hasRepSteps);
        list.append(block);
        if (collapsed)
            continue;
        for (int s = 0; s < w.blocks.at(b).steps.size(); ++s)
            list.append(stepRow(w, b, s, lookups, problems));
        if (options.addRows) {
            QVariantMap add;
            add.insert(QStringLiteral("kind"), QStringLiteral("addstep"));
            add.insert(QStringLiteral("block"), b);
            list.append(add);
        }
        if (options.blockRests && b < w.blocks.size() - 1 && w.restBetweenBlocksSec > 0) {
            QVariantMap rest;
            rest.insert(QStringLiteral("kind"), QStringLiteral("blockrest"));
            rest.insert(QStringLiteral("sec"), w.restBetweenBlocksSec);
            list.append(rest);
        }
    }
    return list;
}

QVariantMap WorkoutView::total(const Workout &w, const Lookups &lookups)
{
    const PlannedTotal planned = totalPlannedSeconds(w, lookups.tempos);
    const QList<WorkoutForm::StepRef> left = WorkoutForm::uncountedSteps(w, lookups.tempos);
    QSet<QString> names;
    for (const WorkoutForm::StepRef &ref : left) {
        const QString key = w.blocks.at(ref.block).steps.at(ref.step).exerciseKey;
        const QString name = lookups.exerciseName(key);
        if (!name.isEmpty())
            names.insert(name);
    }
    QStringList sorted = names.toList();
    sorted.sort(Qt::CaseInsensitive);
    QVariantMap map;
    map.insert(QStringLiteral("seconds"), planned.seconds);
    map.insert(QStringLiteral("hasTempoSteps"), planned.hasTempoSteps);
    map.insert(QStringLiteral("uncounted"), left.size());
    map.insert(QStringLiteral("uncountedNames"), sorted);
    return map;
}

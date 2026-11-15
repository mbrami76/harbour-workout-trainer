// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "exerciseform.h"

#include "musclegroups.h"

QVariantMap ExerciseForm::toMap(const Exercise &e)
{
    QVariantMap map;
    map.insert(QStringLiteral("key"), e.key);
    map.insert(QStringLiteral("name"), e.name);
    map.insert(QStringLiteral("muscles"), MuscleKeys::keysOf(e.muscles));
    map.insert(QStringLiteral("instructions"), e.instructions);
    map.insert(QStringLiteral("tips"), e.tips);
    map.insert(QStringLiteral("icon"), e.icon);
    map.insert(QStringLiteral("colour"), e.colour);
    map.insert(QStringLiteral("language"), e.language);
    map.insert(QStringLiteral("starter"), e.starter);
    map.insert(QStringLiteral("metronomeId"), e.metronomeId);
    return map;
}

Exercise ExerciseForm::fromMap(const QVariantMap &map)
{
    Exercise e;
    e.key = map.value(QStringLiteral("key")).toString();
    e.name = map.value(QStringLiteral("name")).toString().trimmed();
    for (const QString &key : map.value(QStringLiteral("muscles")).toStringList()) {
        for (const QString &replacement : MuscleKeys::replacementKeys(key)) {
            MuscleGroup group;
            if (MuscleKeys::fromKey(replacement, &group))
                e.muscles |= muscleBit(group);
        }
    }
    e.instructions = map.value(QStringLiteral("instructions")).toString();
    e.tips = map.value(QStringLiteral("tips")).toString();
    e.icon = map.value(QStringLiteral("icon")).toString();
    e.colour = map.value(QStringLiteral("colour")).toString();
    if (map.contains(QStringLiteral("language")))
        e.language = map.value(QStringLiteral("language")).toString();
    e.starter = map.value(QStringLiteral("starter")).toBool();
    e.metronomeId = map.value(QStringLiteral("metronomeId")).toInt();
    return e;
}

QString ExerciseForm::problemCode(ExerciseProblem problem)
{
    switch (problem) {
    case ExerciseProblem::NameEmpty: return QStringLiteral("name-empty");
    case ExerciseProblem::NameTooLong: return QStringLiteral("name-too-long");
    case ExerciseProblem::InstructionsTooLong: return QStringLiteral("instructions-too-long");
    case ExerciseProblem::TipsTooLong: return QStringLiteral("tips-too-long");
    case ExerciseProblem::NoMuscles: return QStringLiteral("muscles-none");
    case ExerciseProblem::UnknownMuscles: return QStringLiteral("muscles-unknown");
    case ExerciseProblem::IconInvalid: return QStringLiteral("icon");
    case ExerciseProblem::ColourUnknown: return QStringLiteral("colour");
    case ExerciseProblem::LanguageInvalid: return QStringLiteral("language");
    }
    return QStringLiteral("failed");
}

QStringList ExerciseForm::problemCodes(const QList<ExerciseProblem> &problems)
{
    QStringList codes;
    for (ExerciseProblem problem : problems) {
        const QString code = problemCode(problem);
        if (!codes.contains(code))
            codes.append(code);
    }
    return codes;
}

bool ExerciseForm::matches(const Exercise &exercise, const QString &text, const QString &groupKey)
{
    const QString needle = text.trimmed();
    if (!needle.isEmpty() && !exercise.name.contains(needle, Qt::CaseInsensitive))
        return false;
    if (groupKey.isEmpty())
        return true;
    MuscleGroup group;
    if (!MuscleKeys::fromKey(groupKey, &group))
        return false;
    return (exercise.muscles & muscleBit(group)) != 0;
}

QList<Exercise> ExerciseForm::filter(const QList<Exercise> &exercises, const QString &text, const QString &groupKey)
{
    QList<Exercise> result;
    for (const Exercise &e : exercises) {
        if (matches(e, text, groupKey))
            result.append(e);
    }
    return result;
}

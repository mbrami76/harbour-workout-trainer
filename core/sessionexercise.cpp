// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "sessionexercise.h"

QString SessionExercise::fileKey(const PlanStep *current, const PlanStep *next)
{
    if (!current)
        return QString();
    if (current->type != StepType::Rest)
        return current->exerciseKey;
    return next && next->type != StepType::Rest ? next->exerciseKey : QString();
}

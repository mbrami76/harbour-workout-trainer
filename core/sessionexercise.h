// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef SESSIONEXERCISE_H
#define SESSIONEXERCISE_H

#include <QString>

#include "plan.h"

// Which exercise file a session offers (R-TMR-12). Pure logic.
namespace SessionExercise {

// The key of the exercise whose file the button of the session opens, given the step the session is
// at and the one after it (null for none). During a step with an exercise (and during the lead-in
// before the first one) it is the exercise of that step; during a rest it is the exercise of the
// next step. Empty when there is none: a step without an exercise, a rest that is not followed by
// an exercise, the end of the plan.
QString fileKey(const PlanStep *current, const PlanStep *next);

}

#endif

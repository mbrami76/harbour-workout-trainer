// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef EXERCISEFORM_H
#define EXERCISEFORM_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include "exercisestore.h"

// An exercise as a map, the form the screens use, and the conversions to and from the model
// (R-EXR-1). Keys of the map: key, name, muscles (a list of group keys), instructions, tips, icon,
// colour, language, starter, metronomeId (0 for none). A missing key reads as its default, so a
// map is refused only for the rules of the core, never for its shape. The map always carries the
// metronome link, so that saving the texts of an exercise writes the link it shows (R-EXR-10).
namespace ExerciseForm {

QVariantMap toMap(const Exercise &exercise);

// Name trimmed; group keys that the table does not know are dropped, retired keys are converted.
Exercise fromMap(const QVariantMap &map);

// The reason an exercise is refused, as a fixed code: "name-empty", "name-too-long",
// "instructions-too-long", "tips-too-long", "muscles-none", "muscles-unknown", "icon", "colour" or
// "language".
QString problemCode(ExerciseProblem problem);

// The codes of a list of problems, in order and without repeats.
QStringList problemCodes(const QList<ExerciseProblem> &problems);

// True when the name contains the text without regard to case (an empty text matches all) and
// the exercise has the group (an empty key matches all). An unknown group key matches nothing.
bool matches(const Exercise &exercise, const QString &text, const QString &groupKey);

// The exercises that match, in the order given.
QList<Exercise> filter(const QList<Exercise> &exercises, const QString &text, const QString &groupKey);

}

#endif

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef METRONOMEFORM_H
#define METRONOMEFORM_H

#include <QString>
#include <QVariantMap>

#include "metronomestore.h"

// The settings of a run or of a rhythm as a map, the form the screens and the run bridge use, and
// the conversions to and from the saved rhythm. Keys of the pace map: form ("reps-total",
// "time-per-rep" or "phases"), reps, totalSec (reps-total), repHalfSec (time-per-rep, half
// seconds), phases (a list of numbers of half seconds), phaseNames (a list of strings, one per
// phase), sets, restSec. A saved rhythm has the keys id, key, name, phases and
// phaseNames. A missing key reads as its default (0, empty or 1 for the sets), so that a map is
// never refused for its shape, only for the rules of the core.
namespace MetronomeForm {

// The pace of a map (a quick mode, or a rhythm tried for a number of repetitions). The numbers
// are taken as they are: the rules of the core decide.
Tempo tempoFromMap(const QVariantMap &map);

// The map of a rhythm: id, key, name, phases and phaseNames.
QVariantMap toMap(const SavedMetronome &metronome);

// A rhythm from a map: id (0 if missing), key, name and the phases with their names;
// the rhythm has one repetition.
SavedMetronome fromMap(const QVariantMap &map);

// The reason a pace is refused, as a fixed code: "reps", "rep-short", "rep-long", "phase-count",
// "phase-length" or "phase-name"; empty for none.
QString tempoProblemCode(TempoProblem problem);

// The code of a problem of a saved metronome: the codes of tempoProblemCode() for a pace, and
// "name-empty", "name-too-long" or "identity".
QString problemCode(const MetronomeProblem &problem);

// The codes of a list of problems, in order and without repeats.
QStringList problemCodes(const QList<MetronomeProblem> &problems);

}

#endif

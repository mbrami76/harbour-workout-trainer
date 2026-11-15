// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef WORKOUTLIMITS_H
#define WORKOUTLIMITS_H

// Input limits of the workout model (R-WRK-8). Durations and rests are whole seconds.
namespace Limits {

const int minDurationSec = 1;
const int maxDurationSec = 3600;
const int minReps = 1;
const int maxReps = 999;
const int minRounds = 1;
const int maxRounds = 99;
const int minRestSec = 0;
const int maxRestSec = 600;
const int maxStepsPerBlock = 50;
const int maxBlocksPerWorkout = 50;

// Text limits in UTF-16 code units (R-EXR-1, R-WRK-1).
const int maxNameChars = 80;
const int maxInstructionsChars = 4000;
const int maxTipsChars = 2000;
const int maxIconKeyChars = 40;
const int minLanguageChars = 2;
const int maxLanguageChars = 35;

// Time attack (R-MOD-4): challenge names in UTF-16 code units, results in milliseconds.
const int maxChallengeNameChars = 60;
const int minResultMs = 1;
const int maxResultMs = 24 * 3600 * 1000;

// Metronome (R-MET-2): one repetition lasts 0.5 s to 60 s; a tempo has 1 to 8 phases of 1 to 20
// half seconds each. The screens start a new tempo with defaultPhaseCount phases.
const int minRepMs = 500;
const int maxRepMs = 60000;
const int halfSecondMs = 500;
const int minPhaseUnits = 1;
const int maxPhaseUnits = 20;
const int minPhases = 1;
const int maxPhases = 8;
const int defaultPhaseCount = 3;
// A phase name: at most this many characters as the user sees them (extended grapheme clusters).
const int maxPhaseNameChars = 12;
// Sets and rest of a saved metronome (R-MET-1, R-MET-7).
const int minSets = 1;
const int maxSets = 99;

// Lead-in before a step, in seconds (R-TMR-1).
const int minLeadInSec = 0;
const int maxLeadInSec = 10;
const int defaultLeadInSec = 3;

}

#endif

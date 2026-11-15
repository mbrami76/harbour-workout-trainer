// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef IDENTITY_H
#define IDENTITY_H

#include <QString>

#include "database.h"

// The identity of a workout or a time-attack challenge is its name together with its icon key and
// its colour key; that of a saved metronome is its name alone (R-WRK-12, R-MET-7, R-MOD-4). Two records may share a name only if
// the icon or the colour differs (a rhythm has neither: its name is unique). The empty key is the default and equals itself.
namespace Identity {

enum class Table { Workout, Challenge, Metronome };

// Name as compared: surrounding blank space removed, case folded.
QString nameKey(const QString &name);

// Order of two names in a list: without regard to case, except that a number in a copy suffix follows the plain copy
// ("x (copy)", "x (copy 2)", "x (copy 10)"). Negative, zero or positive like QString::compare (R-WRK-1).
int compareNames(const QString &a, const QString &b);

// True when both records have the same compared name, the same icon key and the same colour key.
bool isSame(const QString &nameA, const QString &iconA, const QString &colourA,
            const QString &nameB, const QString &iconB, const QString &colourB);

// An icon key is empty or 1 to maxIconKeyChars lower-case letters, digits or hyphens. Whether the
// application has a picture for the key is not checked (R-EXR-8).
bool isValidIconKey(const QString &icon);

// Looks for a record of the table, other than excludeId (0 for none), with the same identity.
// Returns false when the database reports an error.
bool isTaken(Database *database, Table table, const QString &name, const QString &icon,
             const QString &colour, int excludeId, bool *taken);

// The name itself if its identity is free, else the name with the suffix " (word)", " (word 2)", ...
// The suffix is added to a name cut short enough to keep the result within maxChars. Returns the
// name unchanged when the database reports an error. The record with excludeId (0 for none) is not
// compared, so that a record can keep its own name.
QString freeName(Database *database, Table table, const QString &name, const QString &icon,
                 const QString &colour, const QString &word, int maxChars, int excludeId = 0);

}

#endif

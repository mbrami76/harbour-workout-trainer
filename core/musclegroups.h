// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef MUSCLEGROUPS_H
#define MUSCLEGROUPS_H

#include <QString>
#include <QStringList>

#include "model.h"

// Stable keys of the muscle groups (R-MUS-4, R-MUS-6). Each group has one key and one permanent
// bit position; the counts and sets come from this table only. A retired group keeps its position
// and key but is never offered or written; stored sets and files that carry it are converted to
// the groups that replace it (R-MUS-5).
namespace MuscleKeys {

// Keys of the active groups in position order.
QStringList keys();

// Key of a group, retired or not; empty for a value outside the table.
QString keyOf(MuscleGroup group);

// Looks a key up, retired keys included; false for an unknown key.
bool fromKey(const QString &key, MuscleGroup *group);

bool isRetired(MuscleGroup group);

// Keys of the active groups in a set, in position order. Retired bits and bits outside the table
// are ignored.
QStringList keysOf(MuscleSet set);

// Clears the retired bits of a stored set and sets the bits of the groups that replace them.
MuscleSet convertRetired(MuscleSet set);

// The keys a key read from a file stands for: the replacements of a retired key, the key itself
// for an active one, nothing for an unknown one.
QStringList replacementKeys(const QString &key);

}

#endif

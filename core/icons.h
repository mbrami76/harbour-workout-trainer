// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef ICONS_H
#define ICONS_H

#include <QString>
#include <QStringList>

// Icon keys the application can offer for a workout-style record: a saved metronome, later a
// workout (R-EXR-8, R-MET-7). The empty key is the default icon and is always offered first. Only
// the default exists until more icons are drawn; a screen shows what this list holds, so adding a
// key here is all that a new icon needs from the logic.
namespace Icons {

// The keys to offer, default first.
QStringList workoutStyleKeys();

// The keys of the bundled workout icons (R-WRK-12), the default (empty key) first, then the twenty
// icons in the order of the picker. Every key has a picture in the application's workout icon folder.
QStringList workoutKeys();

// The keys of the bundled exercise icons (R-EXR-8, R-EXR-11), the default (empty key) first, then
// the others in name order. Every key has a picture in the application's exercise icon folder.
QStringList exerciseKeys();

}

#endif

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef SERVICES_H
#define SERVICES_H

#include <QObject>
#include <QString>

#include "database.h"
#include "exercisestore.h"
#include "metronomestore.h"
#include "monotonicclock.h"
#include "sessionlog.h"
#include "timeattackstore.h"
#include "workoutstore.h"

// The objects the whole app shares: the database, the stores on it, the monotonic clock and the
// starter exercises. One instance is created in main() before the QML engine and lives as long as
// the app. It is exposed to QML as the context property "services" with only `ready` and `error`.
//
// How the later tasks build on it:
// - A bridge object (one file each, in app/src/, named after what it serves) is a QObject that
//   takes a pointer to the Services in its constructor and asks it for the store or the clock it
//   needs. It is registered next to "services" in main(), never created from QML.
// - Every store call runs on the main thread, in the call of the QML handler that asked for it.
//   The stores are small and local, so nothing is moved to a worker thread; a bridge never keeps a
//   store call open across event loop turns.
// - Engines (the session engine, the interval timer, the metronome, the countdown and the
//   stopwatch) own no timer. A bridge drives one by polling it with the monotonic clock: its own
//   QTimer (33 ms) calls update() and the bridge turns the events into signals. The QTimer runs
//   only while the engine runs and the app is active; idle, paused and backgrounded cost nothing.
//   Remaining times are always read from the clock, never counted from the number of polls
//   (R-TMR-5). A bridge is created in main() and registered next to "services" as a context
//   property, so that no QML import of a C++ module is needed.
// - `ready` is false when the data cannot be used. Nothing may touch the stores then; `error`
//   holds a fixed code ("data-folder" or "database") that the error page turns into text.
class Services : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReady CONSTANT)
    Q_PROPERTY(QString error READ error CONSTANT)

public:
    // dataDirectory is the folder of the database (the platform's app data location);
    // starterDirectory holds the installed starter exercises file.
    Services(const QString &dataDirectory, const QString &starterDirectory, QObject *parent = nullptr);

    bool isReady() const { return error_.isEmpty(); }
    // Empty when ready; "data-folder" when the folder cannot be created or written; "database" when
    // the database cannot be opened or migrated (also when it was written by a newer version).
    QString error() const { return error_; }

    Database *database() { return &database_; }
    ExerciseStore *exercises() { return &exercises_; }
    WorkoutStore *workouts() { return &workouts_; }
    MetronomeStore *metronomes() { return &metronomes_; }
    SessionLog *sessionLog() { return &sessionLog_; }
    TimeAttackStore *timeAttack() { return &timeAttack_; }
    const Clock *clock() const { return &clock_; }

    // The language chosen for the interface and the starter library (R-I18N-5); empty until it is
    // known. New exercises carry it as their language tag (R-EXR-12).
    QString language() const { return language_; }
    void setLanguage(const QString &language) { language_ = language; }

    // Offers the starter library for the language (R-EXR-6, R-EXR-15, R-TPL-2, R-I18N-4): adds the
    // entries never offered before and gives the entries that follow the language the texts and
    // the language tag of the new one, in one transaction. What the user made, changed or deleted
    // is not touched. A file that is missing, unreadable or rejected is reported on the log and
    // leaves the data as it is; the app works without it. Emits startersLoaded() when something
    // changed.
    void loadStarters(const QString &language);

signals:
    void startersLoaded();

private:
    QString starterDirectory_;
    QString language_;
    Database database_;
    ExerciseStore exercises_;
    WorkoutStore workouts_;
    MetronomeStore metronomes_;
    SessionLog sessionLog_;
    TimeAttackStore timeAttack_;
    MonotonicClock clock_;
    QString error_;
};

#endif

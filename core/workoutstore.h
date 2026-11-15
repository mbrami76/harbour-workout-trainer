// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef WORKOUTSTORE_H
#define WORKOUTSTORE_H

#include "database.h"
#include "model.h"
#include "plan.h"

struct WorkoutSummary {
    int id = 0;
    QString name;
    bool favorite = false;
    QString icon;
    QString colour;
    int blockCount = 0;
    int stepCount = 0;
    int plannedSeconds = 0;
    // Repetition steps whose exercise has no metronome, left out of plannedSeconds.
    bool hasRepSteps = false;
    // Repetition steps whose exercise has a metronome, counted in plannedSeconds.
    bool hasTempoSteps = false;
};

// Workouts in the database (R-WRK-1, R-WRK-8, R-TPL-1).
class WorkoutStore
{
public:
    explicit WorkoutStore(Database *database) : db_(database) {}

    // validate() plus the name, icon and colour rules; the exercises and the identity (R-WRK-12) are
    // checked on write.
    static QList<Problem> validateWorkout(const Workout &workout, const ExerciseTempos &rhythms = ExerciseTempos());

    // A create or an update is Invalid with Problem::IdentityTaken when another workout has the
    // same name (without case and surrounding blank space), icon key and colour key; nothing is
    // written. An update that leaves name, icon and colour as stored is not compared.
    // The findings a create (excludeId 0) or an update of that workout would report, nothing
    // written: the rules of the core, the exercises and the identity. Sets *failed when the database
    // reports an error.
    QList<Problem> problemsOf(const Workout &workout, int excludeId = 0, bool *failed = nullptr);
    StoreStatus create(const Workout &workout, int *id = nullptr, QList<Problem> *problems = nullptr);
    bool get(int id, Workout *workout);
    // Favorites first, then by name without regard to case.
    QList<WorkoutSummary> list();
    // Replaces the whole tree in one transaction; the favorite flag stays.
    StoreStatus update(int id, const Workout &workout, QList<Problem> *problems = nullptr);
    StoreStatus remove(int id);
    // Independent copy, not a favorite, with the first name of "<name> (copy)", "<name> (copy 2)", ...
    // whose identity is free; the icon and colour are those of the original.
    StoreStatus duplicate(int id, int *newId = nullptr);
    StoreStatus setFavorite(int id, bool favorite);

    // The tempos of the exercises, read from the database now: the snapshot that the engine takes
    // with the workout (R-WRK-10).
    ExerciseTempos tempos();
    // The plan of a workout with the tempos of its exercises taken from the database now
    // (R-WRK-9, R-WRK-10). An exercise that lost its metronome gives plain repetition steps;
    // nothing fails.
    QList<PlanStep> plan(const Workout &workout);

private:
    // excludeId: the workout being updated, or 0. The identity is looked up only when it differs
    // from the stored one. Sets *failed when the database reports an error.
    QList<Problem> check(const Workout &workout, int excludeId, bool *failed);
    bool writeTree(int workoutId, const Workout &workout);
    QList<Workout> load(int id);
    bool exists(int id);
    Database *db_;
};

#endif

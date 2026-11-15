// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "workoutstore.h"

#include <QHash>
#include <QPair>
#include <QSet>
#include <algorithm>

#include "colours.h"
#include "identity.h"
#include "exercisestore.h"
#include "workoutlimits.h"
#include "plan.h"

namespace {

QVariant nullable(int value, bool use)
{
    return use ? QVariant(value) : QVariant(QVariant::Int);
}

}

QList<Problem> WorkoutStore::validateWorkout(const Workout &workout, const ExerciseTempos &rhythms)
{
    QList<Problem> found = validate(workout, rhythms);
    Problem problem;
    if (workout.name.trimmed().isEmpty()) {
        problem.code = Problem::NameEmpty;
        found.append(problem);
    }
    if (workout.name.length() > Limits::maxNameChars) {
        problem.code = Problem::NameTooLong;
        found.append(problem);
    }
    if (!Identity::isValidIconKey(workout.icon)) {
        problem.code = Problem::IconInvalid;
        found.append(problem);
    }
    if (!workout.colour.isEmpty() && !Colours::isKnown(workout.colour)) {
        problem.code = Problem::ColourUnknown;
        found.append(problem);
    }
    return found;
}

QList<Problem> WorkoutStore::problemsOf(const Workout &given, int excludeId, bool *failed)
{
    Workout workout = given;
    normalizeStartModes(workout);
    bool dbFailed = false;
    const QList<Problem> found = check(workout, excludeId, &dbFailed);
    if (failed)
        *failed = dbFailed;
    return found;
}

QList<Problem> WorkoutStore::check(const Workout &workout, int excludeId, bool *failed)
{
    *failed = false;
    QList<Problem> found = validateWorkout(workout, tempos());
    if (!found.isEmpty())
        return found;

    QSet<QString> known;
    for (int b = 0; b < workout.blocks.size(); ++b) {
        const QList<Step> &steps = workout.blocks.at(b).steps;
        for (int s = 0; s < steps.size(); ++s) {
            const QString &key = steps.at(s).exerciseKey;
            if (key.isEmpty() || known.contains(key))
                continue;
            QSqlQuery q;
            if (db_->run(QStringLiteral("SELECT 1 FROM exercise WHERE key = ?"), QVariantList() << key, &q) && q.next()) {
                known.insert(key);
                continue;
            }
            Problem problem;
            problem.code = Problem::UnknownExercise;
            problem.blockIndex = b;
            problem.stepIndex = s;
            found.append(problem);
        }
    }
    if (!found.isEmpty())
        return found;

    // An edit that leaves name, icon and colour as stored is not compared: older data may hold
    // equal identities, and the other fields can still be changed.
    if (excludeId > 0) {
        QSqlQuery q;
        if (!db_->run(QStringLiteral("SELECT name, icon, colour FROM workout WHERE id = ?"),
                      QVariantList() << excludeId, &q)) {
            *failed = true;
            return found;
        }
        if (q.next() && Identity::isSame(workout.name, workout.icon, workout.colour, q.value(0).toString(),
                                         q.value(1).toString(), q.value(2).toString()))
            return found;
    }
    bool taken = false;
    if (!Identity::isTaken(db_, Identity::Table::Workout, workout.name, workout.icon, workout.colour, excludeId,
                           &taken)) {
        *failed = true;
        return found;
    }
    if (taken) {
        Problem problem;
        problem.code = Problem::IdentityTaken;
        found.append(problem);
    }
    return found;
}

bool WorkoutStore::writeTree(int workoutId, const Workout &workout)
{
    for (int b = 0; b < workout.blocks.size(); ++b) {
        const Block &block = workout.blocks.at(b);
        if (!db_->run(QStringLiteral("INSERT INTO block (workout_id, position, rounds, rest_between_rounds) "
                                     "VALUES (?, ?, ?, ?)"),
                      QVariantList() << workoutId << b << block.rounds << block.restBetweenRoundsSec))
            return false;
        const qint64 blockId = db_->lastInsertId();
        for (int s = 0; s < block.steps.size(); ++s) {
            const Step &step = block.steps.at(s);
            QVariant key = step.exerciseKey.isEmpty() ? QVariant(QVariant::String) : QVariant(step.exerciseKey);
            if (!db_->run(QStringLiteral("INSERT INTO step (block_id, position, type, exercise_key, duration_sec, reps, "
                                         "start_on_tap, pace_kind, pace_ms) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"),
                          QVariantList() << blockId << s << stepTypeName(step.type) << key
                                         << nullable(step.durationSec, step.durationSec != 0)
                                         << nullable(step.reps, step.reps != 0)
                                         << (step.startOnTap ? 1 : 0)
                                         << textValue(paceKindName(step.pace)) << (step.pace == PaceKind::None ? 0 : step.paceMs)))
                return false;
        }
    }
    return true;
}

StoreStatus WorkoutStore::create(const Workout &given, int *id, QList<Problem> *problems)
{
    Workout workout = given;
    normalizeStartModes(workout);
    bool failed = false;
    const QList<Problem> found = check(workout, 0, &failed);
    if (problems)
        *problems = found;
    if (failed)
        return StoreStatus::Failed;
    if (!found.isEmpty())
        return StoreStatus::Invalid;

    Transaction transaction(db_);
    if (!transaction.isActive())
        return StoreStatus::Failed;
    if (!db_->run(QStringLiteral("INSERT INTO workout (name, favorite, rest_between_blocks, icon, colour) "
                                 "VALUES (?, 0, ?, ?, ?)"),
                  QVariantList() << textValue(workout.name) << workout.restBetweenBlocksSec
                                 << textValue(workout.icon) << textValue(workout.colour)))
        return StoreStatus::Failed;
    const int newId = int(db_->lastInsertId());
    if (!writeTree(newId, workout) || !transaction.commit())
        return StoreStatus::Failed;
    if (id)
        *id = newId;
    return StoreStatus::Ok;
}

bool WorkoutStore::exists(int id)
{
    QSqlQuery q;
    return db_->run(QStringLiteral("SELECT 1 FROM workout WHERE id = ?"), QVariantList() << id, &q) && q.next();
}

// Loads one workout (id > 0) or all of them with three queries.
QList<Workout> WorkoutStore::load(int id)
{
    QList<Workout> workouts;
    QHash<int, int> workoutIndex;
    QHash<int, QPair<int, int> > blockIndex;
    const QString filter = id > 0 ? QStringLiteral(" WHERE id = %1").arg(id) : QString();
    const QString blockFilter = id > 0 ? QStringLiteral(" WHERE workout_id = %1").arg(id) : QString();

    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT id, name, favorite, rest_between_blocks, icon, colour FROM workout") + filter, QVariantList(), &q))
        return workouts;
    while (q.next()) {
        Workout w;
        w.id = q.value(0).toInt();
        w.name = q.value(1).toString();
        w.favorite = q.value(2).toInt() != 0;
        w.restBetweenBlocksSec = q.value(3).toInt();
        w.icon = q.value(4).toString();
        w.colour = q.value(5).toString();
        workoutIndex.insert(w.id, workouts.size());
        workouts.append(w);
    }

    if (!db_->run(QStringLiteral("SELECT id, workout_id, rounds, rest_between_rounds FROM block")
                      + blockFilter + QStringLiteral(" ORDER BY workout_id, position"), QVariantList(), &q))
        return QList<Workout>();
    while (q.next()) {
        Block block;
        block.rounds = q.value(2).toInt();
        block.restBetweenRoundsSec = q.value(3).toInt();
        const int w = workoutIndex.value(q.value(1).toInt(), -1);
        if (w < 0)
            continue;
        blockIndex.insert(q.value(0).toInt(), qMakePair(w, workouts.at(w).blocks.size()));
        workouts[w].blocks.append(block);
    }

    const QString stepSql = id > 0
        ? QStringLiteral("SELECT s.block_id, s.type, s.exercise_key, s.duration_sec, s.reps, s.start_on_tap, s.pace_kind, s.pace_ms FROM step s "
                         "JOIN block b ON b.id = s.block_id WHERE b.workout_id = %1 ORDER BY s.block_id, s.position").arg(id)
        : QStringLiteral("SELECT block_id, type, exercise_key, duration_sec, reps, start_on_tap, pace_kind, pace_ms FROM step ORDER BY block_id, position");
    if (!db_->run(stepSql, QVariantList(), &q))
        return QList<Workout>();
    while (q.next()) {
        const QPair<int, int> at = blockIndex.value(q.value(0).toInt(), qMakePair(-1, -1));
        if (at.first < 0)
            continue;
        Step step;
        stepTypeFromName(q.value(1).toString(), &step.type);
        step.exerciseKey = q.value(2).toString();
        step.durationSec = q.value(3).toInt();
        step.reps = q.value(4).toInt();
        step.startOnTap = q.value(5).toInt() != 0;
        if (!paceKindFromName(q.value(6).toString(), &step.pace))
            step.pace = PaceKind::None;
        step.paceMs = step.pace == PaceKind::None ? 0 : q.value(7).toInt();
        workouts[at.first].blocks[at.second].steps.append(step);
    }
    // A flag stored before the rule, or written by hand, is not honoured on the first step.
    for (Workout &w : workouts)
        normalizeStartModes(w);
    return workouts;
}

bool WorkoutStore::get(int id, Workout *workout)
{
    const QList<Workout> found = load(id);
    if (found.isEmpty())
        return false;
    *workout = found.first();
    return true;
}

QList<WorkoutSummary> WorkoutStore::list()
{
    QList<WorkoutSummary> summaries;
    const QList<Workout> all = load(0);
    const ExerciseTempos tempos = ExerciseStore(db_).tempoMap();
    for (const Workout &w : all) {
        WorkoutSummary summary;
        summary.id = w.id;
        summary.name = w.name;
        summary.favorite = w.favorite;
        summary.icon = w.icon;
        summary.colour = w.colour;
        summary.blockCount = w.blocks.size();
        for (const Block &b : w.blocks)
            summary.stepCount += b.steps.size();
        const PlannedTotal total = totalPlannedSeconds(w, tempos);
        summary.plannedSeconds = total.seconds;
        summary.hasRepSteps = total.hasRepSteps;
        summary.hasTempoSteps = total.hasTempoSteps;
        summaries.append(summary);
    }
    std::sort(summaries.begin(), summaries.end(), [](const WorkoutSummary &a, const WorkoutSummary &b) {
        if (a.favorite != b.favorite)
            return a.favorite;
        const int byName = Identity::compareNames(a.name, b.name);
        return byName != 0 ? byName < 0 : a.id < b.id;
    });
    return summaries;
}

StoreStatus WorkoutStore::update(int id, const Workout &given, QList<Problem> *problems)
{
    Workout workout = given;
    normalizeStartModes(workout);
    bool failed = false;
    const QList<Problem> found = check(workout, id, &failed);
    if (problems)
        *problems = found;
    if (failed)
        return StoreStatus::Failed;
    if (!found.isEmpty())
        return StoreStatus::Invalid;
    if (!exists(id))
        return StoreStatus::NotFound;

    Transaction transaction(db_);
    if (!transaction.isActive())
        return StoreStatus::Failed;
    // The first change of the name detaches the workout from the language for good (R-EXR-15).
    if (!db_->run(QStringLiteral("UPDATE workout SET follows_language = CASE WHEN name = ?1 THEN follows_language ELSE 0 END, "
                                 "name = ?1, rest_between_blocks = ?2, icon = ?3, colour = ?4 WHERE id = ?5"),
                  QVariantList() << textValue(workout.name) << workout.restBetweenBlocksSec
                                 << textValue(workout.icon) << textValue(workout.colour) << id)
        || !db_->run(QStringLiteral("DELETE FROM block WHERE workout_id = ?"), QVariantList() << id)
        || !writeTree(id, workout) || !transaction.commit())
        return StoreStatus::Failed;
    return StoreStatus::Ok;
}

StoreStatus WorkoutStore::remove(int id)
{
    if (!exists(id))
        return StoreStatus::NotFound;
    return db_->run(QStringLiteral("DELETE FROM workout WHERE id = ?"), QVariantList() << id)
        ? StoreStatus::Ok : StoreStatus::Failed;
}

StoreStatus WorkoutStore::duplicate(int id, int *newId)
{
    Workout copy;
    if (!get(id, &copy))
        return StoreStatus::NotFound;
    copy.name = Identity::freeName(db_, Identity::Table::Workout, copy.name, copy.icon, copy.colour,
                                   QStringLiteral("copy"), Limits::maxNameChars);
    copy.favorite = false;
    return create(copy, newId);
}

ExerciseTempos WorkoutStore::tempos()
{
    return ExerciseStore(db_).tempoMap();
}

QList<PlanStep> WorkoutStore::plan(const Workout &workout)
{
    return expandPlan(workout, tempos());
}

StoreStatus WorkoutStore::setFavorite(int id, bool favorite)
{
    if (!exists(id))
        return StoreStatus::NotFound;
    return db_->run(QStringLiteral("UPDATE workout SET favorite = ? WHERE id = ?"),
                    QVariantList() << (favorite ? 1 : 0) << id)
        ? StoreStatus::Ok : StoreStatus::Failed;
}

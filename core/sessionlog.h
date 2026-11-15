// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef SESSIONLOG_H
#define SESSIONLOG_H

#include <functional>

#include "database.h"
#include "engine.h"
#include "model.h"
#include "plan.h"

enum class EndState { Completed, Stopped };

// One executed step as stored. Names and muscle sets are copies (R-HST-2).
struct LoggedStep {
    int position = 0;
    QString exerciseName;
    MuscleSet muscles = 0;
    StepType type = StepType::Work;
    int plannedSec = 0;
    int plannedReps = 0;
    int actualSec = 0;
};

struct LogEntry {
    int id = 0;
    QString workoutName;
    qint64 startedAtMs = 0;
    int totalSec = 0;
    EndState endState = EndState::Completed;
    QList<LoggedStep> steps;
};

// Finds the name and muscle set of an exercise by key; false if it no longer exists.
typedef std::function<bool(const QString &key, QString *name, MuscleSet *muscles)> ExerciseLookup;

// Builds a log entry from the engine's records (R-HST-1, R-HST-2). Records of the same plan step
// are merged by summing their active time; steps with no active time are dropped; the others keep
// the order of their first entry. Seconds are rounded to the nearest whole second, and the total
// is the rounded sum of the active milliseconds (lead-ins and pauses are not part of it).
LogEntry buildLogEntry(const QString &workoutName, qint64 startedAtMs, const QList<PlanStep> &plan,
                       const QList<StepRecord> &records, EndState endState, const ExerciseLookup &lookup);

// A session that stopped before any step ran has no step with active time and stores nothing
// (R-HST-1).
inline bool isLoggable(const LogEntry &entry) { return !entry.steps.isEmpty(); }

// Session history in the database.
class SessionLog
{
public:
    explicit SessionLog(Database *database) : db_(database) {}

    StoreStatus add(const LogEntry &entry, int *id = nullptr);
    // Newest first; limit 0 means all.
    QList<LogEntry> list(int limit = 0);
    // The end of the newest session that ended as completed, in milliseconds since the epoch: its
    // start plus its active time. False when there is none or the log cannot be read (R-UI-17).
    bool lastCompletedEndMs(qint64 *endMs);

private:
    Database *db_;
};

#endif

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "sessionlog.h"

#include <QHash>

namespace {

qint64 roundedSeconds(qint64 ms)
{
    return (ms + 500) / 1000;
}

}

LogEntry buildLogEntry(const QString &workoutName, qint64 startedAtMs, const QList<PlanStep> &plan,
                       const QList<StepRecord> &records, EndState endState, const ExerciseLookup &lookup)
{
    LogEntry entry;
    entry.workoutName = workoutName;
    entry.startedAtMs = startedAtMs;
    entry.endState = endState;

    QList<int> order;
    QHash<int, qint64> activeMs;
    for (const StepRecord &record : records) {
        if (record.stepIndex < 0 || record.stepIndex >= plan.size())
            continue;
        if (!activeMs.contains(record.stepIndex))
            order.append(record.stepIndex);
        activeMs[record.stepIndex] += record.activeMs;
    }

    qint64 totalMs = 0;
    for (int index : order) {
        const qint64 ms = activeMs.value(index);
        if (ms <= 0)
            continue;
        const PlanStep &step = plan.at(index);
        LoggedStep logged;
        logged.position = entry.steps.size();
        logged.type = step.type;
        logged.plannedSec = step.durationSec;
        logged.plannedReps = step.reps;
        logged.actualSec = int(roundedSeconds(ms));
        if (!step.exerciseKey.isEmpty())
            lookup(step.exerciseKey, &logged.exerciseName, &logged.muscles);
        entry.steps.append(logged);
        totalMs += ms;
    }
    entry.totalSec = int(roundedSeconds(totalMs));
    return entry;
}

StoreStatus SessionLog::add(const LogEntry &entry, int *id)
{
    Transaction transaction(db_);
    if (!transaction.isActive())
        return StoreStatus::Failed;

    const QString state = entry.endState == EndState::Completed ? QStringLiteral("completed") : QStringLiteral("stopped");
    if (!db_->run(QStringLiteral("INSERT INTO session (workout_name, started_at, total_sec, end_state) VALUES (?, ?, ?, ?)"),
                  QVariantList() << textValue(entry.workoutName) << entry.startedAtMs << entry.totalSec << state))
        return StoreStatus::Failed;
    const qint64 sessionId = db_->lastInsertId();

    for (const LoggedStep &step : entry.steps) {
        const QVariant sec = step.plannedSec > 0 ? QVariant(step.plannedSec) : QVariant(QVariant::Int);
        const QVariant reps = step.plannedReps > 0 ? QVariant(step.plannedReps) : QVariant(QVariant::Int);
        if (!db_->run(QStringLiteral("INSERT INTO session_step (session_id, position, exercise_name, muscles, type, "
                                     "planned_sec, planned_reps, actual_sec) VALUES (?, ?, ?, ?, ?, ?, ?, ?)"),
                      QVariantList() << sessionId << step.position << textValue(step.exerciseName) << step.muscles
                                     << stepTypeName(step.type) << sec << reps << step.actualSec))
            return StoreStatus::Failed;
    }
    if (!transaction.commit())
        return StoreStatus::Failed;
    if (id)
        *id = int(sessionId);
    return StoreStatus::Ok;
}

QList<LogEntry> SessionLog::list(int limit)
{
    QList<LogEntry> entries;
    QHash<int, int> index;
    QSqlQuery q;
    QString sql = QStringLiteral("SELECT id, workout_name, started_at, total_sec, end_state FROM session "
                                 "ORDER BY started_at DESC, id DESC");
    if (limit > 0)
        sql += QStringLiteral(" LIMIT %1").arg(limit);
    if (!db_->run(sql, QVariantList(), &q))
        return entries;
    while (q.next()) {
        LogEntry entry;
        entry.id = q.value(0).toInt();
        entry.workoutName = q.value(1).toString();
        entry.startedAtMs = q.value(2).toLongLong();
        entry.totalSec = q.value(3).toInt();
        entry.endState = q.value(4).toString() == QLatin1String("completed") ? EndState::Completed : EndState::Stopped;
        index.insert(entry.id, entries.size());
        entries.append(entry);
    }

    if (!db_->run(QStringLiteral("SELECT session_id, position, exercise_name, muscles, type, planned_sec, planned_reps, "
                                 "actual_sec FROM session_step ORDER BY session_id, position"), QVariantList(), &q))
        return QList<LogEntry>();
    while (q.next()) {
        const int at = index.value(q.value(0).toInt(), -1);
        if (at < 0)
            continue;
        LoggedStep step;
        step.position = q.value(1).toInt();
        step.exerciseName = q.value(2).toString();
        step.muscles = q.value(3).toULongLong();
        stepTypeFromName(q.value(4).toString(), &step.type);
        step.plannedSec = q.value(5).toInt();
        step.plannedReps = q.value(6).toInt();
        step.actualSec = q.value(7).toInt();
        entries[at].steps.append(step);
    }
    return entries;
}

bool SessionLog::lastCompletedEndMs(qint64 *endMs)
{
    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT started_at, total_sec FROM session WHERE end_state = 'completed' "
                                 "ORDER BY started_at DESC, id DESC LIMIT 1"), QVariantList(), &q))
        return false;
    if (!q.next())
        return false;
    *endMs = q.value(0).toLongLong() + q.value(1).toLongLong() * 1000;
    return true;
}

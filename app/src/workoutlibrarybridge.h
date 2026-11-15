// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef WORKOUTLIBRARYBRIDGE_H
#define WORKOUTLIBRARYBRIDGE_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class Services;

// The saved workouts (R-WRK-1, R-WRK-9, R-TPL-1) for QML. Exposed as the context property
// "workoutLibrary", created in main() with the Services. All store calls run on the main thread. A
// result map has `status`: "ok", "invalid", "not-found" or "failed".
class WorkoutLibraryBridge : public QObject
{
    Q_OBJECT
    // Favourites first, then by name without regard to case (R-TPL-1): [{id, name, icon, colour,
    // favorite, blockCount, stepCount, plannedSeconds, hasRepSteps, hasTempoSteps}].
    Q_PROPERTY(QVariantList workouts READ workouts NOTIFY workoutsChanged)

public:
    explicit WorkoutLibraryBridge(Services *services, QObject *parent = nullptr);

    QVariantList workouts() const { return workouts_; }

    // Reads the list again.
    Q_INVOKABLE void refresh();
    // The workout read only: {found, header, rows, total}. header is {id, favorite, name, icon,
    // colour, restBetweenBlocksSec, blockCount, stepCount}; rows are the flat rows of a ListView
    // (see WorkoutView::rows()) with the rests between blocks; total is the planned time with the
    // steps it leaves out.
    Q_INVOKABLE QVariantMap detail(int id);
    // An independent copy with the first free " (copy)" name: {status, name}.
    Q_INVOKABLE QVariantMap duplicate(int id);
    Q_INVOKABLE QVariantMap setFavorite(int id, bool favorite);
    Q_INVOKABLE QVariantMap remove(int id);

signals:
    void workoutsChanged();

private:
    QVariantMap result(const QString &status) const;

    Services *services_;
    QVariantList workouts_;
};

#endif

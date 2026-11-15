// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "workoutlibrarybridge.h"

#include "services.h"
#include "workoutform.h"
#include "workoutview.h"

namespace {

QString statusName(StoreStatus status)
{
    switch (status) {
    case StoreStatus::Ok: return QStringLiteral("ok");
    case StoreStatus::Invalid: return QStringLiteral("invalid");
    case StoreStatus::NotFound: return QStringLiteral("not-found");
    default: return QStringLiteral("failed");
    }
}

QVariantMap summaryToMap(const WorkoutSummary &s)
{
    QVariantMap map;
    map.insert(QStringLiteral("id"), s.id);
    map.insert(QStringLiteral("name"), s.name);
    map.insert(QStringLiteral("icon"), s.icon);
    map.insert(QStringLiteral("colour"), s.colour);
    map.insert(QStringLiteral("favorite"), s.favorite);
    map.insert(QStringLiteral("blockCount"), s.blockCount);
    map.insert(QStringLiteral("stepCount"), s.stepCount);
    map.insert(QStringLiteral("plannedSeconds"), s.plannedSeconds);
    map.insert(QStringLiteral("hasRepSteps"), s.hasRepSteps);
    map.insert(QStringLiteral("hasTempoSteps"), s.hasTempoSteps);
    return map;
}

}

WorkoutLibraryBridge::WorkoutLibraryBridge(Services *services, QObject *parent)
    : QObject(parent), services_(services)
{
    connect(services, &Services::startersLoaded, this, &WorkoutLibraryBridge::refresh);
}

QVariantMap WorkoutLibraryBridge::result(const QString &status) const
{
    QVariantMap map;
    map.insert(QStringLiteral("status"), status);
    return map;
}

void WorkoutLibraryBridge::refresh()
{
    workouts_.clear();
    if (services_->isReady()) {
        for (const WorkoutSummary &s : services_->workouts()->list())
            workouts_.append(summaryToMap(s));
    }
    emit workoutsChanged();
}

QVariantMap WorkoutLibraryBridge::detail(int id)
{
    QVariantMap map;
    Workout w;
    if (!services_->isReady() || !services_->workouts()->get(id, &w)) {
        map.insert(QStringLiteral("found"), false);
        return map;
    }
    const WorkoutView::Lookups lookups = WorkoutView::load(services_);
    WorkoutView::RowOptions options;
    options.blockRests = true;
    map.insert(QStringLiteral("found"), true);
    map.insert(QStringLiteral("header"), WorkoutForm::headerToMap(w));
    map.insert(QStringLiteral("rows"), WorkoutView::rows(w, lookups, QVariantList(), options));
    map.insert(QStringLiteral("total"), WorkoutView::total(w, lookups));
    return map;
}

QVariantMap WorkoutLibraryBridge::duplicate(int id)
{
    if (!services_->isReady())
        return result(QStringLiteral("failed"));
    int newId = 0;
    const StoreStatus status = services_->workouts()->duplicate(id, &newId);
    QVariantMap map = result(statusName(status));
    if (status == StoreStatus::Ok) {
        Workout copy;
        if (services_->workouts()->get(newId, &copy))
            map.insert(QStringLiteral("name"), copy.name);
        refresh();
    }
    return map;
}

QVariantMap WorkoutLibraryBridge::setFavorite(int id, bool favorite)
{
    if (!services_->isReady())
        return result(QStringLiteral("failed"));
    const StoreStatus status = services_->workouts()->setFavorite(id, favorite);
    if (status == StoreStatus::Ok)
        refresh();
    return result(statusName(status));
}

QVariantMap WorkoutLibraryBridge::remove(int id)
{
    if (!services_->isReady())
        return result(QStringLiteral("failed"));
    const StoreStatus status = services_->workouts()->remove(id);
    if (status == StoreStatus::Ok)
        refresh();
    return result(statusName(status));
}

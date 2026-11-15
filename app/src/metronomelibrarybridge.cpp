// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "metronomelibrarybridge.h"

#include "metronomeform.h"
#include "services.h"

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

QVariantList refsToList(const QList<ExerciseRef> &refs)
{
    QVariantList list;
    for (const ExerciseRef &ref : refs) {
        QVariantMap map;
        map.insert(QStringLiteral("key"), ref.key);
        map.insert(QStringLiteral("name"), ref.name);
        list.append(map);
    }
    return list;
}

QVariantMap summaryToMap(const MetronomeSummary &s)
{
    QVariantMap map;
    map.insert(QStringLiteral("id"), s.id);
    map.insert(QStringLiteral("name"), s.name);
    map.insert(QStringLiteral("repetitionMs"), s.repetitionMs);
    map.insert(QStringLiteral("phaseCount"), s.phaseCount);
    QVariantList units;
    for (int unit : s.phaseUnits)
        units.append(unit);
    map.insert(QStringLiteral("phaseUnits"), units);
    QVariantList names;
    for (const QString &name : s.phaseNames)
        names.append(name);
    map.insert(QStringLiteral("phaseNames"), names);
    map.insert(QStringLiteral("exerciseCount"), s.exerciseCount);
    return map;
}

}

MetronomeLibraryBridge::MetronomeLibraryBridge(Services *services, QObject *parent)
    : QObject(parent), services_(services)
{
}


QVariantMap MetronomeLibraryBridge::result(const QString &status) const
{
    QVariantMap map;
    map.insert(QStringLiteral("status"), status);
    return map;
}

void MetronomeLibraryBridge::refresh()
{
    metronomes_.clear();
    if (services_->isReady()) {
        for (const MetronomeSummary &s : services_->metronomes()->list())
            metronomes_.append(summaryToMap(s));
    }
    emit metronomesChanged();
}

QVariantMap MetronomeLibraryBridge::get(int id)
{
    SavedMetronome m;
    if (!services_->isReady() || !services_->metronomes()->get(id, &m))
        return QVariantMap();
    QVariantMap map = MetronomeForm::toMap(m);
    map.insert(QStringLiteral("exerciseCount"), services_->metronomes()->exercisesUsing(id).size());
    return map;
}

QStringList MetronomeLibraryBridge::problems(int id, const QVariantMap &metronome)
{
    if (!services_->isReady())
        return QStringList() << QStringLiteral("failed");
    bool failed = false;
    const QList<MetronomeProblem> found =
        services_->metronomes()->problemsOf(MetronomeForm::fromMap(metronome), id, &failed);
    if (failed)
        return QStringList() << QStringLiteral("failed");
    return MetronomeForm::problemCodes(found);
}

QVariantMap MetronomeLibraryBridge::create(const QVariantMap &metronome)
{
    if (!services_->isReady())
        return result(QStringLiteral("failed"));
    int id = 0;
    QList<MetronomeProblem> found;
    const StoreStatus status = services_->metronomes()->create(MetronomeForm::fromMap(metronome), &id, &found);
    QVariantMap map = result(statusName(status));
    map.insert(QStringLiteral("id"), status == StoreStatus::Ok ? id : 0);
    map.insert(QStringLiteral("problems"), MetronomeForm::problemCodes(found));
    if (status == StoreStatus::Ok)
        refresh();
    return map;
}

QVariantList MetronomeLibraryBridge::exercisesUsing(int id)
{
    if (!services_->isReady())
        return QVariantList();
    return refsToList(services_->metronomes()->exercisesUsing(id));
}

QVariantMap MetronomeLibraryBridge::update(int id, const QVariantMap &metronome)
{
    if (!services_->isReady())
        return result(QStringLiteral("failed"));
    QList<MetronomeProblem> found;
    QList<ExerciseRef> affected;
    const StoreStatus status =
        services_->metronomes()->update(id, MetronomeForm::fromMap(metronome), &found, &affected);
    QVariantMap map = result(statusName(status));
    map.insert(QStringLiteral("problems"), MetronomeForm::problemCodes(found));
    map.insert(QStringLiteral("affected"), refsToList(affected));
    if (status == StoreStatus::Ok)
        refresh();
    return map;
}

QVariantMap MetronomeLibraryBridge::saveAsCopy(const QVariantMap &metronome)
{
    if (!services_->isReady())
        return result(QStringLiteral("failed"));
    int id = 0;
    QList<MetronomeProblem> found;
    const StoreStatus status = services_->metronomes()->saveAsCopy(MetronomeForm::fromMap(metronome), &id, &found);
    QVariantMap map = result(statusName(status));
    map.insert(QStringLiteral("id"), status == StoreStatus::Ok ? id : 0);
    map.insert(QStringLiteral("problems"), MetronomeForm::problemCodes(found));
    if (status == StoreStatus::Ok) {
        map.insert(QStringLiteral("name"), get(id).value(QStringLiteral("name")));
        refresh();
    }
    return map;
}

QVariantMap MetronomeLibraryBridge::duplicate(int id)
{
    if (!services_->isReady())
        return result(QStringLiteral("failed"));
    int newId = 0;
    const StoreStatus status = services_->metronomes()->duplicate(id, &newId);
    QVariantMap map = result(statusName(status));
    map.insert(QStringLiteral("id"), status == StoreStatus::Ok ? newId : 0);
    if (status == StoreStatus::Ok) {
        map.insert(QStringLiteral("name"), get(newId).value(QStringLiteral("name")));
        refresh();
    }
    return map;
}

QVariantMap MetronomeLibraryBridge::remove(int id)
{
    if (!services_->isReady())
        return result(QStringLiteral("failed"));
    QList<ExerciseRef> cleared;
    const StoreStatus status = services_->metronomes()->remove(id, &cleared);
    QVariantMap map = result(statusName(status));
    map.insert(QStringLiteral("cleared"), refsToList(cleared));
    if (status == StoreStatus::Ok)
        refresh();
    return map;
}

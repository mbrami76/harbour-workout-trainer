// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "exerciselibrarybridge.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include "colours.h"
#include "exerciseexchange.h"
#include "exerciseform.h"
#include "icons.h"
#include "language.h"
#include "musclegroups.h"
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

QString fieldName(ExerciseField field)
{
    switch (field) {
    case ExerciseField::Name: return QStringLiteral("name");
    case ExerciseField::Muscles: return QStringLiteral("muscles");
    case ExerciseField::Instructions: return QStringLiteral("instructions");
    case ExerciseField::Tips: return QStringLiteral("tips");
    case ExerciseField::Icon: return QStringLiteral("icon");
    case ExerciseField::Colour: return QStringLiteral("colour");
    case ExerciseField::Language: return QStringLiteral("language");
    case ExerciseField::Metronome: return QStringLiteral("metronome");
    }
    return QString();
}

QVariantMap summaryOf(const Exercise &e)
{
    QVariantMap map;
    map.insert(QStringLiteral("key"), e.key);
    map.insert(QStringLiteral("name"), e.name);
    map.insert(QStringLiteral("icon"), e.icon);
    map.insert(QStringLiteral("colour"), e.colour);
    map.insert(QStringLiteral("muscles"), MuscleKeys::keysOf(e.muscles));
    map.insert(QStringLiteral("hasTempo"), e.metronomeId > 0);
    return map;
}

}

ExerciseLibraryBridge::ExerciseLibraryBridge(Services *services, QObject *parent)
    : QObject(parent), services_(services)
{
    connect(services, &Services::startersLoaded, this, &ExerciseLibraryBridge::refresh);
}

QString ExerciseLibraryBridge::defaultLanguage() const
{
    return Language::newExerciseTag(services_->language());
}

QStringList ExerciseLibraryBridge::languages()
{
    return Language::exerciseChoices(services_->isReady() ? services_->exercises()->languages() : QStringList());
}

QStringList ExerciseLibraryBridge::muscleKeys() const { return MuscleKeys::keys(); }
QStringList ExerciseLibraryBridge::colours() const { return Colours::keys(); }
QStringList ExerciseLibraryBridge::icons() const { return Icons::exerciseKeys(); }

QVariantMap ExerciseLibraryBridge::result(const QString &status) const
{
    QVariantMap map;
    map.insert(QStringLiteral("status"), status);
    return map;
}

void ExerciseLibraryBridge::setSearchText(const QString &text)
{
    if (text == search_)
        return;
    search_ = text;
    rebuild();
    emit searchTextChanged();
}

void ExerciseLibraryBridge::setGroupFilter(const QString &key)
{
    if (key == group_)
        return;
    group_ = key;
    rebuild();
    emit groupFilterChanged();
}

void ExerciseLibraryBridge::refresh()
{
    all_.clear();
    if (services_->isReady())
        all_ = services_->exercises()->list();
    rebuild();
}

void ExerciseLibraryBridge::rebuild()
{
    total_ = all_.size();
    shown_.clear();
    for (const Exercise &e : ExerciseForm::filter(all_, search_, group_))
        shown_.append(summaryOf(e));
    emit exercisesChanged();
}

QVariantMap ExerciseLibraryBridge::get(const QString &key)
{
    Exercise e;
    if (!services_->isReady() || !services_->exercises()->get(key, &e))
        return QVariantMap();
    return ExerciseForm::toMap(e);
}

QStringList ExerciseLibraryBridge::problems(const QVariantMap &exercise)
{
    return ExerciseForm::problemCodes(ExerciseStore::validate(ExerciseForm::fromMap(exercise)));
}

QVariantMap ExerciseLibraryBridge::create(const QVariantMap &exercise)
{
    if (!services_->isReady())
        return result(QStringLiteral("failed"));
    Exercise e = ExerciseForm::fromMap(exercise);
    if (!exercise.contains(QStringLiteral("language")))
        e.language = defaultLanguage();
    QList<ExerciseProblem> found;
    const StoreStatus status = services_->exercises()->create(&e, &found);
    QVariantMap map = result(statusName(status));
    map.insert(QStringLiteral("key"), status == StoreStatus::Ok ? e.key : QString());
    map.insert(QStringLiteral("problems"), ExerciseForm::problemCodes(found));
    if (status == StoreStatus::Ok)
        refresh();
    return map;
}

QVariantMap ExerciseLibraryBridge::update(const QVariantMap &exercise)
{
    if (!services_->isReady())
        return result(QStringLiteral("failed"));
    QList<ExerciseProblem> found;
    const StoreStatus status = services_->exercises()->update(ExerciseForm::fromMap(exercise), &found);
    QVariantMap map = result(statusName(status));
    map.insert(QStringLiteral("problems"), ExerciseForm::problemCodes(found));
    if (status == StoreStatus::Ok)
        refresh();
    return map;
}

QVariantMap ExerciseLibraryBridge::impact(const QVariantMap &exercise)
{
    QVariantMap map;
    QStringList fields;
    QStringList workouts;
    ExerciseChange change;
    if (services_->isReady()
        && services_->exercises()->describeChange(ExerciseForm::fromMap(exercise), &change) == StoreStatus::Ok) {
        for (ExerciseField field : change.fields)
            fields.append(fieldName(field));
        workouts = change.workouts;
    }
    map.insert(QStringLiteral("fields"), fields);
    map.insert(QStringLiteral("workouts"), workouts);
    return map;
}

QVariantMap ExerciseLibraryBridge::saveAsCopy(const QVariantMap &exercise)
{
    QVariantMap copy = exercise;
    copy.remove(QStringLiteral("key"));
    copy.remove(QStringLiteral("starter"));
    copy.insert(QStringLiteral("name"), exercise.value(QStringLiteral("name")).toString().trimmed()
                                            + QStringLiteral(" (copy)"));
    return create(copy);
}

QVariantMap ExerciseLibraryBridge::clone(const QString &key)
{
    if (!services_->isReady())
        return result(QStringLiteral("failed"));
    QString newKey;
    const StoreStatus status = services_->exercises()->clone(key, &newKey);
    QVariantMap map = result(statusName(status));
    map.insert(QStringLiteral("key"), newKey);
    if (status == StoreStatus::Ok) {
        map.insert(QStringLiteral("name"), get(newKey).value(QStringLiteral("name")));
        refresh();
    }
    return map;
}

QStringList ExerciseLibraryBridge::workoutsUsing(const QString &key)
{
    if (!services_->isReady())
        return QStringList();
    return services_->exercises()->workoutsUsing(key);
}

QVariantMap ExerciseLibraryBridge::remove(const QString &key)
{
    if (!services_->isReady())
        return result(QStringLiteral("failed"));
    const StoreStatus status = services_->exercises()->remove(key);
    if (status == StoreStatus::Ok)
        refresh();
    return result(statusName(status));
}

QVariantMap ExerciseLibraryBridge::exportExercise(const QString &key)
{
    Exercise e;
    if (!services_->isReady() || !services_->exercises()->get(key, &e))
        return result(QStringLiteral("not-found"));
    const QString folder = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                               .filePath(QStringLiteral("WorkoutTrainer"));
    if (!QDir().mkpath(folder))
        return result(QStringLiteral("failed"));
    const QString base = ExerciseExchange::fileBaseName(e.name);
    QString path = QDir(folder).filePath(base + QStringLiteral(".json"));
    for (int n = 2; QFileInfo::exists(path) && n < 1000; ++n)
        path = QDir(folder).filePath(base + QLatin1Char('-') + QString::number(n) + QStringLiteral(".json"));
    if (QFileInfo::exists(path) || !Exchange::writeFile(path, ExerciseExchange::toJson(e)))
        return result(QStringLiteral("failed"));
    QVariantMap map = result(QStringLiteral("ok"));
    map.insert(QStringLiteral("path"), path);
    map.insert(QStringLiteral("folder"), folder);
    return map;
}

QVariantMap ExerciseLibraryBridge::importExercise(const QString &path)
{
    if (!services_->isReady())
        return result(QStringLiteral("failed"));
    QVariantMap map = result(QStringLiteral("invalid"));
    QStringList problems;
    QStringList notices;
    // The size is checked before anything is read; one byte more than the limit is read to tell.
    QFile file(path);
    QByteArray bytes;
    if (!QFileInfo(path).isFile() || !file.open(QIODevice::ReadOnly)) {
        problems << QStringLiteral("unreadable");
    } else if (file.size() > ExerciseExchange::maxFileBytes) {
        problems << QStringLiteral("too-large");
    } else {
        bytes = file.read(ExerciseExchange::maxFileBytes + 1);
        const ExerciseExchange::Parsed parsed = ExerciseExchange::parse(bytes);
        for (const ImportIssue &issue : parsed.problems)
            problems << ExerciseExchange::issueCode(issue.code);
        for (const ImportIssue &issue : parsed.notices)
            notices << ExerciseExchange::issueCode(issue.code);
        if (parsed.valid) {
            Exercise e = parsed.exercise;
            e.language = parsed.exercise.language;
            QList<ExerciseProblem> found;
            const StoreStatus status = services_->exercises()->create(&e, &found);
            if (status == StoreStatus::Ok) {
                map = result(QStringLiteral("ok"));
                map.insert(QStringLiteral("key"), e.key);
                map.insert(QStringLiteral("name"), e.name);
                refresh();
            } else {
                map = result(statusName(status));
                problems << QStringLiteral("invalid");
            }
        }
    }
    map.insert(QStringLiteral("problems"), problems);
    map.insert(QStringLiteral("notices"), notices);
    return map;
}

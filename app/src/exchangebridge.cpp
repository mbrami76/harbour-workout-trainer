// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "exchangebridge.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include "musclegroups.h"
#include "services.h"

namespace {

QVariantMap issueMap(const ImportIssue &issue)
{
    QVariantMap map;
    map.insert(QStringLiteral("code"), Exchange::codeName(issue.code));
    map.insert(QStringLiteral("location"), issue.location);
    map.insert(QStringLiteral("count"), issue.count);
    return map;
}

QVariantList issueList(const QList<ImportIssue> &issues)
{
    QVariantList list;
    for (const ImportIssue &issue : issues)
        list.append(issueMap(issue));
    return list;
}

QString shortText(const QString &text)
{
    const int limit = 100;
    return text.size() <= limit ? text : text.left(limit) + QChar(0x2026);
}

QVariantMap exerciseMap(const Exercise &e, const QString &metronomeName)
{
    QVariantMap map;
    map.insert(QStringLiteral("name"), e.name);
    map.insert(QStringLiteral("muscles"), MuscleKeys::keysOf(MuscleKeys::convertRetired(e.muscles)));
    map.insert(QStringLiteral("instructions"), shortText(e.instructions));
    map.insert(QStringLiteral("tips"), shortText(e.tips));
    map.insert(QStringLiteral("icon"), e.icon);
    map.insert(QStringLiteral("colour"), e.colour);
    map.insert(QStringLiteral("language"), e.language);
    map.insert(QStringLiteral("metronome"), metronomeName);
    return map;
}

QStringList differingFields(const Exercise &a, const Exercise &b, const QString &metronomeA, const QString &metronomeB)
{
    QStringList fields;
    if (a.name != b.name)
        fields << QStringLiteral("name");
    if (MuscleKeys::convertRetired(a.muscles) != MuscleKeys::convertRetired(b.muscles))
        fields << QStringLiteral("muscles");
    if (a.instructions != b.instructions)
        fields << QStringLiteral("instructions");
    if (a.tips != b.tips)
        fields << QStringLiteral("tips");
    if (a.icon != b.icon)
        fields << QStringLiteral("icon");
    if (a.colour != b.colour)
        fields << QStringLiteral("colour");
    if (a.language != b.language)
        fields << QStringLiteral("language");
    if (metronomeA != metronomeB)
        fields << QStringLiteral("metronome");
    return fields;
}

QVariantMap countsOf(const Exchange::ImportPreview &p)
{
    QVariantMap counts;
    counts.insert(QStringLiteral("workouts"), p.workoutsToAdd);
    QStringList names;
    for (const Workout &w : p.workouts)
        names << w.name;
    counts.insert(QStringLiteral("workoutNames"), names);
    counts.insert(QStringLiteral("exercisesToAdd"), p.exercisesToAdd);
    counts.insert(QStringLiteral("exercisesReused"), p.exercisesReused);
    counts.insert(QStringLiteral("metronomesToAdd"), p.metronomesToAdd);
    counts.insert(QStringLiteral("metronomesReused"), p.metronomesReused);
    counts.insert(QStringLiteral("sameNameExercises"), p.sameNameExercises);
    return counts;
}

QVariantMap resultCounts(const Exchange::ImportResult &r)
{
    QVariantMap counts;
    counts.insert(QStringLiteral("workoutsAdded"), r.workoutsAdded);
    counts.insert(QStringLiteral("exercisesAdded"), r.exercisesAdded);
    counts.insert(QStringLiteral("exercisesReused"), r.exercisesReused);
    counts.insert(QStringLiteral("exercisesKept"), r.exercisesKept);
    counts.insert(QStringLiteral("exercisesReplaced"), r.exercisesReplaced);
    counts.insert(QStringLiteral("exercisesImportedAsNew"), r.exercisesImportedAsNew);
    counts.insert(QStringLiteral("metronomesAdded"), r.metronomesAdded);
    counts.insert(QStringLiteral("metronomesReused"), r.metronomesReused);
    counts.insert(QStringLiteral("workoutsRenamed"), r.workoutsRenamed);
    counts.insert(QStringLiteral("metronomesRenamed"), r.metronomesRenamed);
    return counts;
}

QVariantMap status(const QString &name)
{
    QVariantMap map;
    map.insert(QStringLiteral("status"), name);
    return map;
}

}

ExchangeBridge::ExchangeBridge(Services *services, QObject *parent)
    : QObject(parent), services_(services)
{
}

QVariantMap ExchangeBridge::writeExport(const QByteArray &bytes, const QString &baseName, int count)
{
    const QString folder = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                               .filePath(QStringLiteral("WorkoutTrainer"));
    if (!QDir().mkpath(folder))
        return status(QStringLiteral("failed"));
    const QString file = Exchange::freeFileName(folder, baseName);
    if (file.isEmpty())
        return status(QStringLiteral("failed"));
    const QString path = QDir(folder).filePath(file);
    if (!Exchange::writeFile(path, bytes))
        return status(QStringLiteral("failed"));
    QVariantMap map = status(QStringLiteral("ok"));
    map.insert(QStringLiteral("folder"), folder);
    map.insert(QStringLiteral("file"), file);
    map.insert(QStringLiteral("count"), count);
    return map;
}

QVariantMap ExchangeBridge::exportWorkout(int id)
{
    Workout w;
    if (!services_->isReady() || !services_->workouts()->get(id, &w))
        return status(QStringLiteral("not-found"));
    QByteArray bytes;
    if (!Exchange::exportWorkouts(services_->database(), QList<int>() << id, &bytes))
        return status(QStringLiteral("failed"));
    return writeExport(bytes, Exchange::workoutFileBase(w.name), 1);
}

QVariantMap ExchangeBridge::exportAll()
{
    if (!services_->isReady())
        return status(QStringLiteral("failed"));
    const int count = services_->workouts()->list().size();
    if (count == 0)
        return status(QStringLiteral("empty"));
    QByteArray bytes;
    if (!Exchange::exportAll(services_->database(), &bytes))
        return status(QStringLiteral("failed"));
    return writeExport(bytes, QStringLiteral("workout-trainer-workouts"), count);
}

QVariantMap ExchangeBridge::previewFile(const QString &path)
{
    cancelImport();
    if (!services_->isReady())
        return status(QStringLiteral("unreadable"));
    // The size is checked before anything is read; one byte more than the limit is read to tell.
    QFile file(path);
    if (!QFileInfo(path).isFile() || !file.open(QIODevice::ReadOnly))
        return status(QStringLiteral("unreadable"));
    if (file.size() > Exchange::maxFileBytes)
        return status(QStringLiteral("too-large"));
    const QByteArray bytes = file.read(Exchange::maxFileBytes + 1);

    const Exchange::ImportPreview p = Exchange::preview(bytes, services_->database());
    QVariantMap map = status(p.valid ? QStringLiteral("ok") : QStringLiteral("rejected"));
    map.insert(QStringLiteral("problems"), issueList(p.problems));
    map.insert(QStringLiteral("notices"), issueList(p.notices));
    map.insert(QStringLiteral("nothingToImport"), p.nothingToImport);
    if (!p.valid)
        return map;

    map.insert(QStringLiteral("counts"), countsOf(p));
    MetronomeStore *metronomes = services_->metronomes();
    QVariantList conflicts;
    for (const Exchange::ImportConflict &c : p.conflicts) {
        QString existingMetronome;
        SavedMetronome stored;
        if (c.existing.metronomeId > 0 && metronomes->get(c.existing.metronomeId, &stored))
            existingMetronome = stored.name;
        QString incomingMetronome;
        for (const SavedMetronome &m : p.metronomes) {
            if (m.key == c.incoming.metronomeKey)
                incomingMetronome = m.name;
        }
        if (incomingMetronome.isEmpty() && !c.incoming.metronomeKey.isEmpty() && metronomes->getByKey(c.incoming.metronomeKey, &stored))
            incomingMetronome = stored.name;
        QVariantMap conflict;
        conflict.insert(QStringLiteral("key"), c.incoming.key);
        conflict.insert(QStringLiteral("existing"), exerciseMap(c.existing, existingMetronome));
        conflict.insert(QStringLiteral("incoming"), exerciseMap(c.incoming, incomingMetronome));
        conflict.insert(QStringLiteral("differs"),
                        differingFields(c.existing, c.incoming, existingMetronome, incomingMetronome));
        conflict.insert(QStringLiteral("workouts"), c.workouts);
        conflicts.append(conflict);
    }
    map.insert(QStringLiteral("conflicts"), conflicts);

    preview_ = p;
    emit previewChanged();
    return map;
}

QVariantMap ExchangeBridge::applyImport(const QVariantMap &choices)
{
    if (!preview_.valid || !services_->isReady())
        return status(QStringLiteral("invalid"));
    Exchange::ImportChoices chosen;
    for (auto it = choices.constBegin(); it != choices.constEnd(); ++it) {
        const QString value = it.value().toString();
        if (value == QLatin1String("replace"))
            chosen.insert(it.key(), Exchange::ConflictChoice::Replace);
        else if (value == QLatin1String("new"))
            chosen.insert(it.key(), Exchange::ConflictChoice::ImportAsNew);
        else if (value == QLatin1String("keep"))
            chosen.insert(it.key(), Exchange::ConflictChoice::Keep);
    }
    const Exchange::ImportResult r = Exchange::apply(preview_, chosen, services_->database());
    if (r.status != StoreStatus::Ok)
        return status(r.status == StoreStatus::Invalid ? QStringLiteral("invalid") : QStringLiteral("failed"));
    QVariantMap map = status(QStringLiteral("ok"));
    map.insert(QStringLiteral("counts"), resultCounts(r));
    map.insert(QStringLiteral("notices"), issueList(r.notices));
    cancelImport();
    emit dataImported();
    return map;
}

void ExchangeBridge::cancelImport()
{
    if (!preview_.valid)
        return;
    preview_ = Exchange::ImportPreview();
    emit previewChanged();
}

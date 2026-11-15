// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef EXCHANGEBRIDGE_H
#define EXCHANGEBRIDGE_H

#include <QObject>
#include <QVariantMap>

#include "exchange.h"

class Services;

// Export and import of workouts (R-IMP-1 to R-IMP-3) for QML. Exposed as the context property
// "exchangeBridge", created in main() with the Services. All store and file calls run on the main
// thread. A result map has `status`; every finding is a fixed code that a screen turns into text,
// together with its place in the file (the place never contains text the user wrote).
//
// An import has two steps. previewFile() reads the chosen file, changes nothing and keeps the
// parsed content; applyImport() writes it in one transaction with the user's choices;
// cancelImport() drops it. A second previewFile() replaces the kept content.
class ExchangeBridge : public QObject
{
    Q_OBJECT
    // A preview is kept and can be applied.
    Q_PROPERTY(bool hasPreview READ hasPreview NOTIFY previewChanged)

public:
    explicit ExchangeBridge(Services *services, QObject *parent = nullptr);

    bool hasPreview() const { return preview_.valid; }

    // Writes one workout, or all, to a new file in the folder WorkoutTrainer of the Documents folder:
    // {status: "ok", "not-found", "empty" or "failed", folder, file, count}. `folder` is the path of
    // the folder and `file` the name of the file only. A name that is taken gets a number; an
    // existing file is never replaced.
    Q_INVOKABLE QVariantMap exportWorkout(int id);
    Q_INVOKABLE QVariantMap exportAll();

    // Reads the file and keeps the result. {status: "ok", "rejected", "too-large" or "unreadable",
    // problems, notices (each {code, location, count}), counts (workouts, workoutNames,
    // exercisesToAdd, exercisesReused, metronomesToAdd, metronomesReused, sameNameExercises),
    // nothingToImport, conflicts}. A conflict is {key, existing, incoming,
    // differs, workouts}: the two exercises as short maps {name, muscles, instructions, tips, icon,
    // colour, language, metronome}, the names of the fields that differ and the names of the
    // workouts that use the existing exercise.
    Q_INVOKABLE QVariantMap previewFile(const QString &path);
    // Applies the kept preview. choices maps the key of a conflicting exercise to "keep",
    // "replace" or "new"; a conflict without an entry is kept. {status: "ok", "invalid" or
    // "failed", counts (workoutsAdded, exercisesAdded, exercisesReused, exercisesKept,
    // exercisesReplaced, exercisesImportedAsNew, metronomesAdded, metronomesReused,
    // workoutsRenamed, metronomesRenamed)}. The kept preview is dropped.
    Q_INVOKABLE QVariantMap applyImport(const QVariantMap &choices);
    Q_INVOKABLE void cancelImport();

signals:
    void previewChanged();
    // The import wrote workouts or exercises: the lists should be read again.
    void dataImported();

private:
    QVariantMap writeExport(const QByteArray &bytes, const QString &baseName, int count);

    Services *services_;
    Exchange::ImportPreview preview_;
};

#endif

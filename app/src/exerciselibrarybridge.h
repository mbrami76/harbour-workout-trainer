// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef EXERCISELIBRARYBRIDGE_H
#define EXERCISELIBRARYBRIDGE_H

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "model.h"

class Services;

// The exercise library (R-EXR-1, R-EXR-4, R-EXR-6, R-EXR-8 to R-EXR-10) for QML. Exposed as the
// context property "exerciseLibrary", created in main() with the Services. All store calls run on
// the main thread. The rules stay in the core: a screen asks problems() for the codes before it
// enables Save, and the store has the last word when it saves.
//
// An exercise is a map as in ExerciseForm: key, name, muscles (group keys), instructions, tips,
// icon, colour, language, starter, metronomeId. A result map has `status`: "ok", "invalid" (see
// `problems`, a list of codes), "not-found" or "failed".
class ExerciseLibraryBridge : public QObject
{
    Q_OBJECT
    // The exercises that match the search text and the group filter, by name without regard to
    // case: [{key, name, icon, colour, muscles, hasTempo}].
    Q_PROPERTY(QVariantList exercises READ exercises NOTIFY exercisesChanged)
    // Number of exercises in the library, whatever the search and the filter.
    Q_PROPERTY(int totalCount READ totalCount NOTIFY exercisesChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    // Key of the group the list is limited to; empty for all.
    Q_PROPERTY(QString groupFilter READ groupFilter WRITE setGroupFilter NOTIFY groupFilterChanged)
    // The keys of the active muscle groups in position order (R-MUS-6), the colour keys of the
    // palette and the icon keys to offer (default first).
    Q_PROPERTY(QStringList muscleKeys READ muscleKeys CONSTANT)
    Q_PROPERTY(QStringList colours READ colours CONSTANT)
    Q_PROPERTY(QStringList icons READ icons CONSTANT)

public:
    explicit ExerciseLibraryBridge(Services *services, QObject *parent = nullptr);

    QVariantList exercises() const { return shown_; }
    int totalCount() const { return total_; }
    QString searchText() const { return search_; }
    void setSearchText(const QString &text);
    QString groupFilter() const { return group_; }
    void setGroupFilter(const QString &key);
    QStringList muscleKeys() const;
    QStringList colours() const;
    QStringList icons() const;

    // Reads the list again.
    Q_INVOKABLE void refresh();
    // The tag a new exercise gets: the chosen language of the interface.
    Q_INVOKABLE QString defaultLanguage() const;
    // The tags the editor offers: the shipped ones, then the other tags in the library.
    Q_INVOKABLE QStringList languages();
    // One exercise as a map; an empty map when it does not exist.
    Q_INVOKABLE QVariantMap get(const QString &key);
    // The codes of what would refuse a save of the map, nothing written.
    Q_INVOKABLE QStringList problems(const QVariantMap &exercise);
    // Saves a new exercise: {status, key, problems}. The language defaults to the one of the
    // interface when the map has none.
    Q_INVOKABLE QVariantMap create(const QVariantMap &exercise);
    // Saves the map over the exercise of its key, with the metronome link it carries:
    // {status, problems}.
    Q_INVOKABLE QVariantMap update(const QVariantMap &exercise);
    // What saving the map would change: {fields (names), workouts (names)}. The caller warns when
    // both are not empty (R-EXR-10).
    Q_INVOKABLE QVariantMap impact(const QVariantMap &exercise);
    // Saves the map as a new exercise whose name gets the suffix " (copy)": {status, key, problems}.
    Q_INVOKABLE QVariantMap saveAsCopy(const QVariantMap &exercise);
    // An independent copy (R-EXR-9): {status, key, name}.
    Q_INVOKABLE QVariantMap clone(const QString &key);
    // Writes the exercise, without its tempo, to a file in the folder WorkoutTrainer of the Documents
    // folder (R-EXR-13): {status, path, folder}. The file name comes from the name of the exercise
    // with unsafe characters left out; an existing file is never replaced.
    Q_INVOKABLE QVariantMap exportExercise(const QString &key);
    // Reads an exercise file (R-EXR-13) and adds the exercise as a new one, without a tempo:
    // {status, key, name, problems, notices}, the last two lists of fixed codes ("too-large",
    // "not-json", "wrong-format", "name-empty", ...). Nothing is added unless the whole file is valid.
    Q_INVOKABLE QVariantMap importExercise(const QString &path);
    // The names of the workouts that use the exercise, sorted.
    Q_INVOKABLE QStringList workoutsUsing(const QString &key);
    // Deletes the exercise, never its metronome: {status}.
    Q_INVOKABLE QVariantMap remove(const QString &key);

signals:
    void exercisesChanged();
    void searchTextChanged();
    void groupFilterChanged();

private:
    QVariantMap result(const QString &status) const;
    void rebuild();

    Services *services_;
    QVariantList shown_;
    QString search_;
    QString group_;
    int total_ = 0;
    // The exercises as read by the last refresh, so that a keystroke in the search does not read
    // the database again.
    QList<Exercise> all_;
};

#endif

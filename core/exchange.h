// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef EXCHANGE_H
#define EXCHANGE_H

#include <QByteArray>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

#include "database.h"
#include "metronomestore.h"
#include "model.h"

// JSON export and import of workouts (R-IMP-1, R-IMP-2) and the parsing shared with the starter
// loader (R-EXR-5). The file format is public (R-IMP-3).

// One finding of an import. The location names the place in the file, such as
// "workouts[2].blocks[0].steps[1].reps"; it never contains text the user wrote (R-GEN-2).
struct ImportIssue {
    enum Code {
        // Problems: the whole file is rejected.
        TooLarge,
        NotUtf8,
        NotJson,
        WrongType,
        WrongFormat,
        NewerVersion,
        UnsupportedVersion,
        Missing,
        OutOfRange,
        TooLong,
        TooMany,
        NotAllowed,
        KeyInvalid,
        KeyPrefix,
        DuplicateKey,
        NameEmpty,
        NoMuscles,
        IconInvalid,
        LanguageInvalid,
        TempoInvalid,
        UnknownExercise,
        UnknownMetronome,
        FileUnreadable,
        // Notices: the file is still imported.
        UnknownColour,
        UnknownGroup,
        ExerciseSkipped,
        WorkoutSkipped,
        SameName,
        NothingToImport
    };

    Code code = WrongType;
    QString location;
    // For SameName: the number of exercises concerned.
    int count = 0;

    // Fixed English text of the finding, without user data.
    QString text() const;
};

namespace Exchange {

const QString exportFormat = QStringLiteral("workout-trainer-workouts");
const QString starterFormat = QStringLiteral("workout-trainer-starter");
const QString starterWorkoutsFormat = QStringLiteral("workout-trainer-starter-workouts");
// Version of the workout export file and of the starter files. One version is readable until the
// first release: a file of another version is refused with a reason and changes nothing.
const int formatVersion = 4;
const int starterVersion = 1;

// Limits of a file (R-NFR-3).
const qint64 maxFileBytes = 4 * 1024 * 1024;
const int maxWorkouts = 200;
const int maxExercises = 1000;
const int maxMetronomes = 1000;
const int maxKeyChars = 64;

// ---- Export ----

// One JSON document for the given workouts, in the given order, with the exercises they use.
// Fails (false) for an empty list or an unknown id.
bool exportWorkouts(Database *database, const QList<int> &ids, QByteArray *bytes);
// All workouts in the order of their ids. The file carries the metronomes the exercises link.
bool exportAll(Database *database, QByteArray *bytes);
// Writes the bytes with QSaveFile. On any failure an existing file stays as it was and *error
// holds a text.
bool writeFile(const QString &path, const QByteArray &bytes, QString *error = nullptr);

// A safe file name without extension for an export of one workout: letters, digits, hyphen and
// underscore only, at most 40 characters, "workout" when nothing is left. Never a path.
QString workoutFileBase(const QString &name);
// "<base>.json" if no such file is in the folder, else "<base>-2.json", "<base>-3.json", ...
// Empty when 999 are taken. An existing file is never chosen.
QString freeFileName(const QString &folder, const QString &base);

// ---- Import ----

// An exercise of the file whose key exists with different content.
struct ImportConflict {
    Exercise existing;
    Exercise incoming;
    // Workouts that use the existing exercise, sorted (R-EXR-10).
    QStringList workouts;
};

// Result of the first phase. Nothing is written to the database.
struct ImportPreview {
    bool valid = false;
    QList<ImportIssue> problems;   // not empty exactly when the file is rejected
    QList<ImportIssue> notices;
    bool nothingToImport = false;

    int workoutsToAdd = 0;
    int exercisesToAdd = 0;
    int exercisesReused = 0;       // identical content, reused silently
    int metronomesToAdd = 0;
    int metronomesReused = 0;      // same key and identical content: the stored one is linked
    int sameNameExercises = 0;     // new keys whose names exist already; added separately
    QList<ImportConflict> conflicts;

    // The parsed content that apply() writes.
    QList<Exercise> exercises;
    // The metronomes of the file as rhythms; the exercises refer to them by
    // Exercise::metronomeKey.
    QList<SavedMetronome> metronomes;
    QList<Workout> workouts;
};

enum class ConflictChoice {
    Keep,        // the existing exercise stays unchanged (default)
    Replace,     // the existing exercise takes the content of the file
    ImportAsNew  // the file's exercise is added under a new key and a changed name
};
// By key of the conflicting exercise; a conflict without an entry is kept.
typedef QHash<QString, ConflictChoice> ImportChoices;

struct ImportResult {
    StoreStatus status = StoreStatus::Failed;
    int workoutsAdded = 0;
    int exercisesAdded = 0;
    int exercisesReused = 0;
    int metronomesAdded = 0;
    int metronomesReused = 0;
    int exercisesKept = 0;
    int exercisesReplaced = 0;
    int exercisesImportedAsNew = 0;
    // Workouts and metronomes whose name got the suffix " (imported)" because their identity was taken.
    int workoutsRenamed = 0;
    int metronomesRenamed = 0;
    QList<ImportIssue> notices;
};

ImportPreview preview(const QByteArray &bytes, Database *database);
// One transaction: all or nothing. Invalid for a preview that is not valid or a choice that does
// not belong to a conflict of the preview; nothing is written then.
ImportResult apply(const ImportPreview &preview, const ImportChoices &choices, Database *database);

// A fixed code for a finding, such as "too-large" or "unknown-exercise", for a screen to turn into text.
QString codeName(ImportIssue::Code code);

// ---- Shared with the starter loader ----

// Checks size, UTF-8 and JSON, the format name and the version. Returns true with the top-level
// object in *root, or false with the problems.
// The version must be exactly supportedVersion: a newer or an older file is refused.
bool parseRoot(const QByteArray &bytes, const QString &format, int supportedVersion, QJsonObject *root,
               QList<ImportIssue> *problems);

struct ExerciseList {
    QList<Exercise> exercises;   // kept, in file order
    QList<int> indices;          // position in the file of each kept exercise
    QList<ImportIssue> problems;
    QList<ImportIssue> notices;
    QStringList skippedKeys;     // exercises with only unknown groups
    int skipped = 0;
};

// Parses one workout object of a file and checks it by the rules of the model, without looking
// any exercise up. Returns true when it has no problem of its own; the findings are appended to
// *problems and *notices.
bool checkWorkout(const QJsonValue &value, const QString &path, Workout *out, QList<ImportIssue> *problems,
                  QList<ImportIssue> *notices);

// Parses and validates the metronome objects of an array at the given path by the rules of the
// export file. Only the valid ones are appended to *out; findings go to *problems and *notices.
void parseMetronomeList(const QJsonArray &array, const QString &path, QList<SavedMetronome> *out,
                        QList<ImportIssue> *problems, QList<ImportIssue> *notices);

// Parses and validates the exercise objects of an array at the given path. A non-empty key
// prefix is required of every key.
void parseExercises(const QJsonArray &array, const QString &path, const QString &keyPrefix, ExerciseList *out);

}

#endif

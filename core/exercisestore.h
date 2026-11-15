// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef EXERCISESTORE_H
#define EXERCISESTORE_H

#include "database.h"
#include "model.h"

enum class ExerciseProblem {
    NameEmpty,
    NameTooLong,
    InstructionsTooLong,
    TipsTooLong,
    NoMuscles,
    UnknownMuscles,
    IconInvalid,
    ColourUnknown,
    LanguageInvalid
};

enum class ExerciseField { Name, Muscles, Instructions, Tips, Icon, Colour, Language, Metronome };

// What saving an edited exercise would do (R-EXR-10): the fields that differ from the stored
// exercise and the workouts that use it. The caller warns when both lists are not empty.
struct ExerciseChange {
    QList<ExerciseField> fields;
    QStringList workouts;
};

// Exercise library in the database (R-EXR-1, R-EXR-2, R-EXR-4, R-EXR-6, R-EXR-8 to R-EXR-10).
// Names are free: nothing here enforces uniqueness.
class ExerciseStore
{
public:
    explicit ExerciseStore(Database *database) : db_(database) {}

    static QList<ExerciseProblem> validate(const Exercise &exercise);
    // True for a well-formed language tag (R-EXR-12).
    static bool isValidLanguage(const QString &tag);
    // The distinct language tags of the stored exercises, sorted (R-EXR-12).
    QStringList languages();

    // Adds a user exercise and sets its generated key. The starter flag is cleared.
    StoreStatus create(Exercise *exercise, QList<ExerciseProblem> *problems = nullptr);
    // Adds an exercise under the key it carries (import, R-IMP-2). Fails when the key exists. The
    // starter flag is cleared.
    StoreStatus createKeyed(const Exercise &exercise, QList<ExerciseProblem> *problems = nullptr);
    bool get(const QString &key, Exercise *exercise);
    // All exercises, sorted by name without regard to case.
    QList<Exercise> list();
    // Replaces name, muscles, instructions, tips, icon, colour, language and metronome link; the key
    // and the starter flag stay. A metronome id that does not exist is stored as no link (R-EXR-10);
    // create() and createKeyed() treat the id the same way.
    StoreStatus update(const Exercise &exercise, QList<ExerciseProblem> *problems = nullptr);
    // Links the metronome to the exercise, or removes the link, and changes nothing else
    // (R-EXR-10). setMetronome() is Invalid, with nothing written, for a metronome that does not
    // exist; clearMetronome() on an exercise without a link is Ok. Both are NotFound for an
    // unknown exercise.
    StoreStatus setMetronome(const QString &key, int metronomeId);
    StoreStatus clearMetronome(const QString &key);
    // Independent copy (R-EXR-9): new generated key, same muscles, texts, icon, colour, language and
    // metronome link, starter flag cleared. The name gets the suffix " (copy)", " (copy 2)", ... until no exercise has that
    // name; a long name is shortened to stay within the limit. Workouts are not touched.
    StoreStatus clone(const QString &key, QString *newKey = nullptr);
    // Compares the edited exercise with the stored one of the same key. Reads only.
    StoreStatus describeChange(const Exercise &edited, ExerciseChange *change);
    // The tempos of the exercises that have a metronome, by exercise key: the snapshot that plans,
    // planned times and engines take (R-WRK-9, R-WRK-10). A metronome whose pace cannot be read
    // gives no entry.
    ExerciseTempos tempoMap();
    // Names of the workouts that use the exercise, sorted.
    QStringList workoutsUsing(const QString &key);
    // Deletes the exercise and the steps that use it, and the blocks left without steps.
    // Workouts left without blocks stay (they fail validation until edited).
    StoreStatus remove(const QString &key);

    // Starter rule (R-EXR-6, R-EXR-15): inserts the exercise, with its starterId and the flag that
    // it follows the language, only if the identifier was never offered before, even when the
    // entry was deleted or edited since. A name that another exercise has gets the suffix
    // " (starter)", " (starter 2)", ... If an entry with the key exists already, it stays
    // unchanged and the identifier is recorded as offered. *inserted tells whether a row was added.
    StoreStatus offerStarter(const Exercise &exercise, bool *inserted = nullptr,
                             QList<ExerciseProblem> *problems = nullptr);
    // Replaces name, instructions, tips and language tag of the entry that has the starter
    // identifier and still follows the language; any other entry is left alone. *changed tells
    // whether a row was updated.
    StoreStatus retextStarter(const Exercise &texts, bool *changed = nullptr);

private:
    bool exists(const QString &key);
    bool insert(const Exercise &exercise, bool starter);
    // The id to store: the given one if that metronome exists, else NULL.
    QVariant linkValue(int metronomeId);
    // The name, or the name with the suffix of a starter entry when another exercise has it.
    QString freeName(const QString &name, const QString &excludeKey);
    Database *db_;
};

#endif

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef STARTERLOADER_H
#define STARTERLOADER_H

#include <QByteArray>
#include <QList>
#include <QString>

#include "database.h"
#include "exchange.h"
#include "language.h"

// Loader of the starter library: exercises, rhythms and workouts (R-EXR-5, R-EXR-6, R-EXR-15, R-TPL-2,
// R-I18N-4, R-I18N-5). The library is a structure file and a workouts file that do not depend on a
// language, and one texts file per language. Every entry has an identifier of 8 lower-case
// hexadecimal digits, one sequence per kind.
namespace StarterLoader {

// The key an entry added from the starter files gets in the database and in export files.
QString exerciseKey(const QString &id);
QString rhythmKey(const QString &id);
// True for 8 lower-case hexadecimal digits.
bool isValidId(const QString &id);

struct Files {
    QByteArray structure;
    QByteArray workouts;
    // The texts of the chosen language and the English texts, the fallback. When the chosen
    // language is English, `texts` and `english` are the same file.
    QByteArray texts;
    QByteArray english;
};

struct Result {
    // Ok; Invalid when a file has a problem (nothing changed); Failed when a file cannot be read or
    // the database reports an error (nothing changed).
    StoreStatus status = StoreStatus::Failed;
    // Entries added to the library, by kind.
    int exercisesAdded = 0;
    int rhythmsAdded = 0;
    int workoutsAdded = 0;
    // Entries that follow the language and took the texts of the chosen language.
    int retexted = 0;
    // Findings that leave the files usable: an exercise whose rhythm is unknown, a text for an
    // identifier that is not in the structure, a workout that refers to an unknown exercise.
    QList<ImportIssue> notices;
    // Findings that reject the files.
    QList<ImportIssue> problems;

    bool changed() const { return exercisesAdded > 0 || rhythmsAdded > 0 || workoutsAdded > 0 || retexted > 0; }
};

// Reads the four files from the directory and applies them for the language. The files are checked
// completely first; a file with a problem changes nothing. Then, in one transaction, the entries
// that follow the language take its texts and language tag (English and the tag "en" where the
// language has no text), and the entries not offered before are added (R-EXR-15). Entries that do
// not follow the language, and everything the user made, are not touched.
Result apply(const QString &directory, const QString &language, Database *database);
// The same for file content in memory.
Result applyBytes(const Files &files, const QString &language, Database *database);

}

#endif

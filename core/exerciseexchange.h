// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef EXERCISEEXCHANGE_H
#define EXERCISEEXCHANGE_H

#include <QByteArray>
#include <QList>
#include <QString>

#include "exchange.h"
#include "model.h"

// A file with one exercise (R-EXR-13): name, muscle groups, instructions, tips, icon, colour and
// language. It never carries a key, a metronome or any other field, and a file is treated as
// untrusted text: it is size-limited, must be valid UTF-8 and JSON of the exact shape, every value
// is checked by the rules of the core, control and direction-changing characters are removed from
// the texts, and nothing in it is ever run or used as a path.
namespace ExerciseExchange {

const QString format = QStringLiteral("workout-trainer-exercise");
const int version = 1;
// A file larger than this is refused unread (the texts of an exercise are at most 6000 characters).
const qint64 maxFileBytes = 64 * 1024;

// The file for the exercise: its key and its metronome link are left out.
QByteArray toJson(const Exercise &exercise);

struct Parsed {
    bool valid = false;
    // The exercise to add: no key, no metronome, not a starter. Valid for the store when `valid`.
    Exercise exercise;
    QList<ImportIssue> problems;   // not empty exactly when the file is refused
    QList<ImportIssue> notices;    // the file is still imported
};

Parsed parse(const QByteArray &bytes);

// The text without control characters (a line break and a tab are kept when `multiline`, else they
// become a space) and without characters that change the direction of the text.
QString clean(const QString &text, bool multiline);

// A safe file name without extension for an exercise name: letters, digits, hyphen and underscore
// only, at most 40 characters, "exercise" when nothing is left. Never a path.
QString fileBaseName(const QString &name);

// A fixed code for a finding, such as "too-large" or "not-json", for a screen to turn into text.
QString issueCode(ImportIssue::Code code);

}

#endif

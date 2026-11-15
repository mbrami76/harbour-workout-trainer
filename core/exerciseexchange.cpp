// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "exerciseexchange.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "exercisestore.h"
#include "musclegroups.h"

namespace {

bool isDirectionControl(ushort u)
{
    return (u >= 0x202a && u <= 0x202e) || (u >= 0x2066 && u <= 0x2069) || u == 0x200e || u == 0x200f
           || u == 0x061c;
}

ImportIssue makeIssue(ImportIssue::Code code, const QString &location)
{
    ImportIssue i;
    i.code = code;
    i.location = location;
    return i;
}

}

QString ExerciseExchange::clean(const QString &text, bool multiline)
{
    QString out;
    out.reserve(text.size());
    for (const QChar c : text) {
        const ushort u = c.unicode();
        if (u == '\n' || u == '\t') {
            out.append(multiline ? c : QChar(' '));
        } else if (u == '\r') {
            // A line break of a file from another system: dropped, the \n that follows stays.
            if (!multiline)
                out.append(QChar(' '));
        } else if (u < 0x20 || u == 0x7f || (u >= 0x80 && u < 0xa0) || isDirectionControl(u)
                   || u == 0xfeff || u == 0x2028 || u == 0x2029 || u == 0xfffe || u == 0xffff) {
            continue;
        } else {
            out.append(c);
        }
    }
    return out;
}

QString ExerciseExchange::fileBaseName(const QString &name)
{
    QString out;
    for (const QChar c : name) {
        const ushort u = c.unicode();
        const bool ok = (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9') || u == '-' || u == '_';
        if (ok)
            out.append(c);
        else if (!out.isEmpty() && !out.endsWith(QLatin1Char('-')))
            out.append(QLatin1Char('-'));
        if (out.length() >= 40)
            break;
    }
    while (out.endsWith(QLatin1Char('-')))
        out.chop(1);
    return out.isEmpty() ? QStringLiteral("exercise") : out;
}

QString ExerciseExchange::issueCode(ImportIssue::Code code)
{
    switch (code) {
    case ImportIssue::TooLarge: return QStringLiteral("too-large");
    case ImportIssue::NotUtf8: return QStringLiteral("not-utf8");
    case ImportIssue::NotJson: return QStringLiteral("not-json");
    case ImportIssue::WrongFormat: return QStringLiteral("wrong-format");
    case ImportIssue::NewerVersion: return QStringLiteral("newer-version");
    case ImportIssue::UnsupportedVersion: return QStringLiteral("unsupported-version");
    case ImportIssue::NameEmpty: return QStringLiteral("name-empty");
    case ImportIssue::NoMuscles: return QStringLiteral("muscles-none");
    case ImportIssue::TooLong: return QStringLiteral("too-long");
    case ImportIssue::IconInvalid: return QStringLiteral("icon");
    case ImportIssue::LanguageInvalid: return QStringLiteral("language");
    case ImportIssue::UnknownColour: return QStringLiteral("unknown-colour");
    case ImportIssue::UnknownGroup: return QStringLiteral("unknown-group");
    case ImportIssue::FileUnreadable: return QStringLiteral("unreadable");
    default: return QStringLiteral("invalid");
    }
}

QByteArray ExerciseExchange::toJson(const Exercise &e)
{
    QJsonObject o;
    o.insert(QStringLiteral("name"), e.name);
    QJsonArray muscles;
    for (const QString &key : MuscleKeys::keysOf(MuscleKeys::convertRetired(e.muscles)))
        muscles.append(key);
    o.insert(QStringLiteral("muscles"), muscles);
    o.insert(QStringLiteral("instructions"), e.instructions);
    o.insert(QStringLiteral("tips"), e.tips);
    if (!e.icon.isEmpty())
        o.insert(QStringLiteral("icon"), e.icon);
    if (!e.colour.isEmpty())
        o.insert(QStringLiteral("colour"), e.colour);
    o.insert(QStringLiteral("language"), e.language);
    QJsonObject root;
    root.insert(QStringLiteral("format"), format);
    root.insert(QStringLiteral("version"), version);
    root.insert(QStringLiteral("exercise"), o);
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

ExerciseExchange::Parsed ExerciseExchange::parse(const QByteArray &bytes)
{
    Parsed result;
    if (bytes.size() > maxFileBytes) {
        result.problems.append(makeIssue(ImportIssue::TooLarge, QString()));
        return result;
    }
    QJsonObject root;
    // The shared reader checks UTF-8, JSON, the format name and the version.
    if (!Exchange::parseRoot(bytes, format, version, &root, &result.problems))
        return result;
    if (!root.value(QLatin1String("exercise")).isObject()) {
        result.problems.append(makeIssue(ImportIssue::Missing, QStringLiteral("exercise")));
        return result;
    }

    // Only the known fields are carried over, with a key that the shared reader accepts and that
    // is dropped afterwards. A metronome or any other field in the file is ignored.
    const QJsonObject in = root.value(QLatin1String("exercise")).toObject();
    QJsonObject copy;
    copy.insert(QStringLiteral("key"), QStringLiteral("file"));
    const char *const fields[] = { "name", "muscles", "instructions", "tips", "icon", "colour", "language" };
    for (const char *field : fields) {
        if (in.contains(QLatin1String(field)))
            copy.insert(QLatin1String(field), in.value(QLatin1String(field)));
    }
    Exchange::ExerciseList list;
    Exchange::parseExercises(QJsonArray() << copy, QStringLiteral("exercise"), QString(), &list);
    result.problems = list.problems;
    result.notices = list.notices;
    if (!result.problems.isEmpty())
        return result;
    if (list.exercises.isEmpty()) {
        // The only groups of the file are unknown to this version.
        result.problems.append(makeIssue(ImportIssue::NoMuscles, QStringLiteral("exercise.muscles")));
        return result;
    }

    Exercise e = list.exercises.first();
    e.key.clear();
    e.metronomeKey.clear();
    e.metronomeId = 0;
    e.starter = false;
    e.name = clean(e.name, false).trimmed();
    e.instructions = clean(e.instructions, true);
    e.tips = clean(e.tips, true);
    // The rules of the core have the last word, after the texts are clean.
    for (ExerciseProblem problem : ExerciseStore::validate(e)) {
        switch (problem) {
        case ExerciseProblem::NameEmpty: result.problems.append(makeIssue(ImportIssue::NameEmpty, QStringLiteral("exercise.name"))); break;
        case ExerciseProblem::NoMuscles: result.problems.append(makeIssue(ImportIssue::NoMuscles, QStringLiteral("exercise.muscles"))); break;
        case ExerciseProblem::IconInvalid: result.problems.append(makeIssue(ImportIssue::IconInvalid, QStringLiteral("exercise.icon"))); break;
        default: result.problems.append(makeIssue(ImportIssue::TooLong, QStringLiteral("exercise"))); break;
        }
    }
    if (!result.problems.isEmpty())
        return result;
    result.exercise = e;
    result.valid = true;
    return result;
}

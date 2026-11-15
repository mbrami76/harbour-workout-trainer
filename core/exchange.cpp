// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "exchange.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonValue>
#include <QSaveFile>
#include <QSet>
#include <algorithm>
#include <cmath>

#include "colours.h"
#include "identity.h"
#include "metronomestore.h"
#include "exercisestore.h"
#include "musclegroups.h"
#include "workoutlimits.h"
#include "workoutstore.h"

QString ImportIssue::text() const
{
    switch (code) {
    case TooLarge: return QStringLiteral("The file is larger than 4 MiB.");
    case NotUtf8: return QStringLiteral("The file is not valid UTF-8 text.");
    case NotJson: return QStringLiteral("The file is not valid JSON.");
    case WrongType: return QStringLiteral("The value has the wrong type.");
    case WrongFormat: return QStringLiteral("The file is not of the expected format.");
    case NewerVersion: return QStringLiteral("The file was written by a newer version of the app. Update the app to import it.");
    case UnsupportedVersion: return QStringLiteral("The file was written by a version of the app that is no longer supported.");
    case Missing: return QStringLiteral("A required value is missing.");
    case OutOfRange: return QStringLiteral("The value is outside the allowed range.");
    case TooLong: return QStringLiteral("The text is too long.");
    case TooMany: return QStringLiteral("The list has too many entries.");
    case NotAllowed: return QStringLiteral("The value is not allowed here.");
    case KeyInvalid: return QStringLiteral("The key must be 1 to 64 letters, digits, hyphens or underscores.");
    case KeyPrefix: return QStringLiteral("The key of a starter exercise must start with \"starter-\".");
    case DuplicateKey: return QStringLiteral("The key is used by an earlier exercise in the file.");
    case NameEmpty: return QStringLiteral("The name is empty.");
    case NoMuscles: return QStringLiteral("The exercise has no muscle group.");
    case IconInvalid: return QStringLiteral("The icon key must be 1 to 40 lower-case letters, digits or hyphens.");
    case LanguageInvalid: return QStringLiteral("The language tag must be 2 to 35 lower-case letters, digits or hyphens and start with a letter.");
    case TempoInvalid: return QStringLiteral("The pace breaks a rule: one repetition must last 0.5 s to 60 s, and a rhythm has 1 to 8 phases of 0.5 s to 10 s with names of at most 12 characters.");
    case UnknownMetronome: return QStringLiteral("The rhythm is neither in the file nor in the library.");
    case UnknownExercise: return QStringLiteral("The exercise is neither in the file nor in the library.");
    case FileUnreadable: return QStringLiteral("The file cannot be read.");
    case UnknownColour: return QStringLiteral("The colour is unknown and was replaced by the default.");
    case UnknownGroup: return QStringLiteral("The muscle group is unknown to this version and was dropped.");
    case ExerciseSkipped: return QStringLiteral("The exercise has only unknown muscle groups and was skipped.");
    case WorkoutSkipped: return QStringLiteral("The workout uses a skipped exercise and was skipped.");
    case SameName: return QStringLiteral("Some exercises have the name of an existing exercise and were added separately.");
    case NothingToImport: return QStringLiteral("The file contains nothing that can be imported.");
    }
    return QString();
}

namespace {

QString atField(const QString &parent, const char *name)
{
    return parent.isEmpty() ? QString::fromLatin1(name) : parent + QLatin1Char('.') + QString::fromLatin1(name);
}

QString atIndex(const QString &parent, int index)
{
    return parent + QLatin1Char('[') + QString::number(index) + QLatin1Char(']');
}

ImportIssue issue(ImportIssue::Code code, const QString &location, int count = 0)
{
    ImportIssue found;
    found.code = code;
    found.location = location;
    found.count = count;
    return found;
}

// Strict UTF-8 check: no overlong forms, no surrogates, nothing above U+10FFFF.
bool validUtf8(const QByteArray &bytes, int start)
{
    const uchar *p = reinterpret_cast<const uchar *>(bytes.constData());
    const int n = bytes.size();
    int i = start;
    while (i < n) {
        const uchar c = p[i];
        int extra = 0;
        uint code = 0;
        uint minimum = 0;
        if (c < 0x80) {
            ++i;
            continue;
        } else if ((c & 0xe0) == 0xc0) {
            extra = 1; code = c & 0x1f; minimum = 0x80;
        } else if ((c & 0xf0) == 0xe0) {
            extra = 2; code = c & 0x0f; minimum = 0x800;
        } else if ((c & 0xf8) == 0xf0) {
            extra = 3; code = c & 0x07; minimum = 0x10000;
        } else {
            return false;
        }
        if (i + extra >= n)
            return false;
        for (int k = 1; k <= extra; ++k) {
            if ((p[i + k] & 0xc0) != 0x80)
                return false;
            code = (code << 6) | (p[i + k] & 0x3f);
        }
        if (code < minimum || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff))
            return false;
        i += extra + 1;
    }
    return true;
}

// Reads a whole number. False for a non-number, a fraction or a non-finite value; huge values
// are clamped so the range checks reject them.
bool readInt(const QJsonValue &value, int *out)
{
    if (!value.isDouble())
        return false;
    const double d = value.toDouble();
    if (!std::isfinite(d) || d != std::floor(d))
        return false;
    *out = int(qBound(-2000000000.0, d, 2000000000.0));
    return true;
}

// Optional string field. Returns false (and records the problem) when present with another type.
bool readText(const QJsonObject &object, const char *name, const QString &parent, bool required,
              QString *out, QList<ImportIssue> *problems)
{
    const QString here = atField(parent, name);
    if (!object.contains(QLatin1String(name))) {
        if (required)
            problems->append(issue(ImportIssue::Missing, here));
        return !required;
    }
    const QJsonValue value = object.value(QLatin1String(name));
    if (!value.isString()) {
        problems->append(issue(ImportIssue::WrongType, here));
        return false;
    }
    *out = value.toString();
    return true;
}

bool readNumber(const QJsonObject &object, const char *name, const QString &parent, bool required,
                int *out, bool *present, QList<ImportIssue> *problems)
{
    const QString here = atField(parent, name);
    *present = object.contains(QLatin1String(name));
    if (!*present) {
        if (required)
            problems->append(issue(ImportIssue::Missing, here));
        return !required;
    }
    if (!readInt(object.value(QLatin1String(name)), out)) {
        problems->append(issue(ImportIssue::WrongType, here));
        return false;
    }
    return true;
}

// Optional "startOnTap": a boolean, false when absent.
void readStartMode(const QJsonObject &stepObject, const QString &stepPath, Step *step, QList<ImportIssue> *problems)
{
    if (!stepObject.contains(QLatin1String("startOnTap")))
        return;
    const QJsonValue value = stepObject.value(QLatin1String("startOnTap"));
    if (!value.isBool())
        problems->append(issue(ImportIssue::WrongType, atField(stepPath, "startOnTap")));
    else
        step->startOnTap = value.toBool();
}

// Reads the phases of a rhythm from the keys "phases" and "phaseNames" of an object at the given
// path, into a rhythm of one repetition. The rhythm is not checked against the limits here.
bool readRhythm(const QJsonObject &object, const QString &path, Tempo *tempo, QList<ImportIssue> *problems)
{
    const QString phasesPath = atField(path, "phases");
    if (!object.contains(QLatin1String("phases"))) {
        problems->append(issue(ImportIssue::Missing, phasesPath));
        return false;
    }
    if (!object.value(QLatin1String("phases")).isArray()) {
        problems->append(issue(ImportIssue::WrongType, phasesPath));
        return false;
    }
    const QJsonArray array = object.value(QLatin1String("phases")).toArray();
    if (array.size() < Limits::minPhases || array.size() > Limits::maxPhases) {
        problems->append(issue(ImportIssue::OutOfRange, phasesPath));
        return false;
    }
    QList<int> units;
    for (int i = 0; i < array.size(); ++i) {
        int unit = 0;
        if (!readInt(array.at(i), &unit)) {
            problems->append(issue(ImportIssue::WrongType, atIndex(phasesPath, i)));
            return false;
        }
        units.append(unit);
    }
    // Optional names, one per phase.
    QStringList names;
    const QString namesPath = atField(path, "phaseNames");
    if (object.contains(QLatin1String("phaseNames"))) {
        if (!object.value(QLatin1String("phaseNames")).isArray()) {
            problems->append(issue(ImportIssue::WrongType, namesPath));
            return false;
        }
        const QJsonArray nameArray = object.value(QLatin1String("phaseNames")).toArray();
        if (nameArray.size() != array.size()) {
            problems->append(issue(ImportIssue::OutOfRange, namesPath));
            return false;
        }
        for (int i = 0; i < nameArray.size(); ++i) {
            if (!nameArray.at(i).isString()) {
                problems->append(issue(ImportIssue::WrongType, atIndex(namesPath, i)));
                return false;
            }
            const QString name = nameArray.at(i).toString();
            if (Tempo::nameLength(name) > Limits::maxPhaseNameChars) {
                problems->append(issue(ImportIssue::TooLong, atIndex(namesPath, i)));
                return false;
            }
            names.append(name);
        }
    }
    *tempo = Tempo::phases(units, 1, names);
    return true;
}

// Optional "pace" of a step: {"form": "total" | "perRep", "ms": number}.
void readStepPace(const QJsonObject &stepObject, const QString &stepPath, Step *step, QList<ImportIssue> *problems)
{
    if (!stepObject.contains(QLatin1String("pace")))
        return;
    const QString path = atField(stepPath, "pace");
    if (!stepObject.value(QLatin1String("pace")).isObject()) {
        problems->append(issue(ImportIssue::WrongType, path));
        return;
    }
    const QJsonObject object = stepObject.value(QLatin1String("pace")).toObject();
    QString form;
    if (!readText(object, "form", path, true, &form, problems))
        return;
    if (form == QLatin1String("total"))
        step->pace = PaceKind::Total;
    else if (form == QLatin1String("perRep"))
        step->pace = PaceKind::PerRep;
    else {
        problems->append(issue(ImportIssue::OutOfRange, atField(path, "form")));
        return;
    }
    bool present = false;
    readNumber(object, "ms", path, true, &step->paceMs, &present, problems);
}

// Reports what is wrong with a metronome the way the file shows it.
void metronomeIssues(const SavedMetronome &m, const QString &path, QList<ImportIssue> *problems)
{
    for (const MetronomeProblem &p : MetronomeStore::validate(m)) {
        switch (p.code) {
        case MetronomeProblem::NameEmpty: problems->append(issue(ImportIssue::NameEmpty, atField(path, "name"))); break;
        case MetronomeProblem::NameTooLong: problems->append(issue(ImportIssue::TooLong, atField(path, "name"))); break;
        case MetronomeProblem::TempoInvalid:
            problems->append(issue(ImportIssue::TempoInvalid, atField(path, "phases")));
            break;
        case MetronomeProblem::IdentityTaken: break;
        }
    }
}

bool validKey(const QString &key)
{
    if (key.isEmpty() || key.length() > Exchange::maxKeyChars)
        return false;
    for (const QChar c : key) {
        const ushort u = c.unicode();
        const bool ok = (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9') || u == '-' || u == '_';
        if (!ok)
            return false;
    }
    return true;
}

bool validIconKey(const QString &icon)
{
    Exercise probe;
    probe.name = QStringLiteral("x");
    probe.muscles = muscleBit(MuscleGroup::Chest);
    probe.icon = icon;
    return !ExerciseStore::validate(probe).contains(ExerciseProblem::IconInvalid);
}

bool validLanguageTag(const QString &tag)
{
    return ExerciseStore::isValidLanguage(tag);
}

bool sameContent(const Exercise &a, const Exercise &b)
{
    return a.name == b.name && MuscleKeys::convertRetired(a.muscles) == MuscleKeys::convertRetired(b.muscles) && a.instructions == b.instructions && a.tips == b.tips
        && a.icon == b.icon && a.colour == b.colour
        && a.language == b.language;
}

QString suffixed(const QString &base, const QString &word, int n)
{
    const QString suffix = n == 1 ? QStringLiteral(" (%1)").arg(word) : QStringLiteral(" (%1 %2)").arg(word).arg(n);
    return base.left(Limits::maxNameChars - suffix.length()) + suffix;
}

// A name not in `taken` (lower case). Without `always`, the plain name is used when it is free.
QString freeName(const QString &base, const QSet<QString> &taken, bool always)
{
    if (!always && !taken.contains(base.toLower()))
        return base;
    for (int n = 1;; ++n) {
        const QString candidate = suffixed(base, QStringLiteral("imported"), n);
        if (!taken.contains(candidate.toLower()))
            return candidate;
    }
}

// Location of a model finding inside the file.
QString workoutLocation(const Problem &p, const QString &path)
{
    const QString block = p.blockIndex >= 0 ? atIndex(atField(path, "blocks"), p.blockIndex) : path;
    const QString step = p.stepIndex >= 0 ? atIndex(atField(block, "steps"), p.stepIndex) : block;
    switch (p.code) {
    case Problem::NoBlocks:
    case Problem::TooManyBlocks: return atField(path, "blocks");
    case Problem::RestBetweenBlocksOutOfRange: return atField(path, "restBetweenBlocksSec");
    case Problem::EmptyBlock:
    case Problem::TooManySteps: return atField(block, "steps");
    case Problem::RoundsOutOfRange: return atField(block, "rounds");
    case Problem::RestBetweenRoundsOutOfRange: return atField(block, "restBetweenRoundsSec");
    case Problem::MissingExercise:
    case Problem::RestHasExercise: return atField(step, "exercise");
    case Problem::RestNotTimed:
    case Problem::RepsOutOfRange: return atField(step, "reps");
    case Problem::DurationOutOfRange: return atField(step, "durationSec");
    case Problem::PaceWithDuration:
    case Problem::PaceRepTooShort:
    case Problem::PaceRepTooLong: return atField(step, "pace");
    case Problem::DurationWithRhythm: return atField(step, "durationSec");
    case Problem::MissingDurationOrReps:
    case Problem::BothDurationAndReps: return step;
    case Problem::NameEmpty:
    case Problem::NameTooLong:
    case Problem::IdentityTaken:
    case Problem::UnknownExercise: return atField(path, "name");
    case Problem::IconInvalid: return atField(path, "icon");
    case Problem::ColourUnknown: return atField(path, "colour");
    }
    return path;
}

ImportIssue::Code workoutCode(Problem::Code code)
{
    switch (code) {
    case Problem::NoBlocks:
    case Problem::EmptyBlock:
    case Problem::MissingExercise:
    case Problem::MissingDurationOrReps: return ImportIssue::Missing;
    case Problem::TooManyBlocks:
    case Problem::TooManySteps: return ImportIssue::TooMany;
    case Problem::RestHasExercise:
    case Problem::RestNotTimed:
    case Problem::BothDurationAndReps:
    case Problem::PaceWithDuration:
    case Problem::DurationWithRhythm: return ImportIssue::NotAllowed;
    case Problem::PaceRepTooShort:
    case Problem::PaceRepTooLong: return ImportIssue::TempoInvalid;
    case Problem::NameEmpty: return ImportIssue::NameEmpty;
    case Problem::NameTooLong: return ImportIssue::TooLong;
    case Problem::IconInvalid: return ImportIssue::IconInvalid;
    case Problem::ColourUnknown: return ImportIssue::UnknownColour;
    case Problem::IdentityTaken: return ImportIssue::NotAllowed;
    case Problem::UnknownExercise: return ImportIssue::UnknownExercise;
    default: return ImportIssue::OutOfRange;
    }
}

QJsonObject exerciseJson(const Exercise &e, const QString &metronomeKey)
{
    QJsonObject o;
    o.insert(QStringLiteral("key"), e.key);
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
    if (!metronomeKey.isEmpty())
        o.insert(QStringLiteral("metronome"), metronomeKey);
    return o;
}

QJsonObject metronomeJson(const SavedMetronome &m)
{
    QJsonObject o;
    o.insert(QStringLiteral("key"), m.key);
    o.insert(QStringLiteral("name"), m.name);
    QJsonArray phases;
    for (int unit : m.tempo.phaseUnitList())
        phases.append(unit);
    o.insert(QStringLiteral("phases"), phases);
    // The names are written only when there is one, so that a file of unnamed phases stays short.
    bool named = false;
    QJsonArray names;
    for (const QString &name : m.tempo.phaseNameList()) {
        named = named || !name.isEmpty();
        names.append(name);
    }
    if (named)
        o.insert(QStringLiteral("phaseNames"), names);
    return o;
}

bool sameMetronome(const SavedMetronome &a, const SavedMetronome &b)
{
    return a.name == b.name && a.tempo == b.tempo;
}

QJsonObject workoutJson(const Workout &w)
{
    QJsonArray blocks;
    for (const Block &block : w.blocks) {
        QJsonArray steps;
        for (const Step &step : block.steps) {
            QJsonObject s;
            s.insert(QStringLiteral("type"), stepTypeName(step.type));
            if (step.type != StepType::Rest)
                s.insert(QStringLiteral("exercise"), step.exerciseKey);
            if (step.isTimed())
                s.insert(QStringLiteral("durationSec"), step.durationSec);
            else
                s.insert(QStringLiteral("reps"), step.reps);
            if (step.startOnTap)
                s.insert(QStringLiteral("startOnTap"), true);
            if (!step.isTimed() && step.pace != PaceKind::None) {
                QJsonObject pace;
                pace.insert(QStringLiteral("form"), step.pace == PaceKind::Total ? QStringLiteral("total")
                                                                                  : QStringLiteral("perRep"));
                pace.insert(QStringLiteral("ms"), step.paceMs);
                s.insert(QStringLiteral("pace"), pace);
            }
            steps.append(s);
        }
        QJsonObject b;
        b.insert(QStringLiteral("rounds"), block.rounds);
        b.insert(QStringLiteral("restBetweenRoundsSec"), block.restBetweenRoundsSec);
        b.insert(QStringLiteral("steps"), steps);
        blocks.append(b);
    }
    QJsonObject o;
    o.insert(QStringLiteral("name"), w.name);
    o.insert(QStringLiteral("restBetweenBlocksSec"), w.restBetweenBlocksSec);
    if (!w.icon.isEmpty())
        o.insert(QStringLiteral("icon"), w.icon);
    if (!w.colour.isEmpty())
        o.insert(QStringLiteral("colour"), w.colour);
    o.insert(QStringLiteral("blocks"), blocks);
    return o;
}

// Parses one workout object into *out; returns false when it has problems of its own.
bool parseWorkout(const QJsonValue &value, const QString &path, Workout *out, QList<ImportIssue> *problems,
                  QList<ImportIssue> *notices)
{
    if (!value.isObject()) {
        problems->append(issue(ImportIssue::WrongType, path));
        return false;
    }
    const QJsonObject object = value.toObject();
    const int before = problems->size();

    QString name;
    if (readText(object, "name", path, true, &name, problems))
        out->name = name.trimmed();

    bool present = false;
    readNumber(object, "restBetweenBlocksSec", path, false, &out->restBetweenBlocksSec, &present, problems);

    if (readText(object, "icon", path, false, &out->icon, problems) && !validIconKey(out->icon))
        problems->append(issue(ImportIssue::IconInvalid, atField(path, "icon")));
    QString colour;
    if (readText(object, "colour", path, false, &colour, problems)) {
        if (colour.isEmpty() || Colours::isKnown(colour))
            out->colour = colour;
        else
            notices->append(issue(ImportIssue::UnknownColour, atField(path, "colour")));
    }

    const QString blocksPath = atField(path, "blocks");
    if (!object.contains(QLatin1String("blocks"))) {
        problems->append(issue(ImportIssue::Missing, blocksPath));
    } else if (!object.value(QLatin1String("blocks")).isArray()) {
        problems->append(issue(ImportIssue::WrongType, blocksPath));
    } else {
        const QJsonArray blocks = object.value(QLatin1String("blocks")).toArray();
        // The model rejects more than the limit; do not walk a huge list first.
        const int count = qMin(blocks.size(), Limits::maxBlocksPerWorkout + 1);
        for (int b = 0; b < count; ++b) {
            const QString blockPath = atIndex(blocksPath, b);
            if (!blocks.at(b).isObject()) {
                problems->append(issue(ImportIssue::WrongType, blockPath));
                continue;
            }
            const QJsonObject blockObject = blocks.at(b).toObject();
            Block block;
            readNumber(blockObject, "rounds", blockPath, false, &block.rounds, &present, problems);
            readNumber(blockObject, "restBetweenRoundsSec", blockPath, false, &block.restBetweenRoundsSec, &present, problems);
            const QString stepsPath = atField(blockPath, "steps");
            if (!blockObject.contains(QLatin1String("steps"))) {
                problems->append(issue(ImportIssue::Missing, stepsPath));
            } else if (!blockObject.value(QLatin1String("steps")).isArray()) {
                problems->append(issue(ImportIssue::WrongType, stepsPath));
            } else {
                const QJsonArray steps = blockObject.value(QLatin1String("steps")).toArray();
                const int stepCount = qMin(steps.size(), Limits::maxStepsPerBlock + 1);
                for (int s = 0; s < stepCount; ++s) {
                    const QString stepPath = atIndex(stepsPath, s);
                    if (!steps.at(s).isObject()) {
                        problems->append(issue(ImportIssue::WrongType, stepPath));
                        continue;
                    }
                    const QJsonObject stepObject = steps.at(s).toObject();
                    Step step;
                    QString type;
                    if (readText(stepObject, "type", stepPath, true, &type, problems)
                        && !type.isEmpty() && !stepTypeFromName(type, &step.type))
                        problems->append(issue(ImportIssue::OutOfRange, atField(stepPath, "type")));
                    QString exercise;
                    if (readText(stepObject, "exercise", stepPath, false, &exercise, problems))
                        step.exerciseKey = exercise;
                    readNumber(stepObject, "durationSec", stepPath, false, &step.durationSec, &present, problems);
                    readNumber(stepObject, "reps", stepPath, false, &step.reps, &present, problems);
                    readStartMode(stepObject, stepPath, &step, problems);
                    readStepPace(stepObject, stepPath, &step, problems);
                    block.steps.append(step);
                }
                if (steps.size() > stepCount) {
                    // Keep the count above the limit so the model reports it.
                    while (block.steps.size() < steps.size() && block.steps.size() <= Limits::maxStepsPerBlock)
                        block.steps.append(Step());
                }
            }
            out->blocks.append(block);
        }
        if (blocks.size() > count) {
            while (out->blocks.size() < blocks.size() && out->blocks.size() <= Limits::maxBlocksPerWorkout)
                out->blocks.append(Block());
        }
    }
    return problems->size() == before;
}


// Parses the "metronomes" list of a file.
void parseMetronomes(const QJsonArray &array, const QString &path, QList<SavedMetronome> *out,
                     QList<ImportIssue> *problems, QList<ImportIssue> *notices)
{
    if (array.size() > Exchange::maxMetronomes) {
        problems->append(issue(ImportIssue::TooMany, path));
        return;
    }
    QSet<QString> seen;
    for (int i = 0; i < array.size(); ++i) {
        const QString here = atIndex(path, i);
        if (!array.at(i).isObject()) {
            problems->append(issue(ImportIssue::WrongType, here));
            continue;
        }
        const QJsonObject object = array.at(i).toObject();
        const int before = problems->size();
        SavedMetronome m;
        bool present = false;

        QString key;
        if (readText(object, "key", here, true, &key, problems)) {
            if (!validKey(key))
                problems->append(issue(ImportIssue::KeyInvalid, atField(here, "key")));
            else if (seen.contains(key))
                problems->append(issue(ImportIssue::DuplicateKey, atField(here, "key")));
            else
                m.key = key;
            seen.insert(key);
        }
        QString name;
        if (readText(object, "name", here, true, &name, problems))
            m.name = name.trimmed();
        // An icon or a colour on a rhythm (files of older versions) is ignored.
        readRhythm(object, here, &m.tempo, problems);
        if (problems->size() != before)
            continue;
        metronomeIssues(m, here, problems);
        if (problems->size() == before)
            out->append(m);
    }
}
}

void Exchange::parseMetronomeList(const QJsonArray &array, const QString &path, QList<SavedMetronome> *out,
                                  QList<ImportIssue> *problems, QList<ImportIssue> *notices)
{
    parseMetronomes(array, path, out, problems, notices);
}

bool Exchange::parseRoot(const QByteArray &bytes, const QString &format, int supportedVersion, QJsonObject *root,
                         QList<ImportIssue> *problems)
{
    if (bytes.size() > maxFileBytes) {
        problems->append(issue(ImportIssue::TooLarge, QString()));
        return false;
    }
    // A leading UTF-8 byte order mark is accepted and ignored.
    const int start = bytes.startsWith("\xEF\xBB\xBF") ? 3 : 0;
    if (!validUtf8(bytes, start)) {
        problems->append(issue(ImportIssue::NotUtf8, QString()));
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(bytes.mid(start), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        problems->append(issue(ImportIssue::NotJson, QString()));
        return false;
    }
    if (!document.isObject()) {
        problems->append(issue(ImportIssue::WrongType, QString()));
        return false;
    }
    const QJsonObject object = document.object();

    QString name;
    if (!readText(object, "format", QString(), true, &name, problems))
        return false;
    if (name != format) {
        problems->append(issue(ImportIssue::WrongFormat, QStringLiteral("format")));
        return false;
    }
    int version = 0;
    bool present = false;
    if (!readNumber(object, "version", QString(), true, &version, &present, problems))
        return false;
    // One readable version until the first release; a later release adds the older ones it reads.
    if (version > supportedVersion) {
        problems->append(issue(ImportIssue::NewerVersion, QStringLiteral("version")));
        return false;
    }
    if (version < supportedVersion) {
        problems->append(issue(ImportIssue::UnsupportedVersion, QStringLiteral("version")));
        return false;
    }
    *root = object;
    return true;
}

bool Exchange::checkWorkout(const QJsonValue &value, const QString &path, Workout *out, QList<ImportIssue> *problems,
                            QList<ImportIssue> *notices)
{
    const int before = problems->size();
    if (!parseWorkout(value, path, out, problems, notices))
        return false;
    for (const Problem &p : WorkoutStore::validateWorkout(*out))
        problems->append(issue(workoutCode(p.code), workoutLocation(p, path)));
    return problems->size() == before;
}

void Exchange::parseExercises(const QJsonArray &array, const QString &path, const QString &keyPrefix, ExerciseList *out)
{
    if (array.size() > maxExercises) {
        out->problems.append(issue(ImportIssue::TooMany, path));
        return;
    }
    QSet<QString> seen;
    for (int i = 0; i < array.size(); ++i) {
        const QString here = atIndex(path, i);
        if (!array.at(i).isObject()) {
            out->problems.append(issue(ImportIssue::WrongType, here));
            continue;
        }
        const QJsonObject object = array.at(i).toObject();
        const int before = out->problems.size();
        Exercise e;

        QString key;
        if (readText(object, "key", here, true, &key, &out->problems)) {
            if (!validKey(key))
                out->problems.append(issue(ImportIssue::KeyInvalid, atField(here, "key")));
            else if (!keyPrefix.isEmpty() && !key.startsWith(keyPrefix))
                out->problems.append(issue(ImportIssue::KeyPrefix, atField(here, "key")));
            else if (seen.contains(key))
                out->problems.append(issue(ImportIssue::DuplicateKey, atField(here, "key")));
            else
                e.key = key;
            seen.insert(key);
        }

        QString name;
        if (readText(object, "name", here, true, &name, &out->problems)) {
            e.name = name.trimmed();
            if (e.name.isEmpty())
                out->problems.append(issue(ImportIssue::NameEmpty, atField(here, "name")));
            else if (e.name.length() > Limits::maxNameChars)
                out->problems.append(issue(ImportIssue::TooLong, atField(here, "name")));
        }
        if (readText(object, "instructions", here, false, &e.instructions, &out->problems)
            && e.instructions.length() > Limits::maxInstructionsChars)
            out->problems.append(issue(ImportIssue::TooLong, atField(here, "instructions")));
        if (readText(object, "tips", here, false, &e.tips, &out->problems) && e.tips.length() > Limits::maxTipsChars)
            out->problems.append(issue(ImportIssue::TooLong, atField(here, "tips")));
        if (readText(object, "icon", here, false, &e.icon, &out->problems) && !validIconKey(e.icon))
            out->problems.append(issue(ImportIssue::IconInvalid, atField(here, "icon")));

        QString language;
        if (readText(object, "language", here, false, &language, &out->problems)
            && object.contains(QLatin1String("language"))) {
            if (!validLanguageTag(language))
                out->problems.append(issue(ImportIssue::LanguageInvalid, atField(here, "language")));
            else
                e.language = language;
        }

        QString colour;
        if (readText(object, "colour", here, false, &colour, &out->problems)) {
            if (colour.isEmpty() || Colours::isKnown(colour))
                e.colour = colour;
            else
                out->notices.append(issue(ImportIssue::UnknownColour, atField(here, "colour")));
        }

        // Optional key of the metronome that gives the exercise its tempo (R-EXR-10).
        QString metronome;
        if (readText(object, "metronome", here, false, &metronome, &out->problems)
            && object.contains(QLatin1String("metronome"))) {
            if (!validKey(metronome))
                out->problems.append(issue(ImportIssue::KeyInvalid, atField(here, "metronome")));
            else
                e.metronomeKey = metronome;
        }

        const QString musclesPath = atField(here, "muscles");
        bool allUnknown = false;
        if (!object.contains(QLatin1String("muscles"))) {
            out->problems.append(issue(ImportIssue::Missing, musclesPath));
        } else if (!object.value(QLatin1String("muscles")).isArray()) {
            out->problems.append(issue(ImportIssue::WrongType, musclesPath));
        } else {
            const QJsonArray groups = object.value(QLatin1String("muscles")).toArray();
            int unknown = 0;
            QList<ImportIssue> dropped;
            for (int g = 0; g < groups.size(); ++g) {
                const QString groupPath = atIndex(musclesPath, g);
                if (!groups.at(g).isString()) {
                    out->problems.append(issue(ImportIssue::WrongType, groupPath));
                    continue;
                }
                // A retired key stands for the groups that replace it, without a notice.
                const QStringList read = MuscleKeys::replacementKeys(groups.at(g).toString());
                if (read.isEmpty()) {
                    ++unknown;
                    dropped.append(issue(ImportIssue::UnknownGroup, groupPath));
                }
                for (const QString &key : read) {
                    MuscleGroup group;
                    if (MuscleKeys::fromKey(key, &group))
                        e.muscles |= muscleBit(group);
                }
            }
            if (groups.isEmpty()) {
                out->problems.append(issue(ImportIssue::NoMuscles, musclesPath));
            } else if (e.muscles == 0 && unknown > 0 && out->problems.size() == before) {
                allUnknown = true;
            } else {
                out->notices.append(dropped);
            }
        }

        if (out->problems.size() != before)
            continue;
        if (allUnknown) {
            ++out->skipped;
            out->skippedKeys.append(e.key);
            out->notices.append(issue(ImportIssue::ExerciseSkipped, here));
            continue;
        }
        out->exercises.append(e);
        out->indices.append(i);
    }
}

bool Exchange::exportWorkouts(Database *database, const QList<int> &ids, QByteArray *bytes)
{
    if (ids.isEmpty())
        return false;
    WorkoutStore workouts(database);
    ExerciseStore exercises(database);
    MetronomeStore metronomeStore(database);

    QList<Workout> chosen;
    QSet<QString> keys;
    for (int id : ids) {
        Workout w;
        if (!workouts.get(id, &w))
            return false;
        for (const Block &block : w.blocks) {
            for (const Step &step : block.steps) {
                if (!step.exerciseKey.isEmpty())
                    keys.insert(step.exerciseKey);
            }
        }
        chosen.append(w);
    }
    QJsonArray workoutArray;
    for (const Workout &w : chosen)
        workoutArray.append(workoutJson(w));

    QList<Exercise> used;
    for (const QString &key : keys) {
        Exercise e;
        if (!exercises.get(key, &e))
            return false;
        used.append(e);
    }
    std::sort(used.begin(), used.end(), [](const Exercise &a, const Exercise &b) {
        const int byName = Identity::compareNames(a.name, b.name);
        return byName != 0 ? byName < 0 : a.key < b.key;
    });

    // The metronomes the exercises link. A link whose metronome is gone or unreadable is written
    // as no link.
    QHash<int, SavedMetronome> linked;
    QJsonArray exerciseArray;
    for (const Exercise &e : used) {
        SavedMetronome m;
        QString metronomeKey;
        if (e.metronomeId != 0 && metronomeStore.get(e.metronomeId, &m) && m.tempo.isValid()) {
            linked.insert(e.metronomeId, m);
            metronomeKey = m.key;
        }
        exerciseArray.append(exerciseJson(e, metronomeKey));
    }
    QList<SavedMetronome> metronomeList = linked.values();
    std::sort(metronomeList.begin(), metronomeList.end(), [](const SavedMetronome &a, const SavedMetronome &b) {
        const int byName = Identity::compareNames(a.name, b.name);
        return byName != 0 ? byName < 0 : a.key < b.key;
    });
    QJsonArray metronomeArray;
    for (const SavedMetronome &m : metronomeList)
        metronomeArray.append(metronomeJson(m));

    QJsonObject root;
    root.insert(QStringLiteral("format"), exportFormat);
    root.insert(QStringLiteral("version"), formatVersion);
    root.insert(QStringLiteral("exercises"), exerciseArray);
    root.insert(QStringLiteral("metronomes"), metronomeArray);
    root.insert(QStringLiteral("workouts"), workoutArray);
    *bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    return true;
}

bool Exchange::exportAll(Database *database, QByteArray *bytes)
{
    QList<int> ids;
    QSqlQuery q;
    if (!database->run(QStringLiteral("SELECT id FROM workout ORDER BY id"), QVariantList(), &q))
        return false;
    while (q.next())
        ids.append(q.value(0).toInt());
    return exportWorkouts(database, ids, bytes);
}

QString Exchange::workoutFileBase(const QString &name)
{
    QString base;
    for (const QChar c : name) {
        const ushort u = c.unicode();
        if ((u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9') || u == '-' || u == '_')
            base.append(c);
        else if (u == ' ' && !base.isEmpty() && !base.endsWith(QLatin1Char('-')))
            base.append(QLatin1Char('-'));
        if (base.size() >= 40)
            break;
    }
    while (base.endsWith(QLatin1Char('-')))
        base.chop(1);
    return base.isEmpty() ? QStringLiteral("workout") : base;
}

QString Exchange::freeFileName(const QString &folder, const QString &base)
{
    QString file = base + QStringLiteral(".json");
    for (int n = 2; QFileInfo::exists(QDir(folder).filePath(file)); ++n) {
        if (n >= 1000)
            return QString();
        file = base + QLatin1Char('-') + QString::number(n) + QStringLiteral(".json");
    }
    return file;
}

bool Exchange::writeFile(const QString &path, const QByteArray &bytes, QString *error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error)
            *error = QStringLiteral("The file cannot be written.");
        return false;
    }
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        file.cancelWriting();
        if (error)
            *error = QStringLiteral("The file cannot be written.");
        return false;
    }
    return true;
}

namespace {

// Whether an exercise of the file would leave the stored exercise of the same key with the same
// metronome. The file names the metronome by key; the stored one is the same when it has that key
// and, if the file carries the metronome, the same content.
bool sameLink(const Exercise &existing, const Exercise &incoming,
              const QHash<QString, SavedMetronome> &fileMetronomes, MetronomeStore &store)
{
    if (incoming.metronomeKey.isEmpty())
        return existing.metronomeId == 0;
    SavedMetronome stored;
    if (existing.metronomeId == 0 || !store.get(existing.metronomeId, &stored) || stored.key != incoming.metronomeKey)
        return false;
    return !fileMetronomes.contains(incoming.metronomeKey) || sameMetronome(stored, fileMetronomes.value(incoming.metronomeKey));
}

}

QString Exchange::codeName(ImportIssue::Code code)
{
    switch (code) {
    case ImportIssue::TooLarge: return QStringLiteral("too-large");
    case ImportIssue::NotUtf8: return QStringLiteral("not-utf8");
    case ImportIssue::NotJson: return QStringLiteral("not-json");
    case ImportIssue::WrongType: return QStringLiteral("wrong-type");
    case ImportIssue::WrongFormat: return QStringLiteral("wrong-format");
    case ImportIssue::NewerVersion: return QStringLiteral("newer-version");
    case ImportIssue::UnsupportedVersion: return QStringLiteral("unsupported-version");
    case ImportIssue::Missing: return QStringLiteral("missing");
    case ImportIssue::OutOfRange: return QStringLiteral("out-of-range");
    case ImportIssue::TooLong: return QStringLiteral("too-long");
    case ImportIssue::TooMany: return QStringLiteral("too-many");
    case ImportIssue::NotAllowed: return QStringLiteral("not-allowed");
    case ImportIssue::KeyInvalid: return QStringLiteral("key-invalid");
    case ImportIssue::KeyPrefix: return QStringLiteral("key-prefix");
    case ImportIssue::DuplicateKey: return QStringLiteral("duplicate-key");
    case ImportIssue::NameEmpty: return QStringLiteral("name-empty");
    case ImportIssue::NoMuscles: return QStringLiteral("muscles-none");
    case ImportIssue::IconInvalid: return QStringLiteral("icon");
    case ImportIssue::LanguageInvalid: return QStringLiteral("language");
    case ImportIssue::TempoInvalid: return QStringLiteral("tempo");
    case ImportIssue::UnknownExercise: return QStringLiteral("unknown-exercise");
    case ImportIssue::UnknownMetronome: return QStringLiteral("unknown-metronome");
    case ImportIssue::FileUnreadable: return QStringLiteral("unreadable");
    case ImportIssue::UnknownColour: return QStringLiteral("unknown-colour");
    case ImportIssue::UnknownGroup: return QStringLiteral("unknown-group");
    case ImportIssue::ExerciseSkipped: return QStringLiteral("exercise-skipped");
    case ImportIssue::WorkoutSkipped: return QStringLiteral("workout-skipped");
    case ImportIssue::SameName: return QStringLiteral("same-name");
    case ImportIssue::NothingToImport: return QStringLiteral("nothing-to-import");
    }
    return QStringLiteral("invalid");
}

Exchange::ImportPreview Exchange::preview(const QByteArray &bytes, Database *database)
{
    ImportPreview result;
    QJsonObject root;
    if (!parseRoot(bytes, exportFormat, formatVersion, &root, &result.problems))
        return result;
    // Lists of the file.
    QJsonArray exerciseArray;
    QJsonArray workoutArray;
    const QString exercisesPath = QStringLiteral("exercises");
    const QString workoutsPath = QStringLiteral("workouts");
    if (!root.contains(exercisesPath))
        result.problems.append(issue(ImportIssue::Missing, exercisesPath));
    else if (!root.value(exercisesPath).isArray())
        result.problems.append(issue(ImportIssue::WrongType, exercisesPath));
    else
        exerciseArray = root.value(exercisesPath).toArray();
    if (!root.contains(workoutsPath))
        result.problems.append(issue(ImportIssue::Missing, workoutsPath));
    else if (!root.value(workoutsPath).isArray())
        result.problems.append(issue(ImportIssue::WrongType, workoutsPath));
    else
        workoutArray = root.value(workoutsPath).toArray();
    if (workoutArray.size() > maxWorkouts)
        result.problems.append(issue(ImportIssue::TooMany, workoutsPath));
    if (!result.problems.isEmpty())
        return result;

    ExerciseList list;
    parseExercises(exerciseArray, exercisesPath, QString(), &list);
    result.problems.append(list.problems);
    result.notices.append(list.notices);

    // The metronomes the exercises name.
    const QString metronomesPath = QStringLiteral("metronomes");
    QList<SavedMetronome> fileMetronomes;
    if (root.contains(metronomesPath)) {
        if (!root.value(metronomesPath).isArray())
            result.problems.append(issue(ImportIssue::WrongType, metronomesPath));
        else
            parseMetronomes(root.value(metronomesPath).toArray(), metronomesPath, &fileMetronomes,
                            &result.problems, &result.notices);
    }
    QHash<QString, SavedMetronome> fileMetronomeByKey;
    for (const SavedMetronome &m : fileMetronomes)
        fileMetronomeByKey.insert(m.key, m);
    MetronomeStore metronomeStore(database);

    // The metronome of an exercise must be in the file or in the library.
    for (int i = 0; i < list.exercises.size(); ++i) {
        const Exercise &e = list.exercises[i];
        SavedMetronome probe;
        if (!e.metronomeKey.isEmpty() && !fileMetronomeByKey.contains(e.metronomeKey)
            && !metronomeStore.getByKey(e.metronomeKey, &probe))
            result.problems.append(issue(ImportIssue::UnknownMetronome,
                                         atField(atIndex(exercisesPath, list.indices.at(i)), "metronome")));
    }

    QSet<QString> fileKeys;
    for (const Exercise &e : list.exercises)
        fileKeys.insert(e.key);
    const QSet<QString> skippedKeys = QSet<QString>::fromList(list.skippedKeys);

    ExerciseStore exercises(database);
    QHash<QString, bool> inLibrary;

    QList<Workout> workouts;
    QList<ImportIssue> workoutNotices;
    for (int i = 0; i < workoutArray.size(); ++i) {
        const QString path = atIndex(workoutsPath, i);
        Workout w;
        if (!parseWorkout(workoutArray.at(i), path, &w, &result.problems, &workoutNotices))
            continue;

        const int before = result.problems.size();
        const QList<Problem> found = WorkoutStore::validateWorkout(w);
        for (const Problem &p : found)
            result.problems.append(issue(workoutCode(p.code), workoutLocation(p, path)));

        bool skip = false;
        for (int b = 0; b < w.blocks.size(); ++b) {
            for (int s = 0; s < w.blocks.at(b).steps.size(); ++s) {
                const QString &key = w.blocks.at(b).steps.at(s).exerciseKey;
                if (key.isEmpty())
                    continue;
                if (skippedKeys.contains(key)) {
                    skip = true;
                } else if (!fileKeys.contains(key)) {
                    if (!inLibrary.contains(key)) {
                        Exercise probe;
                        inLibrary.insert(key, exercises.get(key, &probe));
                    }
                    if (!inLibrary.value(key)) {
                        const QString stepPath = atIndex(atField(atIndex(atField(path, "blocks"), b), "steps"), s);
                        result.problems.append(issue(ImportIssue::UnknownExercise, atField(stepPath, "exercise")));
                    }
                }
            }
        }
        if (result.problems.size() != before)
            continue;
        if (skip) {
            workoutNotices.append(issue(ImportIssue::WorkoutSkipped, path));
            continue;
        }
        // The first step of a workout always starts automatically; a flag there is ignored.
        normalizeStartModes(w);
        workouts.append(w);
    }
    if (!result.problems.isEmpty()) {
        result.notices.clear();
        return result;
    }
    result.notices.append(workoutNotices);

    // Compare the exercises with the library.
    QStringList libraryNames;
    for (const Exercise &e : exercises.list())
        libraryNames.append(e.name.toLower());
    for (const Exercise &e : list.exercises) {
        Exercise existing;
        if (!exercises.get(e.key, &existing)) {
            ++result.exercisesToAdd;
            if (libraryNames.contains(e.name.toLower()))
                ++result.sameNameExercises;
        } else if (sameContent(existing, e) && sameLink(existing, e, fileMetronomeByKey, metronomeStore)) {
            ++result.exercisesReused;
        } else {
            ImportConflict conflict;
            conflict.existing = existing;
            conflict.incoming = e;
            conflict.workouts = exercises.workoutsUsing(e.key);
            result.conflicts.append(conflict);
        }
    }
    if (result.sameNameExercises > 0)
        result.notices.append(issue(ImportIssue::SameName, exercisesPath, result.sameNameExercises));

    for (const SavedMetronome &m : fileMetronomes) {
        SavedMetronome existing;
        if (metronomeStore.getByKey(m.key, &existing) && sameMetronome(existing, m))
            ++result.metronomesReused;
        else
            ++result.metronomesToAdd;
    }

    result.exercises = list.exercises;
    result.metronomes = fileMetronomes;
    result.workouts = workouts;
    result.workoutsToAdd = workouts.size();
    result.nothingToImport = workouts.isEmpty() && list.exercises.isEmpty() && fileMetronomes.isEmpty();
    if (result.nothingToImport)
        result.notices.append(issue(ImportIssue::NothingToImport, QString()));
    result.valid = true;
    return result;
}

Exchange::ImportResult Exchange::apply(const ImportPreview &preview, const ImportChoices &choices, Database *database)
{
    ImportResult result;
    if (!preview.valid) {
        result.status = StoreStatus::Invalid;
        return result;
    }
    QSet<QString> conflictKeys;
    for (const ImportConflict &c : preview.conflicts)
        conflictKeys.insert(c.incoming.key);
    for (auto it = choices.constBegin(); it != choices.constEnd(); ++it) {
        const int value = int(it.value());
        if (!conflictKeys.contains(it.key()) || value < int(ConflictChoice::Keep) || value > int(ConflictChoice::ImportAsNew)) {
            result.status = StoreStatus::Invalid;
            return result;
        }
    }

    ExerciseStore exercises(database);
    WorkoutStore workouts(database);
    MetronomeStore metronomeStore(database);
    Transaction transaction(database);
    if (!transaction.isActive())
        return result;

    // Metronomes before the exercises that link them. A metronome with a known key and the same
    // content is the same record and is linked, not duplicated. One with a known key and other
    // content is added as a new metronome under a new key, so that the stored one and the
    // exercises that use it stay as they are. A clash on the identity gets the suffix.
    QHash<QString, int> metronomeIds;
    ImportResult counts;
    for (const SavedMetronome &incoming : preview.metronomes) {
        SavedMetronome existing;
        if (metronomeStore.getByKey(incoming.key, &existing)) {
            if (sameMetronome(existing, incoming)) {
                metronomeIds.insert(incoming.key, existing.id);
                ++counts.metronomesReused;
                continue;
            }
        }
        SavedMetronome copy = incoming;
        copy.id = 0;
        copy.createdAt = 0;
        if (metronomeStore.getByKey(copy.key, &existing))
            copy.key.clear();
        const QString plainName = copy.name;
        copy.name = Identity::freeName(database, Identity::Table::Metronome, copy.name, QString(), QString(),
                                       QStringLiteral("imported"), Limits::maxNameChars);
        if (copy.name != plainName)
            ++counts.metronomesRenamed;
        int newId = 0;
        if (metronomeStore.create(copy, &newId) != StoreStatus::Ok)
            return result;
        metronomeIds.insert(incoming.key, newId);
        ++counts.metronomesAdded;
    }
    // The stored id of the metronome an exercise names; 0 for none, -1 for a key nobody has.
    auto linkOf = [&](const QString &key) {
        if (key.isEmpty())
            return 0;
        if (metronomeIds.contains(key))
            return metronomeIds.value(key);
        SavedMetronome stored;
        if (!metronomeStore.getByKey(key, &stored))
            return -1;
        metronomeIds.insert(key, stored.id);
        return stored.id;
    };

    QSet<QString> exerciseNames;
    for (const Exercise &e : exercises.list())
        exerciseNames.insert(e.name.toLower());

    // The library may have changed since the preview: every key is looked up again.
    QHash<QString, QString> keyMap;
    for (const Exercise &incoming : preview.exercises) {
        Exercise existing;
        keyMap.insert(incoming.key, incoming.key);
        Exercise wanted = incoming;
        const int link = linkOf(incoming.metronomeKey);
        if (link < 0) {
            result.status = StoreStatus::Invalid;
            return result;
        }
        wanted.metronomeId = link;
        if (!exercises.get(incoming.key, &existing)) {
            if (exercises.createKeyed(wanted) != StoreStatus::Ok)
                return result;
            exerciseNames.insert(incoming.name.toLower());
            ++counts.exercisesAdded;
            continue;
        }
        if (sameContent(existing, incoming) && existing.metronomeId == wanted.metronomeId) {
            ++counts.exercisesReused;
            continue;
        }
        switch (choices.value(incoming.key, ConflictChoice::Keep)) {
        case ConflictChoice::Keep:
            ++counts.exercisesKept;
            break;
        case ConflictChoice::Replace:
            if (exercises.update(wanted) != StoreStatus::Ok)
                return result;
            ++counts.exercisesReplaced;
            break;
        case ConflictChoice::ImportAsNew: {
            Exercise copy = wanted;
            copy.name = freeName(incoming.name, exerciseNames, true);
            if (exercises.create(&copy) != StoreStatus::Ok)
                return result;
            exerciseNames.insert(copy.name.toLower());
            keyMap.insert(incoming.key, copy.key);
            ++counts.exercisesImportedAsNew;
            break;
        }
        }
    }

    for (Workout w : preview.workouts) {
        for (Block &block : w.blocks) {
            for (Step &step : block.steps) {
                if (keyMap.contains(step.exerciseKey))
                    step.exerciseKey = keyMap.value(step.exerciseKey);
            }
        }
        // A clash on name, icon and colour (R-WRK-12) is resolved by the suffix, never refused.
        const QString plainName = w.name;
        w.name = Identity::freeName(database, Identity::Table::Workout, w.name, w.icon, w.colour,
                                    QStringLiteral("imported"), Limits::maxNameChars);
        if (w.name != plainName)
            ++counts.workoutsRenamed;
        w.favorite = false;
        const StoreStatus status = workouts.create(w);
        if (status != StoreStatus::Ok) {
            result.status = status;
            return result;
        }
        ++counts.workoutsAdded;
    }

    if (!transaction.commit())
        return result;
    counts.status = StoreStatus::Ok;
    counts.notices = preview.notices;
    return counts;
}

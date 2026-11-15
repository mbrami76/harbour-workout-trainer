// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "starterloader.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>

#include "exercisestore.h"
#include "identity.h"
#include "metronomestore.h"
#include "workoutlimits.h"
#include "workoutstore.h"

namespace {

const QString exercisePrefix = QStringLiteral("starter-");
const QString rhythmPrefix = QStringLiteral("starter-m-");

ImportIssue issueAt(ImportIssue::Code code, const QString &location)
{
    ImportIssue i;
    i.code = code;
    i.location = location;
    return i;
}

// One kind of entry of the structure: the identifiers in file order and the objects by identifier.
struct Entries {
    QStringList ids;
    QHash<QString, QJsonObject> objects;
};

// The "exercises", "metronomes" or "workouts" list of a structure file: every object has a valid
// identifier that no other object of the list has.
Entries readList(const QJsonObject &root, const QString &path, bool required, QList<ImportIssue> *problems)
{
    Entries entries;
    if (!root.contains(path)) {
        if (required)
            problems->append(issueAt(ImportIssue::Missing, path));
        return entries;
    }
    if (!root.value(path).isArray()) {
        problems->append(issueAt(ImportIssue::WrongType, path));
        return entries;
    }
    const QJsonArray array = root.value(path).toArray();
    if (array.size() > Exchange::maxWorkouts * 2) {
        problems->append(issueAt(ImportIssue::TooMany, path));
        return entries;
    }
    for (int i = 0; i < array.size(); ++i) {
        const QString here = QStringLiteral("%1[%2]").arg(path).arg(i);
        if (!array.at(i).isObject()) {
            problems->append(issueAt(ImportIssue::WrongType, here));
            continue;
        }
        const QJsonObject object = array.at(i).toObject();
        const QJsonValue id = object.value(QStringLiteral("id"));
        if (!id.isString()) {
            problems->append(issueAt(id.isUndefined() ? ImportIssue::Missing : ImportIssue::WrongType, here + QStringLiteral(".id")));
        } else if (!StarterLoader::isValidId(id.toString())) {
            problems->append(issueAt(ImportIssue::KeyInvalid, here + QStringLiteral(".id")));
        } else if (entries.objects.contains(id.toString())) {
            problems->append(issueAt(ImportIssue::DuplicateKey, here + QStringLiteral(".id")));
        } else {
            entries.ids.append(id.toString());
            entries.objects.insert(id.toString(), object);
        }
    }
    return entries;
}

// The texts of one language, by kind and identifier.
struct Texts {
    QHash<QString, QJsonObject> exercises;
    QHash<QString, QJsonObject> rhythms;
    QHash<QString, QJsonObject> workouts;
};

QHash<QString, QJsonObject> readTexts(const QJsonObject &root, const QString &path, const QSet<QString> &known,
                                      ImportIssue::Code unknown, const QString &file, QList<ImportIssue> *problems,
                                      QList<ImportIssue> *notices)
{
    QHash<QString, QJsonObject> texts;
    if (!root.contains(path))
        return texts;
    if (!root.value(path).isObject()) {
        problems->append(issueAt(ImportIssue::WrongType, file + QLatin1Char(' ') + path));
        return texts;
    }
    const QJsonObject object = root.value(path).toObject();
    for (auto it = object.begin(); it != object.end(); ++it) {
        const QString here = QStringLiteral("%1 %2.%3").arg(file, path, it.key());
        if (!known.contains(it.key())) {
            // A text of an entry that this version does not have is ignored.
            notices->append(issueAt(unknown, here));
        } else if (!it.value().isObject()) {
            problems->append(issueAt(ImportIssue::WrongType, here));
        } else {
            texts.insert(it.key(), it.value().toObject());
        }
    }
    return texts;
}

Texts parseTexts(const QByteArray &bytes, const QString &language, const QString &file, const Entries &exercises,
                 const Entries &rhythms, const Entries &workouts, QList<ImportIssue> *problems,
                 QList<ImportIssue> *notices)
{
    Texts texts;
    QJsonObject root;
    QList<ImportIssue> own;
    if (!Exchange::parseRoot(bytes, QStringLiteral("workout-trainer-starter-texts"), Exchange::starterVersion, &root, &own)) {
        for (ImportIssue &i : own)
            i.location = file;
        problems->append(own);
        return texts;
    }
    if (root.value(QStringLiteral("language")).toString() != language)
        problems->append(issueAt(ImportIssue::LanguageInvalid, file + QStringLiteral(" language")));
    texts.exercises = readTexts(root, QStringLiteral("exercises"), QSet<QString>::fromList(exercises.ids),
                                ImportIssue::UnknownExercise, file, problems, notices);
    texts.rhythms = readTexts(root, QStringLiteral("metronomes"), QSet<QString>::fromList(rhythms.ids),
                              ImportIssue::UnknownMetronome, file, problems, notices);
    texts.workouts = readTexts(root, QStringLiteral("workouts"), QSet<QString>::fromList(workouts.ids),
                               ImportIssue::WorkoutSkipped, file, problems, notices);
    return texts;
}

QJsonObject without(QJsonObject object, const QString &field)
{
    object.remove(field);
    return object;
}

// The objects of the export file for the exercises: the structure of each entry with its texts
// of `texts`, else the English texts and the tag "en".
QJsonArray exerciseObjects(const Entries &structure, const Texts &texts, const Texts &english, const QString &language)
{
    QJsonArray array;
    for (const QString &id : structure.ids) {
        const bool own = texts.exercises.contains(id);
        const QJsonObject text = own ? texts.exercises.value(id) : english.exercises.value(id);
        QJsonObject object = without(structure.objects.value(id), QStringLiteral("id"));
        object.insert(QStringLiteral("key"), StarterLoader::exerciseKey(id));
        object.insert(QStringLiteral("name"), text.value(QStringLiteral("name")));
        object.insert(QStringLiteral("instructions"), text.value(QStringLiteral("instructions")));
        object.insert(QStringLiteral("tips"), text.value(QStringLiteral("tips")));
        object.insert(QStringLiteral("language"), own ? language : QStringLiteral("en"));
        if (object.contains(QStringLiteral("metronome")))
            object.insert(QStringLiteral("metronome"), StarterLoader::rhythmKey(object.value(QStringLiteral("metronome")).toString()));
        array.append(object);
    }
    return array;
}

QJsonArray rhythmObjects(const Entries &structure, const Texts &texts, const Texts &english)
{
    QJsonArray array;
    for (const QString &id : structure.ids) {
        const QJsonObject text = texts.rhythms.contains(id) ? texts.rhythms.value(id) : english.rhythms.value(id);
        QJsonObject object = without(structure.objects.value(id), QStringLiteral("id"));
        object.insert(QStringLiteral("key"), StarterLoader::rhythmKey(id));
        object.insert(QStringLiteral("name"), text.value(QStringLiteral("name")));
        if (text.contains(QStringLiteral("phaseNames")) && !text.value(QStringLiteral("phaseNames")).toArray().isEmpty())
            object.insert(QStringLiteral("phaseNames"), text.value(QStringLiteral("phaseNames")));
        array.append(object);
    }
    return array;
}

bool needsEnglish(const Entries &structure, const QHash<QString, QJsonObject> &english, const QString &file,
                  const QString &path, QList<ImportIssue> *problems)
{
    bool ok = true;
    for (const QString &id : structure.ids) {
        if (!english.contains(id)) {
            problems->append(issueAt(ImportIssue::Missing, QStringLiteral("%1 %2.%3").arg(file, path, id)));
            ok = false;
        }
    }
    return ok;
}

QString idOfKey(const QString &key, const QString &prefix)
{
    return key.mid(prefix.size());
}

}

QString StarterLoader::exerciseKey(const QString &id)
{
    return exercisePrefix + id;
}

QString StarterLoader::rhythmKey(const QString &id)
{
    return rhythmPrefix + id;
}

bool StarterLoader::isValidId(const QString &id)
{
    static const QRegularExpression pattern(QStringLiteral("^[0-9a-f]{8}$"));
    return pattern.match(id).hasMatch();
}

StarterLoader::Result StarterLoader::apply(const QString &directory, const QString &language, Database *database)
{
    Result result;
    auto read = [&](const QString &name, QByteArray *bytes) {
        QFile file(QDir(directory).filePath(name));
        if (!file.open(QIODevice::ReadOnly) || file.size() > Exchange::maxFileBytes + 1) {
            result.problems.append(issueAt(ImportIssue::FileUnreadable, name));
            return false;
        }
        *bytes = file.read(Exchange::maxFileBytes + 1);
        return true;
    };
    Files files;
    if (!ExerciseStore::isValidLanguage(language)) {
        result.problems.append(issueAt(ImportIssue::LanguageInvalid, QStringLiteral("language")));
        return result;
    }
    const bool ok = read(Language::structureFileName(), &files.structure)
        && read(Language::workoutsFileName(), &files.workouts)
        && read(Language::textsFileName(language), &files.texts)
        && read(Language::textsFileName(QStringLiteral("en")), &files.english);
    if (!ok)
        return result;
    return applyBytes(files, language, database);
}

StarterLoader::Result StarterLoader::applyBytes(const Files &files, const QString &language, Database *database)
{
    Result result;
    if (!ExerciseStore::isValidLanguage(language)) {
        result.problems.append(issueAt(ImportIssue::LanguageInvalid, QStringLiteral("language")));
        return result;
    }
    const QString structureName = Language::structureFileName();
    const QString textsName = Language::textsFileName(language);
    const QString englishName = Language::textsFileName(QStringLiteral("en"));

    // ---- Check everything before anything is written ----
    QJsonObject structureRoot;
    QJsonObject workoutsRoot;
    QList<ImportIssue> own;
    if (!Exchange::parseRoot(files.structure, Exchange::starterFormat, Exchange::starterVersion, &structureRoot, &own)) {
        for (ImportIssue &i : own)
            i.location = structureName;
        result.problems.append(own);
    }
    own.clear();
    if (!Exchange::parseRoot(files.workouts, Exchange::starterWorkoutsFormat, Exchange::starterVersion, &workoutsRoot, &own)) {
        for (ImportIssue &i : own)
            i.location = Language::workoutsFileName();
        result.problems.append(own);
    }
    if (!result.problems.isEmpty()) {
        result.status = StoreStatus::Invalid;
        return result;
    }

    const Entries exercises = readList(structureRoot, QStringLiteral("exercises"), true, &result.problems);
    const Entries rhythms = readList(structureRoot, QStringLiteral("metronomes"), false, &result.problems);
    const Entries workouts = readList(workoutsRoot, QStringLiteral("workouts"), true, &result.problems);
    const Texts english = parseTexts(files.english, QStringLiteral("en"), englishName, exercises, rhythms, workouts,
                                     &result.problems, &result.notices);
    const bool sameFile = language == QLatin1String("en");
    const Texts chosen = sameFile ? english
                                  : parseTexts(files.texts, language, textsName, exercises, rhythms, workouts,
                                               &result.problems, &result.notices);
    if (result.problems.isEmpty()) {
        needsEnglish(exercises, english.exercises, englishName, QStringLiteral("exercises"), &result.problems);
        needsEnglish(rhythms, english.rhythms, englishName, QStringLiteral("metronomes"), &result.problems);
        needsEnglish(workouts, english.workouts, englishName, QStringLiteral("workouts"), &result.problems);
    }
    // The texts of both languages are put through the checks of the export file: the English ones
    // as they are, the others with the tag of their language.
    Exchange::ExerciseList englishList;
    QList<SavedMetronome> englishRhythms;
    Exchange::ExerciseList chosenList;
    QList<SavedMetronome> chosenRhythms;
    if (result.problems.isEmpty()) {
        QList<ImportIssue> ignored;
        Exchange::parseMetronomeList(rhythmObjects(rhythms, english, english), QStringLiteral("metronomes"), &englishRhythms,
                                     &result.problems, &ignored);
        Exchange::parseExercises(exerciseObjects(exercises, english, english, QStringLiteral("en")), QStringLiteral("exercises"),
                                 exercisePrefix, &englishList);
        result.problems.append(englishList.problems);
        if (!sameFile) {
            Exchange::parseMetronomeList(rhythmObjects(rhythms, chosen, english), QStringLiteral("metronomes"), &chosenRhythms,
                                         &result.problems, &ignored);
            Exchange::parseExercises(exerciseObjects(exercises, chosen, english, language), QStringLiteral("exercises"),
                                     exercisePrefix, &chosenList);
            result.problems.append(chosenList.problems);
        } else {
            chosenRhythms = englishRhythms;
            chosenList = englishList;
        }
        for (const SavedMetronome &m : chosenRhythms) {
            if (!m.key.startsWith(rhythmPrefix))
                result.problems.append(issueAt(ImportIssue::KeyPrefix, QStringLiteral("metronomes")));
        }
        if (chosenList.exercises.size() != exercises.ids.size() || chosenRhythms.size() != rhythms.ids.size())
            result.problems.append(issueAt(ImportIssue::NothingToImport, structureName));
    }
    if (!result.problems.isEmpty()) {
        result.status = StoreStatus::Invalid;
        return result;
    }

    // ---- Write ----
    MetronomeStore metronomes(database);
    ExerciseStore exerciseStore(database);
    WorkoutStore workoutStore(database);
    Transaction transaction(database);
    if (!transaction.isActive())
        return result;

    auto offered = [&](const char *kind, const QString &id, bool *is) {
        QSqlQuery q;
        if (!database->run(QStringLiteral("SELECT 1 FROM starter_offered WHERE kind = ? AND id = ?"),
                           QVariantList() << QLatin1String(kind) << id, &q))
            return false;
        *is = q.next();
        return true;
    };
    auto markOffered = [&](const char *kind, const QString &id) {
        return database->run(QStringLiteral("INSERT INTO starter_offered (kind, id) VALUES (?, ?)"),
                             QVariantList() << QLatin1String(kind) << id);
    };

    // Rhythms first: the exercises that name them link to them.
    for (SavedMetronome m : chosenRhythms) {
        const QString id = idOfKey(m.key, rhythmPrefix);
        bool done = false;
        if (!offered("rhythm", id, &done))
            return result;
        SavedMetronome present;
        if (!done) {
            if (!metronomes.getByKey(m.key, &present)) {
                m.name = Identity::freeName(database, Identity::Table::Metronome, m.name, QString(), QString(),
                                            QStringLiteral("starter"), Limits::maxNameChars);
                if (metronomes.create(m) != StoreStatus::Ok
                    || !database->run(QStringLiteral("UPDATE metronome SET starter_id = ?, follows_language = 1 WHERE key = ?"),
                                      QVariantList() << id << m.key))
                    return result;
                ++result.rhythmsAdded;
            }
            if (!markOffered("rhythm", id))
                return result;
            continue;
        }
        // Offered before: the entry that still follows the language takes the texts of the language.
        QSqlQuery q;
        if (!database->run(QStringLiteral("SELECT id, name, names FROM metronome WHERE starter_id = ? AND follows_language = 1"),
                           QVariantList() << id, &q))
            return result;
        if (!q.next())
            continue;
        const int rowId = q.value(0).toInt();
        const QString name = Identity::freeName(database, Identity::Table::Metronome, m.name, QString(), QString(),
                                                QStringLiteral("starter"), Limits::maxNameChars, rowId);
        const QString names = MetronomeStore::namesToColumn(m.tempo.phaseNameList());
        if (q.value(1).toString() == name && q.value(2).toString() == names)
            continue;
        if (!database->run(QStringLiteral("UPDATE metronome SET name = ?, names = ? WHERE id = ?"),
                           QVariantList() << textValue(name) << textValue(names) << rowId))
            return result;
        ++result.retexted;
    }

    QSet<QString> rhythmIds = QSet<QString>::fromList(rhythms.ids);
    for (Exercise e : chosenList.exercises) {
        e.starterId = idOfKey(e.key, exercisePrefix);
        // The link follows the library: a rhythm the user deleted leaves the exercise without a tempo.
        SavedMetronome linked;
        e.metronomeId = (!e.metronomeKey.isEmpty() && metronomes.getByKey(e.metronomeKey, &linked)) ? linked.id : 0;
        if (!e.metronomeKey.isEmpty() && !rhythmIds.contains(idOfKey(e.metronomeKey, rhythmPrefix)))
            result.notices.append(issueAt(ImportIssue::UnknownMetronome, QStringLiteral("exercises.%1.metronome").arg(e.starterId)));
        bool inserted = false;
        bool changed = false;
        if (exerciseStore.offerStarter(e, &inserted) != StoreStatus::Ok
            || exerciseStore.retextStarter(e, &changed) != StoreStatus::Ok)
            return result;
        if (inserted)
            ++result.exercisesAdded;
        else if (changed)
            ++result.retexted;
    }

    // Workouts: a step names its exercise by starter identifier; it is stored as the key of the entry that has it.
    const QJsonArray structureWorkouts = workoutsRoot.value(QStringLiteral("workouts")).toArray();
    for (int index = 0; index < structureWorkouts.size(); ++index) {
        const QJsonObject object = structureWorkouts.at(index).toObject();
        const QString id = object.value(QStringLiteral("id")).toString();
        if (!workouts.objects.contains(id))
            continue;
        const QString here = QStringLiteral("workouts[%1]").arg(index);
        const bool own = chosen.workouts.contains(id);
        const QJsonObject text = own ? chosen.workouts.value(id) : english.workouts.value(id);
        QJsonObject composed = without(object, QStringLiteral("id"));
        composed.insert(QStringLiteral("name"), text.value(QStringLiteral("name")));
        QJsonArray blocks = composed.value(QStringLiteral("blocks")).toArray();
        for (int b = 0; b < blocks.size(); ++b) {
            QJsonObject block = blocks.at(b).toObject();
            QJsonArray steps = block.value(QStringLiteral("steps")).toArray();
            for (int s = 0; s < steps.size(); ++s) {
                QJsonObject step = steps.at(s).toObject();
                const QString ref = step.value(QStringLiteral("exercise")).toString();
                if (isValidId(ref)) {
                    QSqlQuery q;
                    if (!database->run(QStringLiteral("SELECT key FROM exercise WHERE starter_id = ?"), QVariantList() << ref, &q))
                        return result;
                    if (q.next())
                        step.insert(QStringLiteral("exercise"), q.value(0).toString());
                }
                steps.replace(s, step);
            }
            block.insert(QStringLiteral("steps"), steps);
            blocks.replace(b, block);
        }
        composed.insert(QStringLiteral("blocks"), blocks);

        bool done = false;
        if (!offered("workout", id, &done))
            return result;
        if (done) {
            QSqlQuery q;
            if (!database->run(QStringLiteral("SELECT id, name, icon, colour FROM workout WHERE starter_id = ? AND follows_language = 1"),
                               QVariantList() << id, &q))
                return result;
            if (!q.next())
                continue;
            const int rowId = q.value(0).toInt();
            const QString name = Identity::freeName(database, Identity::Table::Workout, text.value(QStringLiteral("name")).toString(),
                                                    q.value(2).toString(), q.value(3).toString(), QStringLiteral("starter"),
                                                    Limits::maxNameChars, rowId);
            if (q.value(1).toString() == name)
                continue;
            if (!database->run(QStringLiteral("UPDATE workout SET name = ? WHERE id = ?"), QVariantList() << textValue(name) << rowId))
                return result;
            ++result.retexted;
            continue;
        }

        QList<ImportIssue> found;
        QList<ImportIssue> notices;
        Workout w;
        if (Exchange::checkWorkout(composed, here, &w, &found, &notices) && found.isEmpty()) {
            for (int b = 0; b < w.blocks.size(); ++b) {
                for (int s = 0; s < w.blocks.at(b).steps.size(); ++s) {
                    const QString &exercise = w.blocks.at(b).steps.at(s).exerciseKey;
                    Exercise probe;
                    if (!exercise.isEmpty() && !exerciseStore.get(exercise, &probe))
                        found.append(issueAt(ImportIssue::UnknownExercise,
                                             QStringLiteral("%1.blocks[%2].steps[%3].exercise").arg(here).arg(b).arg(s)));
                }
            }
        }
        if (!found.isEmpty()) {
            // Left out and offered again at the next start.
            result.notices.append(found);
            continue;
        }
        // The first step of a workout always starts automatically.
        if (!w.blocks.isEmpty() && !w.blocks.first().steps.isEmpty())
            w.blocks.first().steps.first().startOnTap = false;
        w.favorite = false;
        w.name = Identity::freeName(database, Identity::Table::Workout, w.name, w.icon, w.colour,
                                    QStringLiteral("starter"), Limits::maxNameChars);
        int rowId = 0;
        if (workoutStore.create(w, &rowId) != StoreStatus::Ok
            || !database->run(QStringLiteral("UPDATE workout SET starter_id = ?, follows_language = 1 WHERE id = ?"),
                              QVariantList() << id << rowId)
            || !markOffered("workout", id))
            return result;
        ++result.workoutsAdded;
    }

    if (!transaction.commit())
        return result;
    result.status = StoreStatus::Ok;
    return result;
}

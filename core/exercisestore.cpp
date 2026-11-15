// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "exercisestore.h"
#include "metronomestore.h"

#include <QUuid>
#include <algorithm>

#include "colours.h"
#include "identity.h"
#include "workoutlimits.h"

namespace {

Exercise readExercise(const QSqlQuery &q)
{
    Exercise e;
    e.key = q.value(0).toString();
    e.name = q.value(1).toString();
    e.muscles = q.value(2).toULongLong();
    e.instructions = q.value(3).toString();
    e.tips = q.value(4).toString();
    e.starter = q.value(5).toInt() != 0;
    e.icon = q.value(6).toString();
    e.colour = q.value(7).toString();
    e.language = q.value(8).toString();
    // A link to a metronome that does not exist joins to nothing and reads as no link.
    e.metronomeId = q.value(9).isNull() ? 0 : q.value(9).toInt();
    e.starterId = q.value(10).toString();
    e.followsLanguage = q.value(11).toInt() != 0;
    return e;
}

bool lessByName(const QString &a, const QString &b)
{
    return Identity::compareNames(a, b) < 0;
}

const char columns[] = "e.key, e.name, e.muscles, e.instructions, e.tips, e.starter, e.icon, e.colour, e.language, m.id, e.starter_id, e.follows_language";
const char source[] = "exercise e LEFT JOIN metronome m ON m.id = e.metronome_id";

}

bool ExerciseStore::isValidLanguage(const QString &tag)
{
    if (tag.length() < Limits::minLanguageChars || tag.length() > Limits::maxLanguageChars)
        return false;
    for (int i = 0; i < tag.length(); ++i) {
        const ushort u = tag.at(i).unicode();
        const bool letter = u >= 'a' && u <= 'z';
        const bool ok = letter || (i > 0 && ((u >= '0' && u <= '9') || u == '-'));
        if (!ok)
            return false;
    }
    return true;
}

QStringList ExerciseStore::languages()
{
    QStringList tags;
    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT DISTINCT language FROM exercise ORDER BY language"), QVariantList(), &q))
        return tags;
    while (q.next())
        tags.append(q.value(0).toString());
    return tags;
}

QList<ExerciseProblem> ExerciseStore::validate(const Exercise &exercise)
{
    QList<ExerciseProblem> found;
    if (exercise.name.trimmed().isEmpty())
        found.append(ExerciseProblem::NameEmpty);
    if (exercise.name.length() > Limits::maxNameChars)
        found.append(ExerciseProblem::NameTooLong);
    if (exercise.instructions.length() > Limits::maxInstructionsChars)
        found.append(ExerciseProblem::InstructionsTooLong);
    if (exercise.tips.length() > Limits::maxTipsChars)
        found.append(ExerciseProblem::TipsTooLong);
    if (exercise.muscles == 0)
        found.append(ExerciseProblem::NoMuscles);
    if ((exercise.muscles & ~allMuscles()) != 0)
        found.append(ExerciseProblem::UnknownMuscles);
    if (!Identity::isValidIconKey(exercise.icon))
        found.append(ExerciseProblem::IconInvalid);
    if (!exercise.colour.isEmpty() && !Colours::isKnown(exercise.colour))
        found.append(ExerciseProblem::ColourUnknown);
    if (!isValidLanguage(exercise.language))
        found.append(ExerciseProblem::LanguageInvalid);
    return found;
}

QVariant ExerciseStore::linkValue(int metronomeId)
{
    QSqlQuery q;
    if (metronomeId > 0 && db_->run(QStringLiteral("SELECT 1 FROM metronome WHERE id = ?"),
                                    QVariantList() << metronomeId, &q) && q.next())
        return metronomeId;
    return QVariant(QVariant::Int);
}

bool ExerciseStore::insert(const Exercise &e, bool starter)
{
    return db_->run(QStringLiteral("INSERT INTO exercise (key, name, muscles, instructions, tips, starter, icon, colour, language, "
                                   "metronome_id, starter_id, follows_language) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"),
                    QVariantList() << e.key << textValue(e.name) << e.muscles << textValue(e.instructions)
                                   << textValue(e.tips) << (starter ? 1 : 0) << textValue(e.icon) << textValue(e.colour)
                                   << e.language << linkValue(e.metronomeId)
                                   << textValue(starter ? e.starterId : QString()) << (starter && e.followsLanguage ? 1 : 0));
}

QString ExerciseStore::freeName(const QString &name, const QString &excludeKey)
{
    QSqlQuery q;
    QStringList taken;
    if (!db_->run(QStringLiteral("SELECT name FROM exercise WHERE key != ?"), QVariantList() << excludeKey, &q))
        return name;
    while (q.next())
        taken.append(Identity::nameKey(q.value(0).toString()));
    for (int n = 0;; ++n) {
        QString candidate = name;
        if (n > 0) {
            const QString suffix = n == 1 ? QStringLiteral(" (starter)") : QStringLiteral(" (starter %1)").arg(n);
            candidate = name.left(Limits::maxNameChars - suffix.length()) + suffix;
        }
        if (!taken.contains(Identity::nameKey(candidate)))
            return candidate;
    }
}

bool ExerciseStore::exists(const QString &key)
{
    QSqlQuery q;
    return db_->run(QStringLiteral("SELECT 1 FROM exercise WHERE key = ?"), QVariantList() << key, &q) && q.next();
}

StoreStatus ExerciseStore::create(Exercise *exercise, QList<ExerciseProblem> *problems)
{
    const QList<ExerciseProblem> found = validate(*exercise);
    if (problems)
        *problems = found;
    if (!found.isEmpty())
        return StoreStatus::Invalid;

    Exercise stored = *exercise;
    stored.key = QStringLiteral("u-") + QUuid::createUuid().toRfc4122().toHex();
    if (!insert(stored, false))
        return StoreStatus::Failed;
    exercise->key = stored.key;
    exercise->starter = false;
    if (linkValue(exercise->metronomeId).isNull())
        exercise->metronomeId = 0;
    return StoreStatus::Ok;
}

bool ExerciseStore::get(const QString &key, Exercise *exercise)
{
    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT %1 FROM %2 WHERE e.key = ?").arg(columns, source), QVariantList() << key, &q)
        || !q.next())
        return false;
    *exercise = readExercise(q);
    return true;
}

QList<Exercise> ExerciseStore::list()
{
    QList<Exercise> all;
    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT %1 FROM %2").arg(columns, source), QVariantList(), &q))
        return all;
    while (q.next())
        all.append(readExercise(q));
    std::sort(all.begin(), all.end(), [](const Exercise &a, const Exercise &b) {
        const int byName = Identity::compareNames(a.name, b.name);
        return byName != 0 ? byName < 0 : a.key < b.key;
    });
    return all;
}

StoreStatus ExerciseStore::update(const Exercise &exercise, QList<ExerciseProblem> *problems)
{
    const QList<ExerciseProblem> found = validate(exercise);
    if (problems)
        *problems = found;
    if (!found.isEmpty())
        return StoreStatus::Invalid;
    if (!exists(exercise.key))
        return StoreStatus::NotFound;
    // The first change of a text detaches the entry from the language for good (R-EXR-15).
    if (!db_->run(QStringLiteral("UPDATE exercise SET follows_language = CASE WHEN name = ?1 AND instructions = ?3 AND tips = ?4 "
                                 "THEN follows_language ELSE 0 END, "
                                 "name = ?1, muscles = ?2, instructions = ?3, tips = ?4, icon = ?5, colour = ?6, language = ?7, "
                                 "metronome_id = ?8 WHERE key = ?9"),
                  QVariantList() << textValue(exercise.name) << exercise.muscles << textValue(exercise.instructions)
                                 << textValue(exercise.tips) << textValue(exercise.icon)
                                 << textValue(exercise.colour) << exercise.language
                                 << linkValue(exercise.metronomeId) << exercise.key))
        return StoreStatus::Failed;
    return StoreStatus::Ok;
}

StoreStatus ExerciseStore::setMetronome(const QString &key, int metronomeId)
{
    if (!exists(key))
        return StoreStatus::NotFound;
    if (linkValue(metronomeId).isNull())
        return StoreStatus::Invalid;
    return db_->run(QStringLiteral("UPDATE exercise SET metronome_id = ? WHERE key = ?"),
                    QVariantList() << metronomeId << key)
        ? StoreStatus::Ok : StoreStatus::Failed;
}

StoreStatus ExerciseStore::clearMetronome(const QString &key)
{
    if (!exists(key))
        return StoreStatus::NotFound;
    return db_->run(QStringLiteral("UPDATE exercise SET metronome_id = NULL WHERE key = ?"), QVariantList() << key)
        ? StoreStatus::Ok : StoreStatus::Failed;
}

ExerciseTempos ExerciseStore::tempoMap()
{
    ExerciseTempos map;
    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT e.key, m.phases, m.names FROM exercise e JOIN metronome m ON m.id = e.metronome_id"),
                  QVariantList(), &q))
        return map;
    while (q.next()) {
        const Tempo tempo = MetronomeStore::rhythmFromColumns(q.value(1).toString(), q.value(2).toString());
        if (tempo.isValid())
            map.insert(q.value(0).toString(), tempo);
    }
    return map;
}

QStringList ExerciseStore::workoutsUsing(const QString &key)
{
    QStringList names;
    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT DISTINCT w.name FROM step s "
                                 "JOIN block b ON b.id = s.block_id "
                                 "JOIN workout w ON w.id = b.workout_id WHERE s.exercise_key = ?"),
                  QVariantList() << key, &q))
        return names;
    while (q.next())
        names.append(q.value(0).toString());
    std::sort(names.begin(), names.end(), lessByName);
    return names;
}

StoreStatus ExerciseStore::remove(const QString &key)
{
    if (!exists(key))
        return StoreStatus::NotFound;

    Transaction transaction(db_);
    if (!transaction.isActive())
        return StoreStatus::Failed;

    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT DISTINCT block_id FROM step WHERE exercise_key = ?"),
                  QVariantList() << key, &q))
        return StoreStatus::Failed;
    QList<QVariant> blocks;
    while (q.next())
        blocks.append(q.value(0));

    if (!db_->run(QStringLiteral("DELETE FROM step WHERE exercise_key = ?"), QVariantList() << key))
        return StoreStatus::Failed;
    for (const QVariant &block : blocks) {
        if (!db_->run(QStringLiteral("DELETE FROM block WHERE id = ? AND NOT EXISTS "
                                     "(SELECT 1 FROM step WHERE block_id = block.id)"),
                      QVariantList() << block))
            return StoreStatus::Failed;
    }
    if (!db_->run(QStringLiteral("DELETE FROM exercise WHERE key = ?"), QVariantList() << key))
        return StoreStatus::Failed;
    return transaction.commit() ? StoreStatus::Ok : StoreStatus::Failed;
}

StoreStatus ExerciseStore::offerStarter(const Exercise &given, bool *inserted, QList<ExerciseProblem> *problems)
{
    if (inserted)
        *inserted = false;
    Exercise exercise = given;
    const QList<ExerciseProblem> found = validate(exercise);
    if (problems)
        *problems = found;
    if (!found.isEmpty() || exercise.key.isEmpty() || exercise.starterId.isEmpty())
        return StoreStatus::Invalid;

    Transaction transaction(db_);
    if (!transaction.isActive())
        return StoreStatus::Failed;

    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT 1 FROM starter_offered WHERE kind = 'exercise' AND id = ?"),
                  QVariantList() << exercise.starterId, &q))
        return StoreStatus::Failed;
    if (q.next())
        return StoreStatus::Ok;

    // An entry with this key may exist already (for instance imported earlier): it stays as it is
    // and the identifier counts as offered.
    const bool present = exists(exercise.key);
    if (!present) {
        exercise.name = freeName(exercise.name, QString());
        exercise.followsLanguage = true;
    }
    if ((!present && !insert(exercise, true))
        || !db_->run(QStringLiteral("INSERT INTO starter_offered (kind, id) VALUES ('exercise', ?)"),
                     QVariantList() << exercise.starterId)
        || !transaction.commit())
        return StoreStatus::Failed;
    if (inserted)
        *inserted = !present;
    return StoreStatus::Ok;
}

StoreStatus ExerciseStore::retextStarter(const Exercise &texts, bool *changed)
{
    if (changed)
        *changed = false;
    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT key, name, instructions, tips, language FROM exercise "
                                 "WHERE starter_id = ? AND follows_language = 1"),
                  QVariantList() << texts.starterId, &q))
        return StoreStatus::Failed;
    if (!q.next())
        return StoreStatus::Ok;
    const QString key = q.value(0).toString();
    const QString name = freeName(texts.name, key);
    if (q.value(1).toString() == textValue(name).toString() && q.value(2).toString() == textValue(texts.instructions).toString()
        && q.value(3).toString() == textValue(texts.tips).toString() && q.value(4).toString() == texts.language)
        return StoreStatus::Ok;
    if (!db_->run(QStringLiteral("UPDATE exercise SET name = ?, instructions = ?, tips = ?, language = ? WHERE key = ?"),
                  QVariantList() << textValue(name) << textValue(texts.instructions) << textValue(texts.tips)
                                 << texts.language << key))
        return StoreStatus::Failed;
    if (changed)
        *changed = true;
    return StoreStatus::Ok;
}

StoreStatus ExerciseStore::createKeyed(const Exercise &exercise, QList<ExerciseProblem> *problems)
{
    const QList<ExerciseProblem> found = validate(exercise);
    if (problems)
        *problems = found;
    if (!found.isEmpty() || exercise.key.isEmpty())
        return StoreStatus::Invalid;
    if (exists(exercise.key))
        return StoreStatus::Failed;
    return insert(exercise, false) ? StoreStatus::Ok : StoreStatus::Failed;
}

StoreStatus ExerciseStore::clone(const QString &key, QString *newKey)
{
    Exercise copy;
    if (!get(key, &copy))
        return StoreStatus::NotFound;

    QStringList taken;
    for (const Exercise &e : list())
        taken.append(e.name.toLower());
    const QString base = copy.name;
    for (int n = 1;; ++n) {
        const QString suffix = n == 1 ? QStringLiteral(" (copy)") : QStringLiteral(" (copy %1)").arg(n);
        const QString candidate = base.left(Limits::maxNameChars - suffix.length()) + suffix;
        if (!taken.contains(candidate.toLower())) {
            copy.name = candidate;
            break;
        }
    }

    copy.key = QStringLiteral("u-") + QUuid::createUuid().toRfc4122().toHex();
    copy.starter = false;
    if (!insert(copy, false))
        return StoreStatus::Failed;
    if (newKey)
        *newKey = copy.key;
    return StoreStatus::Ok;
}

StoreStatus ExerciseStore::describeChange(const Exercise &edited, ExerciseChange *change)
{
    Exercise stored;
    if (!get(edited.key, &stored))
        return StoreStatus::NotFound;

    change->fields.clear();
    if (edited.name != stored.name)
        change->fields.append(ExerciseField::Name);
    if (edited.muscles != stored.muscles)
        change->fields.append(ExerciseField::Muscles);
    if (edited.instructions != stored.instructions)
        change->fields.append(ExerciseField::Instructions);
    if (edited.tips != stored.tips)
        change->fields.append(ExerciseField::Tips);
    if (edited.icon != stored.icon)
        change->fields.append(ExerciseField::Icon);
    if (edited.colour != stored.colour)
        change->fields.append(ExerciseField::Colour);
    if (edited.language != stored.language)
        change->fields.append(ExerciseField::Language);
    if (edited.metronomeId != stored.metronomeId)
        change->fields.append(ExerciseField::Metronome);
    change->workouts = workoutsUsing(edited.key);
    return StoreStatus::Ok;
}

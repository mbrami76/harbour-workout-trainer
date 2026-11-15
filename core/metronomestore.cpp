// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "metronomestore.h"

#include <QDateTime>
#include <QUrl>
#include <QUuid>
#include <algorithm>

#include "identity.h"
#include "workoutlimits.h"

namespace {

const char columns[] = "id, key, name, phases, names, created_at";

QList<ExerciseRef> sortedRefs(QList<ExerciseRef> refs)
{
    std::sort(refs.begin(), refs.end(), [](const ExerciseRef &a, const ExerciseRef &b) {
        const int byName = Identity::compareNames(a.name, b.name);
        return byName != 0 ? byName < 0 : a.key < b.key;
    });
    return refs;
}

QString phasesColumn(const Tempo &tempo)
{
    return MetronomeStore::phasesToColumn(tempo.phaseUnitList());
}

QString namesColumn(const Tempo &tempo)
{
    return MetronomeStore::namesToColumn(tempo.phaseNameList());
}

QString newKey()
{
    return QStringLiteral("m-") + QUuid::createUuid().toRfc4122().toHex();
}

}

QString MetronomeStore::phasesToColumn(const QList<int> &units)
{
    QStringList parts;
    for (int unit : units)
        parts.append(QString::number(unit));
    return parts.join(QLatin1Char(';'));
}

QString MetronomeStore::namesToColumn(const QStringList &names)
{
    // Percent-encoded UTF-8 so that any text, separators included, survives. Nothing at all when
    // no phase has a name.
    bool any = false;
    QStringList parts;
    for (const QString &name : names) {
        any = any || !name.isEmpty();
        parts.append(QString::fromLatin1(QUrl::toPercentEncoding(name)));
    }
    return any ? parts.join(QLatin1Char(';')) : QString(QLatin1String(""));
}

Tempo MetronomeStore::rhythmFromColumns(const QString &phases, const QString &names)
{
    QList<int> units;
    if (!phases.isEmpty()) {
        for (const QString &part : phases.split(QLatin1Char(';'))) {
            bool ok = false;
            const int unit = part.toInt(&ok);
            if (!ok)
                return Tempo();
            units.append(unit);
        }
    }
    QStringList decoded;
    if (!names.isEmpty()) {
        for (const QString &part : names.split(QLatin1Char(';')))
            decoded.append(QUrl::fromPercentEncoding(part.toLatin1()));
    }
    return Tempo::phases(units, 1, decoded);
}

QList<MetronomeProblem> MetronomeStore::validate(const SavedMetronome &metronome)
{
    QList<MetronomeProblem> found;
    MetronomeProblem problem;
    if (metronome.name.trimmed().isEmpty()) {
        problem.code = MetronomeProblem::NameEmpty;
        found.append(problem);
    }
    if (metronome.name.length() > Limits::maxNameChars) {
        problem.code = MetronomeProblem::NameTooLong;
        found.append(problem);
    }
    // A rhythm is the phases of one repetition: any other form or count of repetitions is refused.
    if (metronome.tempo.isValid() && (metronome.tempo.form() != TempoForm::Phases || metronome.tempo.reps() != 1)) {
        problem.code = MetronomeProblem::TempoInvalid;
        problem.tempoProblem = TempoProblem::PhaseCountOutOfRange;
        found.append(problem);
        problem.tempoProblem = TempoProblem::None;
    } else if (!metronome.tempo.isValid()) {
        problem.code = MetronomeProblem::TempoInvalid;
        problem.tempoProblem = metronome.tempo.problem();
        found.append(problem);
        problem.tempoProblem = TempoProblem::None;
    }
    return found;
}

QList<MetronomeProblem> MetronomeStore::check(const SavedMetronome &metronome, int excludeId, bool *failed)
{
    *failed = false;
    QList<MetronomeProblem> found = validate(metronome);
    if (!found.isEmpty())
        return found;

    // An edit that leaves the name as stored is not compared.
    if (excludeId > 0) {
        QSqlQuery q;
        if (!db_->run(QStringLiteral("SELECT name FROM metronome WHERE id = ?"),
                      QVariantList() << excludeId, &q)) {
            *failed = true;
            return found;
        }
        if (q.next() && Identity::isSame(metronome.name, QString(), QString(), q.value(0).toString(),
                                         QString(), QString()))
            return found;
    }
    bool taken = false;
    if (!Identity::isTaken(db_, Identity::Table::Metronome, metronome.name, QString(), QString(),
                           excludeId, &taken)) {
        *failed = true;
        return found;
    }
    if (taken) {
        MetronomeProblem problem;
        problem.code = MetronomeProblem::IdentityTaken;
        found.append(problem);
    }
    return found;
}

QList<MetronomeProblem> MetronomeStore::problemsOf(const SavedMetronome &metronome, int excludeId, bool *failed)
{
    bool error = false;
    const QList<MetronomeProblem> found = check(metronome, excludeId, &error);
    if (failed)
        *failed = error;
    return found;
}

bool MetronomeStore::exists(int id)
{
    QSqlQuery q;
    return db_->run(QStringLiteral("SELECT 1 FROM metronome WHERE id = ?"), QVariantList() << id, &q) && q.next();
}

SavedMetronome MetronomeStore::fromRow(const QSqlQuery &q)
{
    SavedMetronome m;
    m.id = q.value(0).toInt();
    m.key = q.value(1).toString();
    m.name = q.value(2).toString();
    // A rhythm that cannot be read leaves the empty tempo, which is invalid; validate() reports it.
    m.tempo = MetronomeStore::rhythmFromColumns(q.value(3).toString(), q.value(4).toString());
    m.createdAt = q.value(5).toLongLong();
    return m;
}

StoreStatus MetronomeStore::create(const SavedMetronome &given, int *id, QList<MetronomeProblem> *problems)
{
    bool failed = false;
    const QList<MetronomeProblem> found = check(given, 0, &failed);
    if (problems)
        *problems = found;
    if (failed)
        return StoreStatus::Failed;
    if (!found.isEmpty())
        return StoreStatus::Invalid;

    QString key = given.key;
    if (key.isEmpty()) {
        key = newKey();
    } else {
        SavedMetronome other;
        if (getByKey(key, &other))
            return StoreStatus::Invalid;
    }
    const qint64 created = given.createdAt != 0 ? given.createdAt : QDateTime::currentMSecsSinceEpoch() / 1000;
    if (!db_->run(QStringLiteral("INSERT INTO metronome (key, name, phases, names, created_at) "
                                 "VALUES (?, ?, ?, ?, ?)"),
                  QVariantList() << key << textValue(given.name)
                                 << phasesColumn(given.tempo) << namesColumn(given.tempo) << created))
        return StoreStatus::Failed;
    if (id)
        *id = int(db_->lastInsertId());
    return StoreStatus::Ok;
}

bool MetronomeStore::get(int id, SavedMetronome *metronome)
{
    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT %1 FROM metronome WHERE id = ?").arg(QLatin1String(columns)),
                  QVariantList() << id, &q) || !q.next())
        return false;
    *metronome = fromRow(q);
    return true;
}

bool MetronomeStore::getByKey(const QString &key, SavedMetronome *metronome)
{
    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT %1 FROM metronome WHERE key = ?").arg(QLatin1String(columns)),
                  QVariantList() << key, &q) || !q.next())
        return false;
    *metronome = fromRow(q);
    return true;
}

QList<ExerciseRef> MetronomeStore::exercisesUsing(int id)
{
    QList<ExerciseRef> refs;
    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT key, name FROM exercise WHERE metronome_id = ?"), QVariantList() << id, &q))
        return refs;
    while (q.next()) {
        ExerciseRef ref;
        ref.key = q.value(0).toString();
        ref.name = q.value(1).toString();
        refs.append(ref);
    }
    return sortedRefs(refs);
}

QList<ExerciseRef> MetronomeStore::impactOfEdit(int id)
{
    return exercisesUsing(id);
}

StoreStatus MetronomeStore::update(int id, const SavedMetronome &given, QList<MetronomeProblem> *problems,
                                   QList<ExerciseRef> *affected)
{
    bool failed = false;
    const QList<MetronomeProblem> found = check(given, id, &failed);
    if (problems)
        *problems = found;
    if (failed)
        return StoreStatus::Failed;
    if (!found.isEmpty())
        return StoreStatus::Invalid;
    if (!exists(id))
        return StoreStatus::NotFound;
    // The first change of the name or of a phase name detaches the rhythm from the language for good (R-EXR-15).
    if (!db_->run(QStringLiteral("UPDATE metronome SET follows_language = CASE WHEN name = ?1 AND names = ?3 "
                                 "THEN follows_language ELSE 0 END, "
                                 "name = ?1, phases = ?2, names = ?3 WHERE id = ?4"),
                  QVariantList() << textValue(given.name)
                                 << phasesColumn(given.tempo) << namesColumn(given.tempo) << id))
        return StoreStatus::Failed;
    if (affected)
        *affected = exercisesUsing(id);
    return StoreStatus::Ok;
}

StoreStatus MetronomeStore::saveAsCopy(const SavedMetronome &changed, int *newId, QList<MetronomeProblem> *problems)
{
    SavedMetronome copy = changed;
    copy.id = 0;
    copy.key.clear();
    copy.createdAt = 0;
    // Only a valid name can be made free; the rules are reported by create().
    if (!validate(copy).isEmpty())
        return create(copy, newId, problems);
    copy.name = Identity::freeName(db_, Identity::Table::Metronome, copy.name, QString(), QString(),
                                   QStringLiteral("copy"), Limits::maxNameChars);
    return create(copy, newId, problems);
}

StoreStatus MetronomeStore::duplicate(int id, int *newId)
{
    SavedMetronome source;
    if (!get(id, &source))
        return StoreStatus::NotFound;
    return saveAsCopy(source, newId);
}

StoreStatus MetronomeStore::remove(int id, QList<ExerciseRef> *cleared)
{
    if (!exists(id))
        return StoreStatus::NotFound;
    // The foreign key of the exercise table clears a link by itself; clearing it here as well
    // makes the rule independent of that setting and gives the list of what was cleared.
    Transaction transaction(db_);
    if (!transaction.isActive())
        return StoreStatus::Failed;
    const QList<ExerciseRef> users = exercisesUsing(id);
    if (!db_->run(QStringLiteral("UPDATE exercise SET metronome_id = NULL WHERE metronome_id = ?"), QVariantList() << id)
        || !db_->run(QStringLiteral("DELETE FROM metronome WHERE id = ?"), QVariantList() << id)
        || !transaction.commit())
        return StoreStatus::Failed;
    if (cleared)
        *cleared = users;
    return StoreStatus::Ok;
}

QList<MetronomeSummary> MetronomeStore::list()
{
    QList<MetronomeSummary> summaries;
    QHash<int, int> counts;
    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT metronome_id, COUNT(*) FROM exercise WHERE metronome_id IS NOT NULL "
                                 "GROUP BY metronome_id"), QVariantList(), &q))
        return summaries;
    while (q.next())
        counts.insert(q.value(0).toInt(), q.value(1).toInt());

    if (!db_->run(QStringLiteral("SELECT %1 FROM metronome").arg(QLatin1String(columns)), QVariantList(), &q))
        return summaries;
    while (q.next()) {
        const SavedMetronome m = fromRow(q);
        MetronomeSummary s;
        s.id = m.id;
        s.key = m.key;
        s.name = m.name;
        s.phaseCount = m.tempo.phaseCount();
        s.phaseUnits = m.tempo.phaseUnitList();
        s.phaseNames = m.tempo.phaseNameList();
        s.repetitionMs = m.tempo.isValid() ? m.tempo.setDurationMs() : 0;
        s.exerciseCount = counts.value(m.id);
        summaries.append(s);
    }
    std::sort(summaries.begin(), summaries.end(), [](const MetronomeSummary &a, const MetronomeSummary &b) {
        const int byName = Identity::compareNames(a.name, b.name);
        return byName != 0 ? byName < 0 : a.id < b.id;
    });
    return summaries;
}

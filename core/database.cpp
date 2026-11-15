// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "database.h"

#include <QDir>
#include <QSqlError>
#include <QUuid>
#include <algorithm>
#include <QSet>

#include "identity.h"
#include "workoutlimits.h"


namespace {

const char dbFileName[] = "workouttrainer.db";

// The schema of version 11, the first step. Until the first release a database older than this
// step is refused and left as it is (R-NFR-3); every later change of the schema is one more
// numbered step in builtinMigrations().
// Steps refer to exercises by key without a cascade: removing an exercise is done explicitly by the
// store. The link of an exercise to its rhythm is cleared by the database when the rhythm is
// deleted (ON DELETE SET NULL, effective because every connection runs with foreign keys on, see
// Database::migrate()).
QStringList currentSchema()
{
    return QStringList()
        << "CREATE TABLE metronome ("
           " id INTEGER PRIMARY KEY AUTOINCREMENT,"
           " key TEXT NOT NULL UNIQUE,"
           " name TEXT NOT NULL CHECK (length(name) > 0),"
           " icon TEXT NOT NULL DEFAULT '',"
           " colour TEXT NOT NULL DEFAULT '',"
           " phases TEXT NOT NULL,"
           " names TEXT NOT NULL DEFAULT '',"
           " created_at INTEGER NOT NULL,"
           " starter_id TEXT NOT NULL DEFAULT '',"
           " follows_language INTEGER NOT NULL DEFAULT 0 CHECK (follows_language IN (0, 1)))"
        << "CREATE TABLE exercise ("
           " key TEXT PRIMARY KEY NOT NULL,"
           " name TEXT NOT NULL CHECK (length(name) > 0),"
           " muscles INTEGER NOT NULL CHECK (muscles > 0),"
           " instructions TEXT NOT NULL DEFAULT '',"
           " tips TEXT NOT NULL DEFAULT '',"
           " starter INTEGER NOT NULL DEFAULT 0 CHECK (starter IN (0, 1)),"
           " icon TEXT NOT NULL DEFAULT '',"
           " colour TEXT NOT NULL DEFAULT '',"
           " language TEXT NOT NULL DEFAULT 'en',"
           " starter_id TEXT NOT NULL DEFAULT '',"
           " follows_language INTEGER NOT NULL DEFAULT 0 CHECK (follows_language IN (0, 1)),"
           " metronome_id INTEGER REFERENCES metronome(id) ON DELETE SET NULL)"
        << "CREATE INDEX exercise_metronome ON exercise(metronome_id)"
        << "CREATE TABLE workout ("
           " id INTEGER PRIMARY KEY AUTOINCREMENT,"
           " name TEXT NOT NULL CHECK (length(name) > 0),"
           " favorite INTEGER NOT NULL DEFAULT 0 CHECK (favorite IN (0, 1)),"
           " rest_between_blocks INTEGER NOT NULL DEFAULT 0 CHECK (rest_between_blocks >= 0),"
           " icon TEXT NOT NULL DEFAULT '',"
           " colour TEXT NOT NULL DEFAULT '',"
           " starter_id TEXT NOT NULL DEFAULT '',"
           " follows_language INTEGER NOT NULL DEFAULT 0 CHECK (follows_language IN (0, 1)))"
        << "CREATE TABLE block ("
           " id INTEGER PRIMARY KEY AUTOINCREMENT,"
           " workout_id INTEGER NOT NULL REFERENCES workout(id) ON DELETE CASCADE,"
           " position INTEGER NOT NULL,"
           " rounds INTEGER NOT NULL CHECK (rounds >= 1),"
           " rest_between_rounds INTEGER NOT NULL DEFAULT 0 CHECK (rest_between_rounds >= 0),"
           " UNIQUE (workout_id, position))"
        // The pace of a step (R-WRK-14): its kind ("total", "per-rep" or empty for none) and its
        // value in milliseconds.
        << "CREATE TABLE step ("
           " id INTEGER PRIMARY KEY AUTOINCREMENT,"
           " block_id INTEGER NOT NULL REFERENCES block(id) ON DELETE CASCADE,"
           " position INTEGER NOT NULL,"
           " type TEXT NOT NULL CHECK (type IN ('work', 'rest', 'warmup', 'cooldown')),"
           " exercise_key TEXT REFERENCES exercise(key),"
           " duration_sec INTEGER CHECK (duration_sec IS NULL OR duration_sec >= 1),"
           " reps INTEGER CHECK (reps IS NULL OR reps >= 1),"
           " start_on_tap INTEGER NOT NULL DEFAULT 0,"
           " pace_kind TEXT NOT NULL DEFAULT '',"
           " pace_ms INTEGER NOT NULL DEFAULT 0,"
           " UNIQUE (block_id, position),"
           " CHECK ((duration_sec IS NULL) <> (reps IS NULL)),"
           " CHECK ((type = 'rest') = (exercise_key IS NULL)))"
        << "CREATE INDEX step_exercise ON step(exercise_key)"
        // The starter identifiers offered so far, one sequence per kind (R-EXR-6, R-EXR-15).
        << "CREATE TABLE starter_offered ("
           " kind TEXT NOT NULL CHECK (kind IN ('exercise', 'rhythm', 'workout')),"
           " id TEXT NOT NULL,"
           " PRIMARY KEY (kind, id))"
        << "CREATE TABLE session ("
           " id INTEGER PRIMARY KEY AUTOINCREMENT,"
           " workout_name TEXT NOT NULL,"
           " started_at INTEGER NOT NULL,"
           " total_sec INTEGER NOT NULL CHECK (total_sec >= 0),"
           " end_state TEXT NOT NULL CHECK (end_state IN ('completed', 'stopped')))"
        << "CREATE INDEX session_started ON session(started_at)"
        << "CREATE TABLE session_step ("
           " session_id INTEGER NOT NULL REFERENCES session(id) ON DELETE CASCADE,"
           " position INTEGER NOT NULL,"
           " exercise_name TEXT NOT NULL DEFAULT '',"
           " muscles INTEGER NOT NULL DEFAULT 0,"
           " type TEXT NOT NULL CHECK (type IN ('work', 'rest', 'warmup', 'cooldown')),"
           " planned_sec INTEGER,"
           " planned_reps INTEGER,"
           " actual_sec INTEGER NOT NULL CHECK (actual_sec >= 0),"
           " PRIMARY KEY (session_id, position))"
        << "CREATE TABLE challenge ("
           " id INTEGER PRIMARY KEY AUTOINCREMENT,"
           " name TEXT NOT NULL CHECK (length(name) > 0),"
           " created_at INTEGER NOT NULL,"
           " icon TEXT NOT NULL DEFAULT '',"
           " colour TEXT NOT NULL DEFAULT '')"
        << "CREATE TABLE result ("
           " id INTEGER PRIMARY KEY AUTOINCREMENT,"
           " challenge_id INTEGER NOT NULL REFERENCES challenge(id) ON DELETE CASCADE,"
           " duration_ms INTEGER NOT NULL CHECK (duration_ms >= 1 AND duration_ms <= 86400000),"
           " finished_at INTEGER NOT NULL)"
        << "CREATE INDEX result_challenge ON result(challenge_id)";
}

}

namespace {

// Version 12: a rhythm has no icon and no colour (R-MET-7). The columns stay in the table, always empty
// and never read: the table cannot be rebuilt without clearing the link of the exercises. Rhythms that
// now have the same name as an earlier one (the lower id keeps the name) take the suffix
// " (copy)", " (copy 2)", ... so that none is lost.
bool dropRhythmIcons(Database *database)
{
    QSqlQuery q;
    if (!database->run(QStringLiteral("SELECT id, name FROM metronome ORDER BY id"), QVariantList(), &q))
        return false;
    struct Row { int id; QString name; };
    QList<Row> rows;
    while (q.next())
        rows.append({q.value(0).toInt(), q.value(1).toString()});
    QSet<QString> seen;
    for (const Row &row : rows) {
        const QString identity = Identity::nameKey(row.name);
        if (!seen.contains(identity)) {
            seen.insert(identity);
            continue;
        }
        const QString name = Identity::freeName(database, Identity::Table::Metronome, row.name, QString(), QString(),
                                                QStringLiteral("copy"), Limits::maxNameChars, row.id);
        if (!database->run(QStringLiteral("UPDATE metronome SET name = ? WHERE id = ?"), QVariantList() << name << row.id))
            return false;
        seen.insert(Identity::nameKey(name));
    }
    return database->run(QStringLiteral("UPDATE metronome SET icon = '', colour = ''"));
}

}

QList<Migration> Database::builtinMigrations()
{
    Migration first;
    first.version = 11;
    first.statements = currentSchema();
    Migration noRhythmIcon;
    noRhythmIcon.version = 12;
    noRhythmIcon.convert = dropRhythmIcons;
    return QList<Migration>() << first << noRhythmIcon;
}

int Database::builtinVersion()
{
    int latest = 0;
    for (const Migration &m : builtinMigrations())
        latest = qMax(latest, m.version);
    return latest;
}

QList<Migration> Database::allMigrations() const
{
    QList<Migration> all = builtinMigrations();
    for (const Migration &m : extra_) {
        bool known = false;
        for (const Migration &have : all)
            known = known || have.version == m.version;
        if (!known)
            all.append(m);
    }
    std::sort(all.begin(), all.end(), [](const Migration &a, const Migration &b) { return a.version < b.version; });
    return all;
}

Database::Database()
{
}

Database::~Database()
{
    close();
}

QSqlDatabase Database::handle() const
{
    return QSqlDatabase::database(connection_, false);
}

bool Database::isOpen() const
{
    return !connection_.isEmpty() && handle().isOpen();
}

void Database::close()
{
    if (connection_.isEmpty())
        return;
    {
        QSqlDatabase db = handle();
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(connection_);
    connection_.clear();
}

bool Database::fail(const QString &text, QString *error)
{
    lastError_ = text;
    if (error)
        *error = text;
    close();
    return false;
}

void Database::removeStaleFiles(const QString &directory) const
{
    QDir dir(directory);
    const QStringList stale = dir.entryList(QStringList() << "*.tmp" << "*.new", QDir::Files);
    for (const QString &name : stale)
        dir.remove(name);
}

bool Database::run(const QString &sql, const QVariantList &args, QSqlQuery *result)
{
    QSqlQuery query(handle());
    if (!query.prepare(sql) ) {
        lastError_ = query.lastError().text();
        return false;
    }
    for (const QVariant &value : args)
        query.addBindValue(value);
    if (!query.exec()) {
        lastError_ = query.lastError().text();
        return false;
    }
    lastInsertId_ = query.lastInsertId().toLongLong();
    if (result)
        *result = query;
    return true;
}

int Database::schemaVersion()
{
    QSqlQuery query;
    if (!run(QStringLiteral("PRAGMA user_version"), QVariantList(), &query) || !query.next())
        return -1;
    return query.value(0).toInt();
}

bool Database::migrate(QString *error)
{
    const QList<Migration> all = allMigrations();
    const int latest = all.isEmpty() ? 0 : all.last().version;
    int current = schemaVersion();
    if (current < 0)
        return fail(lastError_, error);
    if (current > latest)
        return fail(QStringLiteral("The database version %1 is newer than the supported version %2.")
                        .arg(current).arg(latest), error);
    // A database that has been created has a version of the list; one below the first step has no
    // way to reach it (nothing older than the first step exists before the release).
    if (current > 0 && !all.isEmpty() && current < all.first().version)
        return fail(QStringLiteral("The database version %1 is older than the supported version %2 and cannot be converted.")
                        .arg(current).arg(all.first().version), error);

    if (!run(QStringLiteral("PRAGMA foreign_keys = ON")) || !run(QStringLiteral("PRAGMA synchronous = FULL")))
        return fail(lastError_, error);
    // The rules that clear and restrict references live in the schema; they act only when this
    // setting is on, so a library that ignores it must not open the file.
    QSqlQuery foreignKeys;
    if (!run(QStringLiteral("PRAGMA foreign_keys"), QVariantList(), &foreignKeys) || !foreignKeys.next()
        || foreignKeys.value(0).toInt() != 1)
        return fail(QStringLiteral("The database library does not enforce foreign keys."), error);

    for (const Migration &migration : all) {
        if (migration.version <= current)
            continue;
        Transaction transaction(this);
        if (!transaction.isActive())
            return fail(lastError_, error);
        for (const QString &statement : migration.statements) {
            if (!run(statement))
                return fail(lastError_, error);
        }
        if (migration.convert && !migration.convert(this))
            return fail(lastError_, error);
        if (!run(QStringLiteral("PRAGMA user_version = %1").arg(migration.version)) || !transaction.commit())
            return fail(lastError_, error);
        current = migration.version;
    }
    return true;
}

bool Database::open(const QString &directory, QString *error)
{
    close();
    transactionDepth_ = 0;
    QDir dir(directory);
    if (!dir.mkpath(QStringLiteral(".")))
        return fail(QStringLiteral("The data directory cannot be created."), error);
    removeStaleFiles(dir.absolutePath());

    connection_ = QStringLiteral("workouttrainer-") + QUuid::createUuid().toRfc4122().toHex();
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection_);
    db.setDatabaseName(dir.filePath(QLatin1String(dbFileName)));
    if (!db.open())
        return fail(db.lastError().text(), error);
    return migrate(error);
}

Transaction::Transaction(Database *database)
    : database_(database)
{
    level_ = database_->transactionDepth_;
    const QString sql = level_ == 0 ? QStringLiteral("BEGIN") : QStringLiteral("SAVEPOINT rt_sp%1").arg(level_);
    active_ = database_->run(sql);
    if (active_)
        ++database_->transactionDepth_;
}

Transaction::~Transaction()
{
    if (!active_)
        return;
    if (level_ == 0) {
        database_->run(QStringLiteral("ROLLBACK"));
    } else {
        database_->run(QStringLiteral("ROLLBACK TO rt_sp%1").arg(level_));
        database_->run(QStringLiteral("RELEASE rt_sp%1").arg(level_));
    }
    database_->transactionDepth_ = level_;
}

bool Transaction::commit()
{
    if (!active_)
        return false;
    const QString sql = level_ == 0 ? QStringLiteral("COMMIT") : QStringLiteral("RELEASE rt_sp%1").arg(level_);
    if (!database_->run(sql))
        return false;
    active_ = false;
    database_->transactionDepth_ = level_;
    return true;
}

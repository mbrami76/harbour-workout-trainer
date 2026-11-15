// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "musclegroups.h"
#include "timeattackstore.h"
#include "workoutstore.h"
#include "testdb.h"

class TstDatabase : public QObject
{
    Q_OBJECT

private slots:
    void newDatabaseGoesToLatestVersion();
    void connectionSettings();
    void instancesCoexist();
    void upgradeKeepsData();
    void failedMigrationLeavesVersionAndSchema();
    void newerVersionIsRefusedUntouched();
    void staleTemporaryFilesAreRemoved();
    void reopenKeepsData();
    void transactionWithoutCommitRollsBack();
    void transactionCommitKeepsData();
    void unusableDirectoryIsReported();
    void versionFiveCascadeIsEnforced();
    void nestedCommitKeepsData();
    void nestedRollbackUndoesOnlyTheInnerChanges();
    void outerRollbackUndoesACommittedInnerTransaction();
    void deepNestingWorks();
    void currentSchemaHasTheRhythmAndPaceColumns();
    void anOlderVersionIsRefusedUntouched();
    void versionElevenRhythmsLoseTheirIconAndKeepEveryRow();
    void versionElevenWithoutClashesKeepsTheNames();
    void foreignKeysAreOnForEveryConnection();
};

static QByteArray fileBytes(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

void TstDatabase::newDatabaseGoesToLatestVersion()
{
    TestDb t;
    QVERIFY2(t.open(), qPrintable(t.error));
    QCOMPARE(t.db.schemaVersion(), Database::builtinVersion());
    QVERIFY(QFileInfo(inDir(t.dir, "workouttrainer.db")).exists());
    const QStringList tables = QStringList() << "exercise" << "workout" << "block" << "step"
                                             << "starter_offered" << "session" << "session_step";
    for (const QString &table : tables)
        QCOMPARE(countRows(t.db, table), 0);
}

void TstDatabase::connectionSettings()
{
    TestDb t;
    QVERIFY(t.open());
    QSqlQuery q;
    QVERIFY(t.db.run("PRAGMA foreign_keys", QVariantList(), &q) && q.next());
    QCOMPARE(q.value(0).toInt(), 1);
    QVERIFY(t.db.run("PRAGMA synchronous", QVariantList(), &q) && q.next());
    QCOMPARE(q.value(0).toInt(), 2);
    QVERIFY(t.db.run("PRAGMA journal_mode", QVariantList(), &q) && q.next());
    QCOMPARE(q.value(0).toString(), QStringLiteral("delete"));
}

void TstDatabase::instancesCoexist()
{
    TestDb a;
    TestDb b;
    QVERIFY(a.open());
    QVERIFY(b.open());
    QVERIFY(!addExercise(a.db, "Only in a").isEmpty());
    QCOMPARE(countRows(a.db, "exercise"), 1);
    QCOMPARE(countRows(b.db, "exercise"), 0);
}

void TstDatabase::upgradeKeepsData()
{
    TestDb t;
    QVERIFY(t.open());
    const QString key = addExercise(t.db, "Kept");
    QVERIFY(!key.isEmpty());
    t.db.close();

    Migration second;
    second.version = Database::builtinVersion() + 1;
    second.statements << "ALTER TABLE exercise ADD COLUMN note TEXT"
                      << "UPDATE exercise SET note = 'added'";
    t.db.setExtraMigrations(QList<Migration>() << second);
    QVERIFY2(t.db.open(t.dir.path(), &t.error), qPrintable(t.error));
    QCOMPARE(t.db.schemaVersion(), Database::builtinVersion() + 1);

    ExerciseStore store(&t.db);
    Exercise e;
    QVERIFY(store.get(key, &e));
    QCOMPARE(e.name, QStringLiteral("Kept"));
    QSqlQuery q;
    QVERIFY(t.db.run("SELECT note FROM exercise WHERE key = ?", QVariantList() << key, &q) && q.next());
    QCOMPARE(q.value(0).toString(), QStringLiteral("added"));
    QVERIFY(integrityOk(t.db));
}

void TstDatabase::failedMigrationLeavesVersionAndSchema()
{
    TestDb t;
    QVERIFY(t.open());
    QVERIFY(!addExercise(t.db, "Kept").isEmpty());
    t.db.close();

    Migration broken;
    broken.version = Database::builtinVersion() + 1;
    broken.statements << "ALTER TABLE exercise ADD COLUMN extra TEXT" << "THIS IS NOT SQL";
    t.db.setExtraMigrations(QList<Migration>() << broken);
    QString error;
    QVERIFY(!t.db.open(t.dir.path(), &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!t.db.isOpen());

    t.db.setExtraMigrations(QList<Migration>());
    QVERIFY(t.db.open(t.dir.path(), &error));
    QCOMPARE(t.db.schemaVersion(), Database::builtinVersion());
    QSqlQuery q;
    QVERIFY(!t.db.run("SELECT extra FROM exercise", QVariantList(), &q));
    QCOMPARE(countRows(t.db, "exercise"), 1);
}

void TstDatabase::newerVersionIsRefusedUntouched()
{
    TestDb t;
    Migration second;
    second.version = Database::builtinVersion() + 1;
    second.statements << "ALTER TABLE exercise ADD COLUMN note TEXT";
    t.db.setExtraMigrations(QList<Migration>() << second);
    QVERIFY(t.open());
    QVERIFY(!addExercise(t.db, "Kept").isEmpty());
    t.db.close();

    const QString path = inDir(t.dir, "workouttrainer.db");
    const QByteArray before = fileBytes(path);
    QVERIFY(!before.isEmpty());

    Database older;
    QString error;
    QVERIFY(!older.open(t.dir.path(), &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!older.isOpen());
    QCOMPARE(fileBytes(path), before);

    QVERIFY(t.db.open(t.dir.path(), &error));
    QCOMPARE(t.db.schemaVersion(), Database::builtinVersion() + 1);
    QCOMPARE(countRows(t.db, "exercise"), 1);
}

void TstDatabase::staleTemporaryFilesAreRemoved()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QStringList stale = QStringList() << "export.tmp" << "workouts.new" << "other.tmp";
    for (const QString &name : stale) {
        QFile f(inDir(dir, name));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("x");
    }
    QFile keep(inDir(dir, "keep.txt"));
    QVERIFY(keep.open(QIODevice::WriteOnly));
    keep.close();

    Database db;
    QString error;
    QVERIFY2(db.open(dir.path(), &error), qPrintable(error));
    for (const QString &name : stale)
        QVERIFY(!QFileInfo(inDir(dir, name)).exists());
    QVERIFY(QFileInfo(inDir(dir, "keep.txt")).exists());
}

void TstDatabase::reopenKeepsData()
{
    TestDb t;
    QVERIFY(t.open());
    QVERIFY(!addExercise(t.db, "Kept").isEmpty());
    t.db.close();
    QVERIFY(t.db.open(t.dir.path(), &t.error));
    QCOMPARE(t.db.schemaVersion(), Database::builtinVersion());
    QCOMPARE(countRows(t.db, "exercise"), 1);
}

void TstDatabase::transactionWithoutCommitRollsBack()
{
    TestDb t;
    QVERIFY(t.open());
    {
        Transaction transaction(&t.db);
        QVERIFY(transaction.isActive());
        QVERIFY(t.db.run("INSERT INTO starter_offered (kind, id) VALUES ('exercise', 'a')"));
        QVERIFY(t.db.run("INSERT INTO starter_offered (kind, id) VALUES ('exercise', 'b')"));
    }
    QCOMPARE(countRows(t.db, "starter_offered"), 0);
    QVERIFY(integrityOk(t.db));
}

void TstDatabase::transactionCommitKeepsData()
{
    TestDb t;
    QVERIFY(t.open());
    {
        Transaction transaction(&t.db);
        QVERIFY(t.db.run("INSERT INTO starter_offered (kind, id) VALUES ('exercise', 'a')"));
        QVERIFY(transaction.commit());
    }
    QCOMPARE(countRows(t.db, "starter_offered"), 1);
}

void TstDatabase::unusableDirectoryIsReported()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile blocker(inDir(dir, "file"));
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.close();

    Database db;
    QString error;
    QVERIFY(!db.open(inDir(dir, "file/sub"), &error));
    QVERIFY(!error.isEmpty());
}

void TstDatabase::versionFiveCascadeIsEnforced()
{
    TestDb t;
    QVERIFY2(t.open(), qPrintable(t.error));
    // A result needs an existing challenge: the foreign key is enforced.
    QVERIFY(!t.db.run("INSERT INTO result (challenge_id, duration_ms, finished_at) VALUES (99, 1000, 1)"));
    QVERIFY(t.db.run("INSERT INTO challenge (name, created_at) VALUES ('C', 1)"));
    const qint64 id = t.db.lastInsertId();
    QVERIFY(t.db.run("INSERT INTO result (challenge_id, duration_ms, finished_at) VALUES (?, 1000, 1)", QVariantList() << id));
    // The table rejects durations outside 1 ms to 24 h.
    QVERIFY(!t.db.run("INSERT INTO result (challenge_id, duration_ms, finished_at) VALUES (?, 0, 1)", QVariantList() << id));
    QVERIFY(!t.db.run("INSERT INTO result (challenge_id, duration_ms, finished_at) VALUES (?, 86400001, 1)", QVariantList() << id));
    QVERIFY(t.db.run("DELETE FROM challenge WHERE id = ?", QVariantList() << id));
    QCOMPARE(countRows(t.db, "result"), 0);
}

static int offered(Database &db)
{
    return countRows(db, "starter_offered");
}

void TstDatabase::nestedCommitKeepsData()
{
    TestDb t;
    QVERIFY(t.open());
    {
        Transaction outer(&t.db);
        QVERIFY(t.db.run("INSERT INTO starter_offered (kind, id) VALUES ('exercise', 'a')"));
        {
            Transaction inner(&t.db);
            QVERIFY(inner.isActive());
            QVERIFY(t.db.run("INSERT INTO starter_offered (kind, id) VALUES ('exercise', 'b')"));
            QVERIFY(inner.commit());
        }
        QCOMPARE(offered(t.db), 2);
        QVERIFY(outer.commit());
    }
    QCOMPARE(offered(t.db), 2);
    QVERIFY(integrityOk(t.db));
}

void TstDatabase::nestedRollbackUndoesOnlyTheInnerChanges()
{
    TestDb t;
    QVERIFY(t.open());
    {
        Transaction outer(&t.db);
        QVERIFY(t.db.run("INSERT INTO starter_offered (kind, id) VALUES ('exercise', 'a')"));
        {
            Transaction inner(&t.db);
            QVERIFY(t.db.run("INSERT INTO starter_offered (kind, id) VALUES ('exercise', 'b')"));
        }
        QCOMPARE(offered(t.db), 1);
        // The outer transaction is still usable after the inner rollback.
        QVERIFY(t.db.run("INSERT INTO starter_offered (kind, id) VALUES ('exercise', 'c')"));
        QVERIFY(outer.commit());
    }
    QCOMPARE(offered(t.db), 2);
    QVERIFY(integrityOk(t.db));
}

void TstDatabase::outerRollbackUndoesACommittedInnerTransaction()
{
    TestDb t;
    QVERIFY(t.open());
    {
        Transaction outer(&t.db);
        QVERIFY(t.db.run("INSERT INTO starter_offered (kind, id) VALUES ('exercise', 'a')"));
        {
            Transaction inner(&t.db);
            QVERIFY(t.db.run("INSERT INTO starter_offered (kind, id) VALUES ('exercise', 'b')"));
            QVERIFY(inner.commit());
        }
        QCOMPARE(offered(t.db), 2);
    }
    QCOMPARE(offered(t.db), 0);
    // A new top-level transaction works after the rollback.
    Transaction again(&t.db);
    QVERIFY(again.isActive());
    QVERIFY(t.db.run("INSERT INTO starter_offered (kind, id) VALUES ('exercise', 'z')"));
    QVERIFY(again.commit());
    QCOMPARE(offered(t.db), 1);
}

void TstDatabase::deepNestingWorks()
{
    TestDb t;
    QVERIFY(t.open());
    {
        Transaction l0(&t.db);
        Transaction l1(&t.db);
        Transaction l2(&t.db);
        QVERIFY(t.db.run("INSERT INTO starter_offered (kind, id) VALUES ('exercise', 'deep')"));
        QVERIFY(l2.commit());
        QVERIFY(l1.commit());
        QVERIFY(l0.commit());
    }
    QCOMPARE(offered(t.db), 1);
}

namespace {

QStringList columnsOf(Database &db, const QString &table)
{
    QSqlQuery q;
    QStringList names;
    if (db.run(QStringLiteral("PRAGMA table_info(%1)").arg(table), QVariantList(), &q)) {
        while (q.next())
            names << q.value(1).toString();
    }
    return names;
}

}

void TstDatabase::currentSchemaHasTheRhythmAndPaceColumns()
{
    TestDb t;
    QVERIFY2(t.open(), qPrintable(t.error));
    QCOMPARE(t.db.schemaVersion(), 12);
    QCOMPARE(Database::builtinVersion(), 12);
    QCOMPARE(columnsOf(t.db, "metronome"),
             QStringList() << "id" << "key" << "name" << "icon" << "colour" << "phases" << "names" << "created_at" << "starter_id" << "follows_language");
    const QStringList stepColumns = columnsOf(t.db, "step");
    QVERIFY(stepColumns.contains("pace_kind"));
    QVERIFY(stepColumns.contains("pace_ms"));
    QVERIFY(stepColumns.contains("start_on_tap"));
    QVERIFY(!stepColumns.contains("metronome_id"));
    QVERIFY(columnsOf(t.db, "exercise").contains("metronome_id"));
    QVERIFY(columnsOf(t.db, "exercise").contains("language"));
    QVERIFY(columnsOf(t.db, "workout").contains("colour"));
    QVERIFY(columnsOf(t.db, "challenge").contains("icon"));
    QVERIFY(integrityOk(t.db));
}

void TstDatabase::anOlderVersionIsRefusedUntouched()
{
    TestDb t;
    QVERIFY2(t.open(), qPrintable(t.error));
    QVERIFY(!addExercise(t.db, "Kept").isEmpty());
    // A file of an older version: nothing older than the first step exists before the release, so
    // it is refused with a reason and left as it is.
    QVERIFY(t.db.run("PRAGMA user_version = 9"));
    t.db.close();
    const QString path = inDir(t.dir, "workouttrainer.db");
    const QByteArray before = fileBytes(path);
    QVERIFY(!before.isEmpty());

    Database again;
    QString error;
    QVERIFY(!again.open(t.dir.path(), &error));
    QVERIFY(error.contains(QStringLiteral("older")));
    QVERIFY(!again.isOpen());
    QCOMPARE(fileBytes(path), before);
}

// Rewinds a database to version 11 and stores rhythms the way that version did: with an icon.
static void rewindToElevenWith(TestDb &t, const QList<QStringList> &rows)
{
    for (const QStringList &r : rows) {
        QVERIFY(t.db.run("INSERT INTO metronome (key, name, icon, colour, phases, created_at) VALUES (?, ?, ?, ?, '4;4', 1)",
                         QVariantList() << r.at(0) << r.at(1) << r.at(2) << r.at(3)));
    }
    QVERIFY(t.db.run("PRAGMA user_version = 11"));
    t.db.close();
}

void TstDatabase::versionElevenRhythmsLoseTheirIconAndKeepEveryRow()
{
    TestDb t;
    QVERIFY2(t.open(), qPrintable(t.error));
    // Four rhythms: three share one name and differ by icon or colour, the fourth has another name.
    rewindToElevenWith(t, QList<QStringList>()
        << (QStringList() << "m-1" << "Pace" << "wave" << "red")
        << (QStringList() << "m-2" << "pace" << "dot" << "blue")
        << (QStringList() << "m-3" << "Pace" << "" << "")
        << (QStringList() << "m-4" << "Other" << "wave" << "blue"));
    {
        Database again;
        QVERIFY2(again.open(t.dir.path(), &t.error), qPrintable(t.error));
        QCOMPARE(again.schemaVersion(), 12);
        QSqlQuery q;
        QVERIFY(again.run("SELECT key, name, icon, colour FROM metronome ORDER BY id", QVariantList(), &q));
        QStringList got;
        while (q.next())
            got << q.value(0).toString() + "|" + q.value(1).toString() + "|" + q.value(2).toString() + "|" + q.value(3).toString();
        // The lowest id keeps its name; the others take the copy suffix; nothing is lost; no icon or colour stays.
        QCOMPARE(got, QStringList() << "m-1|Pace||" << "m-2|pace (copy)||" << "m-3|Pace (copy 2)||" << "m-4|Other||");
        QVERIFY(integrityOk(again));
    }
    // A second opening changes nothing.
    Database third;
    QVERIFY(third.open(t.dir.path(), &t.error));
    QCOMPARE(third.schemaVersion(), 12);
    QCOMPARE(countRows(third, "metronome"), 4);
}

void TstDatabase::versionElevenWithoutClashesKeepsTheNames()
{
    TestDb t;
    QVERIFY2(t.open(), qPrintable(t.error));
    QVERIFY(!addExercise(t.db, "Squat").isEmpty());
    rewindToElevenWith(t, QList<QStringList>()
        << (QStringList() << "m-1" << "Slow" << "wave" << "red")
        << (QStringList() << "m-2" << "Fast" << "wave" << "red"));
    Database again;
    QVERIFY2(again.open(t.dir.path(), &t.error), qPrintable(t.error));
    QSqlQuery q;
    QVERIFY(again.run("SELECT name FROM metronome ORDER BY id", QVariantList(), &q));
    QStringList names;
    while (q.next())
        names << q.value(0).toString();
    QCOMPARE(names, QStringList() << "Slow" << "Fast");
    // The exercises are untouched by the migration.
    QCOMPARE(countRows(again, "exercise"), 1);
}

void TstDatabase::foreignKeysAreOnForEveryConnection()
{
    TestDb t;
    QVERIFY2(t.open(), qPrintable(t.error));
    QSqlQuery q;
    QVERIFY(t.db.run("PRAGMA foreign_keys", QVariantList(), &q) && q.next());
    QCOMPARE(q.value(0).toInt(), 1);
    // An exercise cannot point at a metronome that does not exist.
    QVERIFY(!t.db.run("INSERT INTO exercise (key, name, muscles, metronome_id) VALUES ('x', 'X', 16, 99)"));
}

QTEST_APPLESS_MAIN(TstDatabase)

#include "tst_database.moc"

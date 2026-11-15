// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef DATABASE_H
#define DATABASE_H

#include <QList>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <functional>

// Outcome of a store operation.
enum class StoreStatus {
    Ok,
    Invalid,   // the input failed validation; nothing was written
    NotFound,  // the addressed record does not exist; nothing was written
    InUse,     // the record is used by others and cannot be removed; nothing was written
    Failed     // the database reported an error; the write was rolled back
};

class Database;

// One numbered schema step. Applying it also sets the schema version to its number.
struct Migration {
    int version = 0;
    QStringList statements;
    // Optional code run after the statements, inside the same transaction. It converts stored
    // data that plain statements cannot; false aborts and rolls back the whole step.
    std::function<bool(Database *)> convert;
};

// One SQLite file, workouttrainer.db, in a directory given by the caller (R-NFR-3). Each instance
// has its own connection. The schema version is PRAGMA user_version (R-HST-4).
class Database
{
public:
    Database();
    ~Database();

    // Opens or creates the database and migrates it to the latest version. Returns false with an
    // error text when the file cannot be used; a file with a newer version is left untouched.
    bool open(const QString &directory, QString *error = nullptr);
    void close();
    bool isOpen() const;

    // Runs one statement with positional arguments; the result stays in *result when given.
    // On failure lastError() holds the database text (never the bound values).
    bool run(const QString &sql, const QVariantList &args = QVariantList(), QSqlQuery *result = nullptr);
    qint64 lastInsertId() const { return lastInsertId_; }
    QString lastError() const { return lastError_; }

    int schemaVersion();

    // Migrations added after the built-in ones; for tests. Set before open().
    void setExtraMigrations(const QList<Migration> &migrations) { extra_ = migrations; }
    static QList<Migration> builtinMigrations();
    static int builtinVersion();

private:
    QSqlDatabase handle() const;
    QList<Migration> allMigrations() const;
    bool fail(const QString &text, QString *error);
    void removeStaleFiles(const QString &directory) const;
    bool migrate(QString *error);

    friend class Transaction;

    QString connection_;
    int transactionDepth_ = 0;
    QList<Migration> extra_;
    qint64 lastInsertId_ = 0;
    QString lastError_;
};

// Text column value: a null string is stored as an empty string, never as NULL.
inline QVariant textValue(const QString &text)
{
    return text.isNull() ? QString(QLatin1String("")) : text;
}

// All-or-nothing group of statements. The destructor rolls back unless commit() succeeded.
// When a transaction is already open on the database, a new one is a savepoint inside it: its
// commit keeps the changes only if the outer transaction commits, its rollback undoes only
// its own changes.
class Transaction
{
public:
    explicit Transaction(Database *database);
    ~Transaction();

    bool isActive() const { return active_; }
    bool commit();

private:
    Database *database_;
    bool active_ = false;
    int level_ = 0;
};

#endif

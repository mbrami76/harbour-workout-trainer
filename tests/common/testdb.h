// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef TESTDB_H
#define TESTDB_H

#include <QTemporaryDir>
#include <QtTest>

#include "database.h"
#include "exercisestore.h"
#include "metronomestore.h"
#include "testsupport.h"

// QTemporaryDir::filePath needs Qt 5.9.
inline QString inDir(const QTemporaryDir &dir, const QString &name)
{
    return QDir(dir.path()).filePath(name);
}

// A database in a temporary directory that is removed with the object.
struct TestDb {
    QTemporaryDir dir;
    Database db;
    QString error;

    bool open() { return dir.isValid() && db.open(dir.path(), &error); }
};

inline bool integrityOk(Database &db)
{
    QSqlQuery q;
    if (!db.run(QStringLiteral("PRAGMA integrity_check"), QVariantList(), &q) || !q.next())
        return false;
    QSqlQuery fk;
    if (!db.run(QStringLiteral("PRAGMA foreign_key_check"), QVariantList(), &fk))
        return false;
    return q.value(0).toString() == QLatin1String("ok") && !fk.next();
}

inline int countRows(Database &db, const QString &table)
{
    QSqlQuery q;
    if (!db.run(QStringLiteral("SELECT COUNT(*) FROM ") + table, QVariantList(), &q) || !q.next())
        return -1;
    return q.value(0).toInt();
}

inline Exercise makeExercise(const QString &name, MuscleSet muscles = muscleBit(MuscleGroup::Chest))
{
    Exercise e;
    e.name = name;
    e.muscles = muscles;
    e.instructions = QStringLiteral("Do it slowly.");
    e.tips = QStringLiteral("Keep the back straight.");
    return e;
}

// Adds a user exercise and returns its key.
inline QString addExercise(Database &db, const QString &name, MuscleSet muscles = muscleBit(MuscleGroup::Chest))
{
    ExerciseStore store(&db);
    Exercise e = makeExercise(name, muscles);
    return store.create(&e) == StoreStatus::Ok ? e.key : QString();
}

// A rhythm of one phase as long as one repetition of the pace (the mean one for a total time), in
// half seconds and at most 10 s; a pace of the form with phases keeps its phases. For tests that
// need some valid rhythm and give a pace of the shape they know.
inline Tempo rhythmOf(const Tempo &pace)
{
    if (!pace.isValid())
        return Tempo();
    if (pace.form() == TempoForm::Phases)
        return Tempo::phases(pace.phaseUnitList(), 1, pace.phaseNameList());
    const qint64 repMs = (pace.setDurationMs() + pace.reps() / 2) / pace.reps();
    return Tempo::phases(QList<int>() << qBound(1, int((repMs + 250) / 500), 20), 1);
}

// Stores a rhythm (the phases of the given pace, see rhythmOf()) and returns its id, or 0.
inline int addMetronome(Database &db, const QString &name, const Tempo &tempo)
{
    SavedMetronome m;
    m.name = name;
    m.tempo = rhythmOf(tempo);
    int id = 0;
    return MetronomeStore(&db).create(m, &id) == StoreStatus::Ok ? id : 0;
}

inline bool sameStep(const Step &a, const Step &b)
{
    return a.type == b.type && a.exerciseKey == b.exerciseKey && a.durationSec == b.durationSec && a.reps == b.reps
        && a.startOnTap == b.startOnTap;
}

inline bool sameWorkout(const Workout &a, const Workout &b)
{
    if (a.name != b.name || a.icon != b.icon || a.colour != b.colour || a.restBetweenBlocksSec != b.restBetweenBlocksSec || a.blocks.size() != b.blocks.size())
        return false;
    for (int i = 0; i < a.blocks.size(); ++i) {
        const Block &x = a.blocks.at(i);
        const Block &y = b.blocks.at(i);
        if (x.rounds != y.rounds || x.restBetweenRoundsSec != y.restBetweenRoundsSec || x.steps.size() != y.steps.size())
            return false;
        for (int j = 0; j < x.steps.size(); ++j) {
            if (!sameStep(x.steps.at(j), y.steps.at(j)))
                return false;
        }
    }
    return true;
}

#endif

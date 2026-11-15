// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef METRONOMESTORE_H
#define METRONOMESTORE_H

#include <QList>

#include "database.h"
#include "tempo.h"

// A saved metronome, shown to the user as a rhythm (R-MET-2, R-MET-7): a named series of phases
// that make one repetition. It has no repetitions, sets or rest.
struct SavedMetronome {
    int id = 0;
    // Stable identifier, generated on create; it travels in export files and never changes.
    QString key;
    QString name;
    // The rhythm: the phases of one repetition (form Phases, one repetition). Whoever uses the
    // rhythm applies it to a number of repetitions with Tempo::forReps().
    Tempo tempo;
    // UTC seconds.
    qint64 createdAt = 0;
};

// Why a metronome is refused (R-MET-7, R-WRK-8).
struct MetronomeProblem {
    enum Code {
        NameEmpty,
        NameTooLong,
        TempoInvalid,       // the rhythm breaks a rule of R-MET-2; see tempoProblem
        IdentityTaken       // another metronome has the same name (R-MET-7)
    };
    Code code = NameEmpty;
    // The reason, for TempoInvalid only.
    TempoProblem tempoProblem = TempoProblem::None;
};

// An exercise that links a metronome (R-EXR-10).
struct ExerciseRef {
    QString key;
    QString name;
};

struct MetronomeSummary {
    int id = 0;
    QString key;
    QString name;
    // Length of one repetition: the sum of the phases.
    qint64 repetitionMs = 0;
    int phaseCount = 0;
    // Length of each phase in half seconds.
    QList<int> phaseUnits;
    QStringList phaseNames;
    // Number of exercises linked to the metronome.
    int exerciseCount = 0;
};

// Saved metronomes (rhythms) in the database (R-MET-7, R-MET-8). Like workouts, two metronomes may share a
// name only if they differ in more than letter case and surrounding blank space; a create or an edit that would make them
// equal is Invalid with MetronomeProblem::IdentityTaken and writes nothing. An edit that leaves
// the name as stored is not compared.
class MetronomeStore
{
public:
    explicit MetronomeStore(Database *database) : db_(database) {}

    // The stored text of a rhythm: the phases in half seconds joined by ";", and the phase names
    // percent-encoded and joined by ";" (empty when no phase has a name). rhythmFromColumns() is
    // the inverse; a text that cannot be read gives an invalid tempo.
    static QString phasesToColumn(const QList<int> &units);
    static QString namesToColumn(const QStringList &names);
    static Tempo rhythmFromColumns(const QString &phases, const QString &names);

    // Name and rhythm rules; the identity is checked on write.
    static QList<MetronomeProblem> validate(const SavedMetronome &metronome);

    // What saving the values would report, writing nothing: the rules of validate() and, when those
    // hold, the identity against the other metronomes (excludeId is the metronome being edited, 0
    // for a new one; an edit that leaves the name as stored is not compared). *failed
    // is set when the database reports an error.
    QList<MetronomeProblem> problemsOf(const SavedMetronome &metronome, int excludeId = 0, bool *failed = nullptr);

    // Stores a new metronome. The id is ignored. An empty key is generated; a given key (for
    // imports) must be unused, else the result is Invalid without a problem entry. A createdAt of
    // 0 means now.
    StoreStatus create(const SavedMetronome &metronome, int *id = nullptr, QList<MetronomeProblem> *problems = nullptr);
    bool get(int id, SavedMetronome *metronome);
    bool getByKey(const QString &key, SavedMetronome *metronome);
    // Replaces name and rhythm; key and creation time stay. When given,
    // *affected receives the exercises that use the metronome, so that the screen can tell the user
    // which exercises the change reaches (R-MET-8). The change applies to all of them at once.
    StoreStatus update(int id, const SavedMetronome &metronome, QList<MetronomeProblem> *problems = nullptr,
                       QList<ExerciseRef> *affected = nullptr);
    // The exercises linked to the metronome, sorted by name without regard to case, then key; the
    // count is the size of the list. Empty for a metronome that does not exist.
    QList<ExerciseRef> exercisesUsing(int id);
    // The exercises that would change if the metronome were saved: the same list. Call it before
    // update() to warn the user (R-MET-8).
    QList<ExerciseRef> impactOfEdit(int id);
    // Saves the given values as a new metronome and leaves the original and every exercise
    // unchanged (R-MET-8). A name whose identity is taken gets the first free suffix " (copy)",
    // " (copy 2)", ... Nothing is relinked.
    StoreStatus saveAsCopy(const SavedMetronome &changed, int *newId = nullptr, QList<MetronomeProblem> *problems = nullptr);
    // Independent copy with a new key and the first free name of "<name> (copy)", ...
    StoreStatus duplicate(int id, int *newId = nullptr);
    // Clears the link of every exercise that uses the metronome and deletes the metronome, in one
    // transaction (R-MET-8). It is never refused because of exercises: *cleared receives the
    // exercises that lost the link, which keep all their other data. If any step fails, nothing is
    // changed (Failed).
    StoreStatus remove(int id, QList<ExerciseRef> *cleared = nullptr);

    // Sorted by name without regard to case, then by id.
    QList<MetronomeSummary> list();

private:
    QList<MetronomeProblem> check(const SavedMetronome &metronome, int excludeId, bool *failed);
    bool exists(int id);
    static SavedMetronome fromRow(const QSqlQuery &q);
    Database *db_;
};

#endif

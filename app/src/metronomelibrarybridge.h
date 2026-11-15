// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef METRONOMELIBRARYBRIDGE_H
#define METRONOMELIBRARYBRIDGE_H

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class Services;

// The saved metronomes (R-MET-7, R-MET-8) for QML. Exposed as the context property
// "metronomeLibrary", created in main() with the Services. All store calls run on the main thread.
// The rules stay in the core: a screen asks problems() for the codes before it enables Save, and
// the store has the last word when it saves.
//
// A metronome (shown as a rhythm) is a map in the form of MetronomeForm (phases, phaseNames) with id,
// key and name. A result map has `status`: "ok",
// "invalid" (see `problems`, a list of codes), "not-found" or "failed". The codes of a problem are
// those of MetronomeForm::problemCode(), for example "name-empty", "identity" or "rep-short".
class MetronomeLibraryBridge : public QObject
{
    Q_OBJECT
    // [{id, name, repetitionMs, phaseCount, phaseUnits (half seconds), phaseNames, exerciseCount}],
    // by name without regard to case.
    Q_PROPERTY(QVariantList metronomes READ metronomes NOTIFY metronomesChanged)

public:
    explicit MetronomeLibraryBridge(Services *services, QObject *parent = nullptr);

    QVariantList metronomes() const { return metronomes_; }

    // Reads the list again.
    Q_INVOKABLE void refresh();
    // One saved metronome as a map with its exerciseCount; an empty map when it does not exist.
    Q_INVOKABLE QVariantMap get(int id);
    // The codes of what would refuse a save of the map, nothing written: the rules of the core and
    // the identity against the other metronomes. id is the metronome being edited, 0 for a new one.
    Q_INVOKABLE QStringList problems(int id, const QVariantMap &metronome);
    // Saves a new metronome: {status, id, problems}.
    Q_INVOKABLE QVariantMap create(const QVariantMap &metronome);
    // The exercises that use the metronome, by name: [{key, name}]. Ask before update().
    Q_INVOKABLE QVariantList exercisesUsing(int id);
    // Saves the map over the metronome, whatever exercises use it: {status, problems, affected}.
    Q_INVOKABLE QVariantMap update(int id, const QVariantMap &metronome);
    // Saves the map as a new metronome with the first free name suffix and leaves the original and
    // every exercise alone: {status, id, name, problems}.
    Q_INVOKABLE QVariantMap saveAsCopy(const QVariantMap &metronome);
    // An independent copy with the first free " (copy)" name: {status, id, name}.
    Q_INVOKABLE QVariantMap duplicate(int id);
    // Deletes the metronome, clears the links of its exercises in one operation and keeps the
    // exercises: {status, cleared} with the exercises that lost the link, [{key, name}].
    Q_INVOKABLE QVariantMap remove(int id);

signals:
    void metronomesChanged();

private:
    QVariantMap result(const QString &status) const;

    Services *services_;
    QVariantList metronomes_;
};

#endif

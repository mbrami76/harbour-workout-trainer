// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "metronomeform.h"

#include "workoutlimits.h"

namespace {

const QString formTotal = QStringLiteral("reps-total");
const QString formPerRep = QStringLiteral("time-per-rep");

}

Tempo MetronomeForm::tempoFromMap(const QVariantMap &c)
{
    const QString form = c.value(QStringLiteral("form")).toString();
    const int reps = c.value(QStringLiteral("reps")).toInt();
    if (form == formTotal)
        return Tempo::repsAndTotal(reps, qint64(c.value(QStringLiteral("totalSec")).toInt()) * 1000);
    if (form == formPerRep)
        return Tempo::timePerRep(qint64(c.value(QStringLiteral("repHalfSec")).toInt()) * Limits::halfSecondMs, reps);
    QList<int> units;
    for (const QVariant &v : c.value(QStringLiteral("phases")).toList())
        units.append(v.toInt());
    QStringList names;
    for (const QVariant &v : c.value(QStringLiteral("phaseNames")).toList())
        names.append(v.toString());
    return Tempo::phases(units, reps, names);
}

QVariantMap MetronomeForm::toMap(const SavedMetronome &m)
{
    QVariantMap map;
    map.insert(QStringLiteral("id"), m.id);
    map.insert(QStringLiteral("key"), m.key);
    map.insert(QStringLiteral("name"), m.name);
    QVariantList phases;
    QVariantList names;
    for (int unit : m.tempo.phaseUnitList())
        phases.append(unit);
    for (const QString &name : m.tempo.phaseNameList())
        names.append(name);
    map.insert(QStringLiteral("phases"), phases);
    map.insert(QStringLiteral("phaseNames"), names);
    return map;
}

SavedMetronome MetronomeForm::fromMap(const QVariantMap &map)
{
    SavedMetronome m;
    m.id = map.value(QStringLiteral("id")).toInt();
    m.key = map.value(QStringLiteral("key")).toString();
    m.name = map.value(QStringLiteral("name")).toString().trimmed();
    QList<int> units;
    for (const QVariant &v : map.value(QStringLiteral("phases")).toList())
        units.append(v.toInt());
    QStringList names;
    for (const QVariant &v : map.value(QStringLiteral("phaseNames")).toList())
        names.append(v.toString());
    m.tempo = Tempo::phases(units, 1, names);
    return m;
}

QString MetronomeForm::tempoProblemCode(TempoProblem problem)
{
    switch (problem) {
    case TempoProblem::None: return QString();
    case TempoProblem::RepsOutOfRange: return QStringLiteral("reps");
    case TempoProblem::RepTooShort: return QStringLiteral("rep-short");
    case TempoProblem::RepTooLong: return QStringLiteral("rep-long");
    case TempoProblem::PhaseCountOutOfRange: return QStringLiteral("phase-count");
    case TempoProblem::PhaseOutOfRange: return QStringLiteral("phase-length");
    case TempoProblem::PhaseNameTooLong: return QStringLiteral("phase-name");
    }
    return QStringLiteral("reps");
}

QString MetronomeForm::problemCode(const MetronomeProblem &problem)
{
    switch (problem.code) {
    case MetronomeProblem::NameEmpty: return QStringLiteral("name-empty");
    case MetronomeProblem::NameTooLong: return QStringLiteral("name-too-long");
    case MetronomeProblem::TempoInvalid: return tempoProblemCode(problem.tempoProblem);
    case MetronomeProblem::IdentityTaken: return QStringLiteral("identity");
    }
    return QStringLiteral("failed");
}

QStringList MetronomeForm::problemCodes(const QList<MetronomeProblem> &problems)
{
    QStringList codes;
    for (const MetronomeProblem &p : problems) {
        const QString code = problemCode(p);
        if (!codes.contains(code))
            codes.append(code);
    }
    return codes;
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef METRONOMEHELPERS_H
#define METRONOMEHELPERS_H

#include <QList>

#include "fakeclock.h"
#include "metronome.h"

typedef QList<MetronomeEvent> MetEvents;

inline MetronomeConfig makeConfig(const Tempo &tempo, int leadInSec, int sets, int restSec)
{
    MetronomeConfig c;
    c.tempo = tempo;
    c.leadInSec = leadInSec;
    c.sets = sets;
    c.restSec = restSec;
    return c;
}

inline int countOf(const MetEvents &events, MetronomeEventType type)
{
    int n = 0;
    for (const MetronomeEvent &e : events) {
        if (e.type == type)
            ++n;
    }
    return n;
}

inline MetEvents ofType(const MetEvents &events, MetronomeEventType type)
{
    MetEvents found;
    for (const MetronomeEvent &e : events) {
        if (e.type == type)
            found.append(e);
    }
    return found;
}

// Advances the clock in steps and collects the events of every update.
inline MetEvents drive(Metronome &m, FakeClock &clock, qint64 totalMs, qint64 stepMs)
{
    MetEvents all;
    for (qint64 t = 0; t < totalMs; t += stepMs) {
        clock.advance(qMin(stepMs, totalMs - t));
        all += m.update();
    }
    return all;
}

// A readable line of an event, to compare whole sequences.
inline QString describe(const MetronomeEvent &e)
{
    static const char *names[] = { "leadtick", "set", "rep", "beat", "setend", "rest", "resttick", "finished", "stopped" };
    return QStringLiteral("%1@%2 s%3 r%4 p%5 a%6 t%7").arg(QLatin1String(names[int(e.type)])).arg(e.atMs).arg(e.set)
        .arg(e.repetition).arg(e.phase).arg(e.accent ? 1 : 0).arg(e.remainingSec);
}

inline QStringList describeAll(const MetEvents &events)
{
    QStringList lines;
    for (const MetronomeEvent &e : events)
        lines.append(describe(e));
    return lines;
}

#endif

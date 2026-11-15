// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef ENGINEHELPERS_H
#define ENGINEHELPERS_H

#include <QScopedPointer>
#include <QStringList>

#include "engine.h"
#include "fakeclock.h"
#include "sessionlog.h"
#include "testsupport.h"

typedef QList<EngineEvent> EngEvents;

// An engine on a fake clock that collects every event.
struct EngineRig {
    // A new rig starts with no exercise tempos; tempoStep() fills them while the workout is built.
    EngineRig() { tempoSnapshot().clear(); }
    FakeClock clock;
    QScopedPointer<SessionEngine> engine;
    EngEvents log;

    void make(const Workout &w, int leadInSec = 0, const QList<int> &ticks = QList<int>() << 10 << 5 << 3 << 2 << 1)
    {
        EngineConfig config;
        config.leadInSec = leadInSec;
        config.tickSeconds = ticks;
        engine.reset(new SessionEngine(&clock, w, tempoSnapshot(), config));
        clock.set(0);
        log.clear();
    }
    void take(const EngEvents &events) { log += events; }
    void at(qint64 ms)
    {
        clock.set(ms);
        take(engine->update());
    }
    // Moves the clock in steps up to ms, calling update() after each one.
    void runTo(qint64 ms, qint64 step)
    {
        while (clock.nowMs() < ms) {
            clock.set(qMin(clock.nowMs() + step, ms));
            take(engine->update());
        }
    }
    bool is(SessionState state) const { return engine->state() == state; }
};

inline int countOf(const EngEvents &events, EventType type)
{
    int n = 0;
    for (const EngineEvent &e : events) {
        if (e.type == type)
            ++n;
    }
    return n;
}

inline EngEvents ofType(const EngEvents &events, EventType type)
{
    EngEvents found;
    for (const EngineEvent &e : events) {
        if (e.type == type)
            found.append(e);
    }
    return found;
}

inline QString describe(const EngineEvent &e)
{
    static const char *names[] = { "step", "tick", "round", "finished", "stopped", "wait", "beat" };
    QString text = QStringLiteral("%1@%2 i%3").arg(QLatin1String(names[int(e.type)])).arg(e.atMs).arg(e.stepIndex);
    if (e.type == EventType::Beat)
        text += QStringLiteral(" r%1 p%2 a%3").arg(e.repetition).arg(e.phase).arg(e.accent ? 1 : 0);
    if (e.type == EventType::Tick)
        text += QStringLiteral(" t%1%2").arg(e.remainingSec).arg(e.inLeadIn ? "L" : "");
    return text;
}

inline QStringList describeAll(const EngEvents &events)
{
    QStringList lines;
    for (const EngineEvent &e : events)
        lines.append(describe(e));
    return lines;
}

// Times of the events of one type.
inline QList<qint64> timesOf(const EngEvents &events, EventType type)
{
    QList<qint64> times;
    for (const EngineEvent &e : events) {
        if (e.type == type)
            times.append(e.atMs);
    }
    return times;
}

#endif

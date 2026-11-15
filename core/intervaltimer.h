// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef INTERVALTIMER_H
#define INTERVALTIMER_H

#include <QList>
#include <QSet>

#include "clock.h"

enum class PeriodKind { Work, Recovery };

enum class IntervalState { Ready, Running, Paused, Stopped };

enum class IntervalEventType {
    PeriodStarted,  // every period, the first one included
    PeriodChanged,  // sent before PeriodStarted when the kind differs from the previous period
    Tick,           // a listed number of seconds is left in the period
    Stopped
};

// atMs is the scheduled time of the event on the clock, not the time of the call that reported
// it. periodIndex counts the periods from 0. remainingSec is set for ticks only.
struct IntervalEvent {
    IntervalEventType type = IntervalEventType::PeriodStarted;
    qint64 atMs = 0;
    PeriodKind kind = PeriodKind::Work;
    int periodIndex = 0;
    int remainingSec = 0;
};

// Endless work and recovery periods (R-MOD-2). The work period comes first and starts at once,
// without a lead-in. With a recovery of 0 seconds, work periods follow each other. It owns no
// timer: the caller invokes update() periodically and after every user action. Period ends are
// computed from the start time and the durations, so the periods do not drift however often
// update() is called (R-TMR-5). Every call returns the events it produced, in order.
class IntervalTimer
{
public:
    static const int minWorkSec = 1;
    static const int maxWorkSec = 3600;
    static const int minRecoverySec = 0;
    static const int maxRecoverySec = 600;

    static bool isValid(int workSec, int recoverySec);

    // Tick seconds as in the session engine: whole seconds from 1, each ticks once per period,
    // and only if the period is longer than that.
    IntervalTimer(const Clock *clock, int workSec, int recoverySec,
                  const QList<int> &tickSeconds = QList<int>() << 10 << 5 << 3 << 2 << 1);

    // False when the durations are outside the limits; start() then does nothing.
    bool valid() const { return valid_; }

    QList<IntervalEvent> start();
    QList<IntervalEvent> pause();
    QList<IntervalEvent> resume();
    QList<IntervalEvent> stop();
    QList<IntervalEvent> update();

    IntervalState state() const { return state_; }
    PeriodKind periodKind() const { return kind_; }
    // Number of periods started so far; the current period has the index periodCount() - 1.
    int periodCount() const { return periodCount_; }
    // Work periods that ran to their end.
    int completedWorkPeriods() const { return completedWork_; }
    qint64 periodRemainingMs() const;
    qint64 periodElapsedMs() const;
    qint64 periodDurationMs() const { return durationOf(kind_); }

private:
    qint64 durationOf(PeriodKind kind) const;
    void beginPeriod(PeriodKind kind, qint64 at, QList<IntervalEvent> &events);
    void emitTicks(qint64 now, QList<IntervalEvent> &events);
    void advance(qint64 now, QList<IntervalEvent> &events);
    IntervalEvent event(IntervalEventType type, qint64 at) const;

    const Clock *clock_;
    qint64 workMs_;
    qint64 recoveryMs_;
    bool valid_;
    QList<int> ticks_;

    IntervalState state_ = IntervalState::Ready;
    PeriodKind kind_ = PeriodKind::Work;
    int periodCount_ = 0;
    int completedWork_ = 0;
    qint64 periodEnd_ = 0;
    qint64 tickBase_ = 0;
    qint64 remainingAtPause_ = 0;
    QSet<int> ticked_;
};

#endif

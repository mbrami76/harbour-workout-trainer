// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef METRONOME_H
#define METRONOME_H

#include <QList>
#include <QSet>

#include "clock.h"
#include "tempo.h"

enum class MetronomeState { Ready, LeadIn, Running, Resting, Paused, Finished, Stopped };

enum class MetronomeEventType {
    LeadInTick,        // seconds left in the lead-in (remainingSec)
    SetStarted,        // set
    RepetitionStarted, // set, repetition
    Beat,              // set, repetition, phase (0 without phases), accent, beatKind
    SetEnded,          // set: the end cue
    RestStarted,       // set (the set that has just ended)
    RestTick,          // seconds left in the rest (remainingSec)
    Finished,
    Stopped
};

// atMs is the scheduled time of the event on the clock, not the time of the call that reported
// it. set and repetition count from 1; they are 0 where they do not apply.
struct MetronomeEvent {
    MetronomeEventType type = MetronomeEventType::Beat;
    qint64 atMs = 0;
    int set = 0;
    int repetition = 0;
    int phase = 0;
    bool accent = false;
    BeatKind beatKind = BeatKind::First;
    int remainingSec = 0;
};

struct MetronomeConfig {
    Tempo tempo;
    int leadInSec = 3;
    int sets = 1;
    int restSec = 0;
    // Remaining whole seconds at which a tick is emitted in the lead-in and in a rest.
    QList<int> tickSeconds = QList<int>() << 10 << 5 << 3 << 2 << 1;
};

// Metronome run (R-MET-1 to R-MET-6): lead-in, sets of repetitions and rests between the sets,
// without a rest after the last set. It owns no thread and no timer: the caller invokes update()
// periodically and after every user action, and receives the events that are due, in order and
// once. Every time inside the run is computed from the start of the set or the rest and the
// durations, never from the number of calls, so the pace does not drift (R-MET-5).
//
// Events at the same instant come in this order: set ended, rest started or finished, set
// started, repetition started, beat. The first beat of a repetition is accented.
class Metronome
{
public:
    Metronome(const Clock *clock, const MetronomeConfig &config);

    // False when the tempo, the lead-in, the number of sets or the rest are outside the limits;
    // start() then does nothing.
    bool valid() const { return valid_; }
    static bool isValid(const MetronomeConfig &config);

    QList<MetronomeEvent> start();
    // Freezes the run. A resume continues the lead-in or the rest where they were, and restarts
    // the current repetition at its first beat: the part of the repetition before the pause does
    // not count as elapsed.
    QList<MetronomeEvent> pause();
    QList<MetronomeEvent> resume();
    QList<MetronomeEvent> stop();
    QList<MetronomeEvent> update();

    // Live changes of this run (R-MET-6). addSet() inserts one more set, with its rest, after
    // the current one; it fails at 99 sets. removeSet() drops the next set that has not started,
    // and the rest before it; it fails if there is none. The running set (in the lead-in, the
    // first set) is never removed. During a rest, removing the last remaining set ends the run
    // at once. Both fail when the run is finished or stopped. Events that fall due while the
    // change is made are returned by the next call that returns events.
    bool addSet();
    bool removeSet();

    MetronomeState state() const { return state_; }
    // Number of the current set (the set just ended while resting), from 1; 0 when ready.
    int setNumber() const;
    int setCount() const { return sets_; }
    // Sets after the current one.
    int setsLeft() const;
    int repetitionCount() const { return tempo_.reps(); }
    // Current repetition from 1, or 0 outside a set.
    int repetitionNumber() const;
    // Phase number 1 to 8 of the running phase, or 0 without phases or outside a set.
    int phaseNumber() const;
    // Share of the current repetition and of the current phase that has elapsed, 0 to 1; 0
    // outside a set. Without phases both are the same.
    double repetitionFraction() const;
    double phaseFraction() const;
    qint64 leadInRemainingMs() const;
    qint64 restRemainingMs() const;
    // Time left until the run ends if nothing is paused or changed. For a paused set it counts
    // the current repetition from its start.
    qint64 remainingMs() const;

private:
    enum class Phase { Ready, LeadIn, Set, Rest, Done };

    void settle(QList<MetronomeEvent> &events);
    void advance(qint64 now, QList<MetronomeEvent> &events);
    QList<MetronomeEvent> take();
    void emitTicks(qint64 endMs, qint64 baseMs, QSet<int> &done, MetronomeEventType type, qint64 now,
                   QList<MetronomeEvent> &events);
    void beginSet(qint64 at, QList<MetronomeEvent> &events);
    void emitBeats(qint64 now, QList<MetronomeEvent> &events);
    void endSet(QList<MetronomeEvent> &events);
    void finish(qint64 at, QList<MetronomeEvent> &events);
    MetronomeEvent event(MetronomeEventType type, qint64 at) const;
    // Position inside the set at time now, clamped to the set.
    qint64 setOffset(qint64 now) const;
    qint64 setDurationMs() const { return tempo_.setDurationMs(); }
    qint64 timeAfterCurrentSet() const;

    const Clock *clock_;
    Tempo tempo_;
    qint64 leadInMs_;
    qint64 restMs_;
    int sets_;
    bool valid_;
    QList<int> ticks_;

    MetronomeState state_ = MetronomeState::Ready;
    Phase phase_ = Phase::Ready;
    Phase pausedIn_ = Phase::Ready;
    int setIndex_ = 0;
    qint64 leadEnd_ = 0;
    qint64 setStart_ = 0;
    qint64 restEnd_ = 0;
    int nextRep_ = 0;
    int nextBeat_ = 0;
    int pausedRep_ = 0;
    qint64 frozenMs_ = 0;
    QSet<int> tickedLead_;
    QSet<int> tickedRest_;
    QList<MetronomeEvent> pending_;
};

#endif

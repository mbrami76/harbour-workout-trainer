// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "metronome.h"

#include <algorithm>
#include <functional>

#include "workoutlimits.h"

bool Metronome::isValid(const MetronomeConfig &config)
{
    return config.tempo.isValid()
        && config.leadInSec >= Limits::minLeadInSec && config.leadInSec <= Limits::maxLeadInSec
        && config.sets >= Limits::minRounds && config.sets <= Limits::maxRounds
        && config.restSec >= Limits::minRestSec && config.restSec <= Limits::maxRestSec;
}

Metronome::Metronome(const Clock *clock, const MetronomeConfig &config)
    : clock_(clock),
      tempo_(config.tempo),
      leadInMs_(qint64(config.leadInSec) * 1000),
      restMs_(qint64(config.restSec) * 1000),
      sets_(config.sets),
      valid_(isValid(config))
{
    for (int seconds : config.tickSeconds) {
        if (seconds >= 1 && !ticks_.contains(seconds))
            ticks_.append(seconds);
    }
    std::sort(ticks_.begin(), ticks_.end(), std::greater<int>());
}

MetronomeEvent Metronome::event(MetronomeEventType type, qint64 at) const
{
    MetronomeEvent e;
    e.type = type;
    e.atMs = at;
    e.set = setIndex_ + 1;
    return e;
}

QList<MetronomeEvent> Metronome::take()
{
    QList<MetronomeEvent> events = pending_;
    pending_.clear();
    return events;
}

void Metronome::emitTicks(qint64 endMs, qint64 baseMs, QSet<int> &done, MetronomeEventType type, qint64 now,
                          QList<MetronomeEvent> &events)
{
    for (int seconds : ticks_) {
        const qint64 remaining = qint64(seconds) * 1000;
        if (remaining >= baseMs || done.contains(seconds))
            continue;
        const qint64 at = endMs - remaining;
        if (at > now)
            break;
        MetronomeEvent e = event(type, at);
        e.remainingSec = seconds;
        events.append(e);
        done.insert(seconds);
    }
}

void Metronome::beginSet(qint64 at, QList<MetronomeEvent> &events)
{
    state_ = MetronomeState::Running;
    phase_ = Phase::Set;
    setStart_ = at;
    nextRep_ = 0;
    nextBeat_ = 0;
    events.append(event(MetronomeEventType::SetStarted, at));
}

void Metronome::emitBeats(qint64 now, QList<MetronomeEvent> &events)
{
    while (nextRep_ < tempo_.reps()) {
        const qint64 at = setStart_ + tempo_.repStartMs(nextRep_) + tempo_.beatOffsetMs(nextBeat_);
        if (at > now)
            break;
        if (nextBeat_ == 0) {
            MetronomeEvent started = event(MetronomeEventType::RepetitionStarted, at);
            started.repetition = nextRep_ + 1;
            events.append(started);
        }
        MetronomeEvent beat = event(MetronomeEventType::Beat, at);
        beat.repetition = nextRep_ + 1;
        beat.phase = tempo_.beatPhase(nextBeat_);
        beat.accent = Tempo::isAccent(nextBeat_);
        beat.beatKind = tempo_.beatKind(nextBeat_);
        events.append(beat);
        if (++nextBeat_ >= tempo_.beatsPerRep()) {
            nextBeat_ = 0;
            ++nextRep_;
        }
    }
}

void Metronome::finish(qint64 at, QList<MetronomeEvent> &events)
{
    state_ = MetronomeState::Finished;
    phase_ = Phase::Done;
    events.append(event(MetronomeEventType::Finished, at));
}

void Metronome::endSet(QList<MetronomeEvent> &events)
{
    const qint64 at = setStart_ + setDurationMs();
    events.append(event(MetronomeEventType::SetEnded, at));
    if (setIndex_ + 1 >= sets_) {
        finish(at, events);
    } else if (restMs_ > 0) {
        state_ = MetronomeState::Resting;
        phase_ = Phase::Rest;
        restEnd_ = at + restMs_;
        tickedRest_.clear();
        events.append(event(MetronomeEventType::RestStarted, at));
    } else {
        ++setIndex_;
        beginSet(at, events);
    }
}

void Metronome::advance(qint64 now, QList<MetronomeEvent> &events)
{
    for (;;) {
        if (state_ == MetronomeState::LeadIn) {
            emitTicks(leadEnd_, leadInMs_, tickedLead_, MetronomeEventType::LeadInTick, now, events);
            if (leadEnd_ > now)
                return;
            beginSet(leadEnd_, events);
        } else if (state_ == MetronomeState::Running) {
            emitBeats(now, events);
            if (setStart_ + setDurationMs() > now)
                return;
            endSet(events);
        } else if (state_ == MetronomeState::Resting) {
            emitTicks(restEnd_, restMs_, tickedRest_, MetronomeEventType::RestTick, now, events);
            if (restEnd_ > now)
                return;
            ++setIndex_;
            beginSet(restEnd_, events);
        } else {
            return;
        }
    }
}

QList<MetronomeEvent> Metronome::start()
{
    QList<MetronomeEvent> events = take();
    if (state_ != MetronomeState::Ready || !valid_)
        return events;
    const qint64 now = clock_->nowMs();
    setIndex_ = 0;
    if (leadInMs_ > 0) {
        state_ = MetronomeState::LeadIn;
        phase_ = Phase::LeadIn;
        leadEnd_ = now + leadInMs_;
        tickedLead_.clear();
    } else {
        beginSet(now, events);
    }
    advance(now, events);
    return events;
}

QList<MetronomeEvent> Metronome::pause()
{
    QList<MetronomeEvent> events = take();
    const qint64 now = clock_->nowMs();
    advance(now, events);
    switch (state_) {
    case MetronomeState::LeadIn:
        frozenMs_ = leadEnd_ - now;
        break;
    case MetronomeState::Running:
        pausedRep_ = tempo_.repIndexAt(now - setStart_);
        break;
    case MetronomeState::Resting:
        frozenMs_ = restEnd_ - now;
        break;
    default:
        return events;
    }
    pausedIn_ = phase_;
    state_ = MetronomeState::Paused;
    return events;
}

QList<MetronomeEvent> Metronome::resume()
{
    QList<MetronomeEvent> events = take();
    if (state_ != MetronomeState::Paused)
        return events;
    const qint64 now = clock_->nowMs();
    switch (pausedIn_) {
    case Phase::LeadIn:
        state_ = MetronomeState::LeadIn;
        leadEnd_ = now + frozenMs_;
        break;
    case Phase::Set:
        state_ = MetronomeState::Running;
        // The repetition starts again at its first beat: the set moves so that it began now.
        setStart_ = now - tempo_.repStartMs(pausedRep_);
        nextRep_ = pausedRep_;
        nextBeat_ = 0;
        break;
    default:
        state_ = MetronomeState::Resting;
        restEnd_ = now + frozenMs_;
        break;
    }
    phase_ = pausedIn_;
    advance(now, events);
    return events;
}

QList<MetronomeEvent> Metronome::stop()
{
    QList<MetronomeEvent> events = take();
    const qint64 now = clock_->nowMs();
    advance(now, events);
    if (state_ == MetronomeState::Ready || state_ == MetronomeState::Finished || state_ == MetronomeState::Stopped)
        return events;
    state_ = MetronomeState::Stopped;
    phase_ = Phase::Done;
    events.append(event(MetronomeEventType::Stopped, now));
    return events;
}

QList<MetronomeEvent> Metronome::update()
{
    QList<MetronomeEvent> events = take();
    advance(clock_->nowMs(), events);
    return events;
}

bool Metronome::addSet()
{
    advance(clock_->nowMs(), pending_);
    if (state_ == MetronomeState::Finished || state_ == MetronomeState::Stopped || sets_ >= Limits::maxRounds)
        return false;
    ++sets_;
    return true;
}

bool Metronome::removeSet()
{
    const qint64 now = clock_->nowMs();
    advance(now, pending_);
    if (state_ == MetronomeState::Finished || state_ == MetronomeState::Stopped || setsLeft() <= 0)
        return false;
    const bool resting = state_ == MetronomeState::Resting
        || (state_ == MetronomeState::Paused && pausedIn_ == Phase::Rest);
    const bool last = setsLeft() == 1;
    --sets_;
    // The rest that is running cannot be removed: with no set after it, the run ends now.
    if (resting && last)
        finish(now, pending_);
    return true;
}

int Metronome::setNumber() const
{
    return state_ == MetronomeState::Ready ? 0 : setIndex_ + 1;
}

int Metronome::setsLeft() const
{
    if (state_ == MetronomeState::Finished || state_ == MetronomeState::Stopped)
        return 0;
    return qMax(0, sets_ - setIndex_ - 1);
}

qint64 Metronome::setOffset(qint64 now) const
{
    return qBound<qint64>(0, now - setStart_, setDurationMs());
}

int Metronome::repetitionNumber() const
{
    if (state_ == MetronomeState::Running)
        return tempo_.repIndexAt(setOffset(clock_->nowMs())) + 1;
    if (state_ == MetronomeState::Paused && pausedIn_ == Phase::Set)
        return pausedRep_ + 1;
    return 0;
}

int Metronome::phaseNumber() const
{
    if (state_ == MetronomeState::Running) {
        const qint64 offset = setOffset(clock_->nowMs());
        const int rep = tempo_.repIndexAt(offset);
        return tempo_.phaseAt(offset - tempo_.repStartMs(rep));
    }
    if (state_ == MetronomeState::Paused && pausedIn_ == Phase::Set)
        return tempo_.beatPhase(0);
    return 0;
}

double Metronome::repetitionFraction() const
{
    if (state_ != MetronomeState::Running)
        return 0;
    const qint64 offset = setOffset(clock_->nowMs());
    const int rep = tempo_.repIndexAt(offset);
    return double(offset - tempo_.repStartMs(rep)) / double(tempo_.repDurationMs(rep));
}

double Metronome::phaseFraction() const
{
    if (state_ != MetronomeState::Running)
        return 0;
    const qint64 offset = setOffset(clock_->nowMs());
    const int rep = tempo_.repIndexAt(offset);
    const qint64 inRep = offset - tempo_.repStartMs(rep);
    const int beat = tempo_.beatIndexAt(inRep);
    return double(inRep - tempo_.beatOffsetMs(beat)) / double(tempo_.beatSpanMs(beat, rep));
}

qint64 Metronome::leadInRemainingMs() const
{
    if (state_ == MetronomeState::LeadIn)
        return qMax<qint64>(0, leadEnd_ - clock_->nowMs());
    if (state_ == MetronomeState::Paused && pausedIn_ == Phase::LeadIn)
        return frozenMs_;
    return 0;
}

qint64 Metronome::restRemainingMs() const
{
    if (state_ == MetronomeState::Resting)
        return qMax<qint64>(0, restEnd_ - clock_->nowMs());
    if (state_ == MetronomeState::Paused && pausedIn_ == Phase::Rest)
        return frozenMs_;
    return 0;
}

// Each set after the current one, with the rest before it.
qint64 Metronome::timeAfterCurrentSet() const
{
    return qint64(setsLeft()) * (restMs_ + setDurationMs());
}

qint64 Metronome::remainingMs() const
{
    const qint64 now = clock_->nowMs();
    const qint64 after = timeAfterCurrentSet();
    switch (state_) {
    case MetronomeState::Ready:
        return leadInMs_ + setDurationMs() + after;
    case MetronomeState::LeadIn:
        return qMax<qint64>(0, leadEnd_ - now) + setDurationMs() + after;
    case MetronomeState::Running:
        return qMax<qint64>(0, setStart_ + setDurationMs() - now) + after;
    case MetronomeState::Resting:
        return qMax<qint64>(0, restEnd_ - now) + after - restMs_;
    case MetronomeState::Paused:
        if (pausedIn_ == Phase::LeadIn)
            return frozenMs_ + setDurationMs() + after;
        if (pausedIn_ == Phase::Set)
            return setDurationMs() - tempo_.repStartMs(pausedRep_) + after;
        return frozenMs_ + after - restMs_;
    default:
        return 0;
    }
}

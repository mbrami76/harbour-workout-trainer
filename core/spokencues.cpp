// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "spokencues.h"

namespace SpokenCues {

QList<Cue> periodCues(qint64 durationMs)
{
    QList<Cue> list;
    if (durationMs >= halfwayMinPeriodMs) {
        Cue c;
        c.kind = Kind::Halfway;
        c.atMs = durationMs / 2;
        list.append(c);
    }
    if (durationMs > tenSecondsMs) {
        Cue c;
        c.kind = Kind::TenSecondsLeft;
        c.atMs = durationMs - tenSecondsMs;
        list.append(c);
    }
    return list;
}

QList<Cue> tempoCues(const Tempo &tempo)
{
    QList<Cue> list;
    const int reps = tempo.reps();
    if (reps >= middleCallMinReps) {
        Cue c;
        c.kind = Kind::ToGo;
        c.repetition = (reps + 1) / 2;
        c.count = reps - c.repetition + 1;
        c.atMs = tempo.repStartMs(c.repetition - 1);
        list.append(c);
    }
    if (reps > lastCallCount) {
        Cue c;
        c.kind = Kind::ThreeToGo;
        c.repetition = reps - lastCallCount + 1;
        c.count = lastCallCount;
        c.atMs = tempo.repStartMs(c.repetition - 1);
        list.append(c);
    }
    return list;
}

PhaseNames phaseNamesFromInt(int value)
{
    switch (value) {
    case 0: return PhaseNames::None;
    case 1: return PhaseNames::First;
    case 2: return PhaseNames::FirstThree;
    case 3: return PhaseNames::All;
    default: return defaultPhaseNames;
    }
}

bool phaseNameSpoken(PhaseNames names, int repetition)
{
    switch (names) {
    case PhaseNames::None: return false;
    case PhaseNames::First: return repetition == 1;
    case PhaseNames::FirstThree: return repetition >= 1 && repetition <= 3;
    case PhaseNames::All: return repetition >= 1;
    }
    return false;
}

}

void CueArbiter::setAnnouncing(bool on)
{
    announcing_ = on;
    if (on) {
        // An announcement has the voice: what a cue was saying is stopped by the caller.
        nameSpeaking_ = false;
    }
}

void CueArbiter::nameEnded()
{
    nameSpeaking_ = false;
}

void CueArbiter::reset()
{
    announcing_ = false;
    nameSpeaking_ = false;
}

CueArbiter::Order CueArbiter::onPhaseStart(bool nameToSpeak)
{
    Order order;
    order.cutName = nameSpeaking_;
    nameSpeaking_ = false;
    if (announcing_ || !nameToSpeak)
        return order;
    order.speak = true;
    nameSpeaking_ = true;
    return order;
}

CueArbiter::Order CueArbiter::onPeriodCue()
{
    Order order;
    if (announcing_)
        return order;
    order.speak = true;
    return order;
}

CueArbiter::Order CueArbiter::onCountCue(bool nameAtStart)
{
    Order order;
    if (announcing_ || nameSpeaking_ || nameAtStart)
        return order;
    order.speak = true;
    return order;
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef SPOKENCUES_H
#define SPOKENCUES_H

#include <QList>
#include <QtGlobal>

#include "tempo.h"

// The cues the voice speaks during a step (R-ALR-15). Pure logic: which cues a step has, when they
// fall, which phase names are spoken and what is skipped when two things would be spoken at once.
namespace SpokenCues {

// A period with a set time and no tempo: "Halfway" from this length on, at the middle (R-UI-7).
const qint64 halfwayMinPeriodMs = 45000;
// "Ten seconds left" ten seconds before the end of a period longer than that (R-UI-7).
const qint64 tenSecondsMs = 10000;
// A tempo step with at least this many repetitions has the call at its middle repetition.
const int middleCallMinReps = 6;
// The call before the last three repetitions; a step needs more repetitions than this to have it.
const int lastCallCount = 3;

enum class Kind {
    Halfway,        // the middle of a period
    TenSecondsLeft, // ten seconds before the end of a period
    ToGo,           // "N to go", with the number of repetitions left
    ThreeToGo       // "Three to go"
};

// A cue of a step. For a period `atMs` is the time from the start of the step; for a tempo step the
// cue falls at the start of repetition `repetition` (from 1), `atMs` being that start.
struct Cue {
    Kind kind = Kind::Halfway;
    qint64 atMs = 0;
    int repetition = 0;
    // ToGo only: the repetitions left, the one that starts included.
    int count = 0;

    bool operator==(const Cue &o) const { return kind == o.kind && atMs == o.atMs && repetition == o.repetition && count == o.count; }
};

// The cues of a period of the given length (a step by duration, a rest, a period of the interval
// timer, the countdown), at the times of the halo cues: none for a short period.
QList<Cue> periodCues(qint64 durationMs);

// The repetitions-left calls of a tempo step (R-ALR-15 b): "N to go" at the middle repetition of a
// step of 6 repetitions or more, and "Three to go" at the repetition that begins the last three.
// The middle repetition is ceil(reps / 2); the number is the repetitions left with the one that
// starts. A step of 3 repetitions or fewer has no last call (it would fall at the start).
QList<Cue> tempoCues(const Tempo &tempo);

// How many repetitions of a step have their phase names spoken (R-ALR-15 c).
enum class PhaseNames { None = 0, First = 1, FirstThree = 2, All = 3 };

// The stored value of a setting as the choice it stands for; anything unknown is the default.
PhaseNames phaseNamesFromInt(int value);
const PhaseNames defaultPhaseNames = PhaseNames::FirstThree;

// Whether the names of the phases are spoken in repetition `repetition` (from 1, counted from the
// start of the step).
bool phaseNameSpoken(PhaseNames names, int repetition);

// What the user chose in the settings.
struct Settings {
    // The switch "Spoken cues" of (a) and (b).
    bool cues = true;
    PhaseNames phaseNames = defaultPhaseNames;

    static Settings off()
    {
        Settings s;
        s.cues = false;
        s.phaseNames = PhaseNames::None;
        return s;
    }
};

}

// Decides what happens when something is to be spoken while the voice is busy (R-ALR-15 d): a cue
// never interrupts an announcement; a phase name is cut by the next phase and takes precedence over
// the repetitions-left calls, which are skipped, not delayed. One per run; the caller says what the
// voice is doing.
class CueArbiter
{
public:
    // An announcement (R-ALR-6, R-ALR-11) is being spoken, or is over.
    void setAnnouncing(bool on);
    // The voice has finished the name it was speaking, or it was stopped.
    void nameEnded();
    void reset();

    bool announcing() const { return announcing_; }
    bool nameSpeaking() const { return nameSpeaking_; }

    struct Order {
        // True when the text is to be spoken.
        bool speak = false;
        // True when the name that is being spoken is cut first.
        bool cutName = false;
    };

    // A phase starts. Its name, when it has one that the setting selects, is spoken; the name of the
    // phase before is cut in every case.
    Order onPhaseStart(bool nameToSpeak);
    // A period cue (halfway, ten seconds left) falls.
    Order onPeriodCue();
    // A repetitions-left call falls at the start of a repetition; `nameAtStart` tells whether the
    // phase that starts then has a name that is to be spoken.
    Order onCountCue(bool nameAtStart);

private:
    bool announcing_ = false;
    bool nameSpeaking_ = false;
};

#endif

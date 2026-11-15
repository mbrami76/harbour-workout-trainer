// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef SPEECH_H
#define SPEECH_H

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <functional>

#include "engine.h"
#include "plan.h"
#include "spokencues.h"
#include "speechengine.h"

// Spoken announcements (R-ALR-6, R-ALR-10, R-ALR-11, R-ALR-13, R-ALR-14): what is said, in which language
// and in which order. Pure logic: nothing here plays or synthesises audio.

namespace SpeechTiming {

// Time between the end of the audio of an announcement and the start of the step timer, in
// milliseconds. 0 starts the timer when the last audio has been played. It is the only value that
// shifts the voice against the timer: it is to be set after a listening test on the phone, where the
// audio path has a delay of its own.
const int voiceToTimerGapMs = 0;

// Pauses of the voice (R-ALR-14), in milliseconds. They are made of silence put into the audio, so
// they are the same for every voice. To be set after a listening test on the phone.
//
// After the end of a sentence (full stop, question mark or exclamation mark) inside a text.
const int sentencePauseMs = 600;
// After a comma, a semicolon or a colon inside a text.
const int clausePauseMs = 300;
// Between two texts of one announcement: after the sentence of a rest, and between the head
// ("Get ready to perform X. 30 seconds.") and the description. None follows the last text.
const int betweenTextsMs = 600;

// A rest shorter than this has no rest sentence, and from `restInMinutes` seconds on its length is
// spoken as minutes and seconds (R-ALR-11).
const int minRestSentenceSec = 5;
const int restInMinutesSec = 90;

// How far the preparation works ahead, in positions of the plan: the announcement being spoken, the
// next step and the one after it, so that the whole of the next step is covered (R-ALR-10).
const int lookaheadSteps = 3;

}

// A Cue is a cue spoken during a step (R-ALR-15): a short text that is kept for the whole run and
// is made without pauses inside it.
enum class UtteranceKind { RestSentence, Announcement, Description, Cue };

// One text to speak, in the language of the text it belongs to (R-EXR-12). The exercise key is
// empty for a sentence that belongs to no exercise.
struct Utterance {
    UtteranceKind kind = UtteranceKind::Announcement;
    QString text;
    QString language;
    QString exerciseKey;

    bool isEmpty() const { return text.trimmed().isEmpty(); }
    bool operator==(const Utterance &o) const { return kind == o.kind && text == o.text && language == o.language; }
};

// What one Announce event of the engine says, in order: the sentence of a rest and the
// announcement of the step after it, or the announcement of a step. `planIndex` is the step at
// which the speech begins (the rest, or the step itself); `targetIndex` the step that the speech
// is for, after whose start its audio is no longer needed.
struct SpeechGroup {
    int planIndex = 0;
    int targetIndex = 0;
    QList<Utterance> utterances;
};

// The fixed sentences. They are messages of the interface translation, looked up in the language of
// the text they introduce, not in the language of the interface (R-ALR-6).
enum class Sentence {
    GetReadyToPerform,   // contains %1: the name of the exercise
    GetReady,
    Repetitions,         // number form
    Seconds,             // number form
    Minutes,             // number form
    RestFor,             // contains %1: the length of the rest, as in Seconds and Minutes
    Halfway,             // R-ALR-15
    TenSecondsLeft,
    ToGo,                // contains %1: the number of repetitions left
    ThreeToGo
};

class Sentences
{
public:
    virtual ~Sentences() {}
    // The sentence in the language, with the number form of n for the ones that have one. English
    // when the language has no translation.
    virtual QString text(const QString &language, Sentence sentence, int n = 0) const = 0;
};

// The texts of an exercise that are spoken.
struct SpokenExercise {
    QString name;
    QString instructions;
    QString language;
};

class SpeechPlanner
{
public:
    // exercises by key; appLanguage is the language of the interface, used where a text has none.
    SpeechPlanner(const Sentences *sentences, const QHash<QString, SpokenExercise> &exercises, const QString &appLanguage)
        : sentences_(sentences), exercises_(exercises), appLanguage_(appLanguage) {}

    // The language the texts of a step are spoken in: that of its exercise, else the app's.
    QString languageOf(const PlanStep &step) const;
    // "30 seconds", "10 repetitions", "1 minute 30 seconds".
    QString amount(const PlanStep &step, const QString &language) const;
    // The length of a rest in words: seconds below 90, minutes and seconds from 90 (R-ALR-11).
    QString restAmount(int seconds, const QString &language) const;
    // "Now you can rest for N seconds." in the language of the step after the rest; an empty
    // utterance for a rest shorter than 5 s (R-ALR-11).
    Utterance restSentence(const PlanStep *following, const PlanStep &rest) const;
    // "Get ready to perform <exercise>. <amount>." and, when `first`, its description (the tips are
    // not spoken). A step without an exercise gets "Get ready. <amount>." and nothing else.
    QList<Utterance> announcement(const PlanStep &step, bool first) const;
    // The speech of the plan group by group, in the order of need (R-ALR-10): the descriptions of
    // an exercise only at its first appearance. With cues in the settings, the texts of the cues of
    // each step follow the group of its announcement.
    QList<SpeechGroup> groups(const QList<PlanStep> &plan, const SpokenCues::Settings &cues = SpokenCues::Settings::off()) const;
    // The text of a cue (R-ALR-15), in the language of the interface; `count` is the number of
    // repetitions left for SpokenCues::Kind::ToGo.
    Utterance cueUtterance(SpokenCues::Kind kind, int count) const;
    // The name of a phase of a rhythm, in the language of the interface; empty for an empty name.
    Utterance phaseUtterance(const QString &name) const;
    // The texts of the cues a step can have under the settings: the period cues of a step with a
    // set time, the repetitions-left calls and the names of the phases of a tempo step.
    QList<Utterance> cueTexts(const PlanStep &step, const SpokenCues::Settings &cues) const;

    const QString &appLanguage() const { return appLanguage_; }

private:
    const Sentences *sentences_;
    QHash<QString, SpokenExercise> exercises_;
    QString appLanguage_;
};

typedef std::function<bool(const Utterance &)> ReadyCheck;

// Decides at run time what is spoken for the events of a run: the first-time rule of the
// descriptions, the language of each text, and what is skipped silently because no voice reads the
// language or the text failed (R-ALR-8, R-ALR-10).
class SpeechDirector
{
public:
    // `usable` says whether the audio of a text can still be had (not failed).
    SpeechDirector(const SpeechPlanner *planner, const VoiceProvider *voices, const ReadyCheck &usable,
                   const SpokenCues::Settings &cues = SpokenCues::Settings::off())
        : planner_(planner), voices_(voices), usable_(usable), cues_(cues) {}

    // What to say for an Announce event of the engine, in order. A repeated announcement (after a
    // pause) is the same list again, from its start.
    QList<Utterance> onAnnounce(const EngineEvent &event, const QList<PlanStep> &plan);
    // The exercises whose description has been given.
    int described() const { return described_.size(); }

    // What the voice does for a cue of the running step (R-ALR-15). `utterance` is empty when nothing
    // is spoken: the switch is off, a cue is skipped because an announcement or a name is being
    // spoken, or no voice reads the text or the text failed (R-ALR-8). `cutName` tells that the name
    // being spoken is to be stopped first.
    struct CueOrder {
        Utterance utterance;
        bool cutName = false;
    };
    // The period cues (halfway, ten seconds left) and the repetitions-left calls, from a Cue event.
    CueOrder onCue(const EngineEvent &event, const QList<PlanStep> &plan);
    // A phase of a rhythm starts, from a Beat event of the first beat of a phase.
    CueOrder onPhase(const EngineEvent &event, const QList<PlanStep> &plan);
    // Whether an announcement is being spoken, and when the voice has finished a name.
    void setAnnouncing(bool on) { arbiter_.setAnnouncing(on); }
    void nameFinished() { arbiter_.nameEnded(); }
    bool nameSpeaking() const { return arbiter_.nameSpeaking(); }

private:
    QList<Utterance> speakable(const QList<Utterance> &utterances) const;

    const SpeechPlanner *planner_;
    const VoiceProvider *voices_;
    QList<Utterance> speakableCue(const Utterance &u) const;

    ReadyCheck usable_;
    SpokenCues::Settings cues_;
    CueArbiter arbiter_;
    QSet<QString> described_;
    QList<Utterance> last_;
};

#endif

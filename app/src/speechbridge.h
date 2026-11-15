// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef SPEECHBRIDGE_H
#define SPEECHBRIDGE_H

#include <QHash>
#include <QObject>
#include <QScopedPointer>
#include <QElapsedTimer>
#include <QStringList>

#include "speech.h"
#include "speechengine.h"
#include "speechplayer.h"
#include "speechprep.h"
#include "speechsentences.h"
#include "spokencues.h"

class LanguageBridge;
class SpeechRun;

// Spoken announcements (R-ALR-6 to R-ALR-13): which engine and voices exist, whether speech is
// switched on, and the runs that speak. Registered as the context property "speech".
class SpeechBridge : public QObject
{
    Q_OBJECT
    // The switch of the settings; set from QML.
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    // This build has an engine and at least one voice: speech can be switched on.
    Q_PROPERTY(bool available READ available CONSTANT)
    // "ok", "no-engine", "no-voices" or "no-voice-for-app-language".
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    // The language tags that have a voice.
    Q_PROPERTY(QStringList voiceLanguages READ voiceLanguages CONSTANT)
    // The switch "Spoken cues" (R-ALR-15 a, b) and the choice of the spoken phase names (R-ALR-15 c) as
    // the number of SpokenCues::PhaseNames; set from QML. A run takes them when it begins.
    Q_PROPERTY(bool cues READ cues WRITE setCues NOTIFY cuesChanged)
    Q_PROPERTY(int phaseNames READ phaseNames WRITE setPhaseNames NOTIFY phaseNamesChanged)

public:
    // The provider is chosen by the build: the internal engine on the phone, none elsewhere, a fake
    // one in the development build.
    SpeechBridge(LanguageBridge *languages, const QString &appDirectory, QObject *parent = nullptr);
    ~SpeechBridge() override;

    bool enabled() const { return enabled_; }
    void setEnabled(bool on);
    bool available() const { return provider_->hasEngine() && !provider_->voices().isEmpty(); }
    QString status() const;
    QStringList voiceLanguages() const;
    bool cues() const { return cues_; }
    void setCues(bool on);
    int phaseNames() const { return int(phaseNames_); }
    void setPhaseNames(int value);
    SpokenCues::Settings cueSettings() const;

    // A run that speaks for the plan, or nullptr when speech is off or unavailable or the plan has
    // nothing a voice can read. The caller owns it. The texts are prepared in the background from here.
    SpeechRun *beginRun(const QList<PlanStep> &plan, const QHash<QString, SpokenExercise> &exercises, QObject *parent);

    const VoiceProvider *provider() const { return provider_.data(); }
    const Sentences *sentences() const { return &sentences_; }
    QString appLanguage() const;
    bool silent() const { return silent_; }

signals:
    void enabledChanged();
    void statusChanged();
    void cuesChanged();
    void phaseNamesChanged();

private:
    LanguageBridge *languages_;
    QScopedPointer<VoiceProvider> provider_;
    AppSentences sentences_;
    bool enabled_ = true;
    bool silent_ = false;
    bool cues_ = true;
    SpokenCues::PhaseNames phaseNames_ = SpokenCues::defaultPhaseNames;
};

// One run that speaks: it prepares the audio in a window ahead of the speech, decides what is said for
// each event of the engine and plays it (R-ALR-10). It is released, with the engine and the audio,
// when the run ends (R-NFR-2).
class SpeechRun : public QObject
{
    Q_OBJECT

public:
    SpeechRun(SpeechBridge *bridge, const QHash<QString, SpokenExercise> &exercises, QObject *parent);
    ~SpeechRun() override;

    // Starts preparing the speech of the plan, group by group.
    void prepare(const QList<SpeechGroup> &groups);
    // The first announcement can start, or the engine cannot be used: the run can start.
    bool ready() const;
    // Share of the texts of the window that are done, 0 to 1.
    double progress() const;
    bool engineFailed() const { return prep_.engineFailed(); }

    // The engine reported an announcement. True when something is being spoken: voiceFinished() will
    // follow. False when nothing can be said and the step need not wait.
    bool announce(const EngineEvent &event, const QList<PlanStep> &plan);
    // The run reached a step: the window moves and the audio that is no longer needed is freed.
    void stepStarted(int step);
    // A cue of the running step is due (R-ALR-15 a, b), or a phase of a rhythm starts (c): the voice
    // speaks it unless an announcement or a name has the voice, or no audio is ready for it.
    void cue(const EngineEvent &event, const QList<PlanStep> &plan);
    void phaseStarted(const EngineEvent &event, const QList<PlanStep> &plan);
    // The plan changed from a step on (a round was added or removed).
    void planChanged(int fromStep, const QList<PlanStep> &plan);
    // Stops the voice without a finish.
    void stop();

    const SpeechPlanner *planner() const { return &planner_; }
    const SpeechPrep *prep() const { return &prep_; }

signals:
    // The voice has said all it had to, and SpeechTiming::voiceToTimerGapMs has passed.
    void voiceFinished();

private slots:
    void feed();
    void onPlayerFinished();

private:
    void startFeeding(const QList<Utterance> &list);
    void speakCue(const SpeechDirector::CueOrder &order);

    SpeechBridge *bridge_;
    SpeechPlanner planner_;
    SpeechPrep prep_;
    SpeechDirector director_;
    SpeechPlayer player_;
    // Speaks the cues and the phase names, apart from the announcements, whose end the step waits for.
    SpeechPlayer cuePlayer_;
    QTimer feeder_;
    QList<Utterance> list_;
    int index_ = 0;
    int chunk_ = 0;
    bool appended_ = false;
    // When the player has run dry and waits for a text that the thread has not begun.
    QElapsedTimer dryTimer_;
    bool dry_ = false;
    // Bumped at every start and stop of the speech, so that a finish of an old one is not taken for the new.
    int generation_ = 0;
};

#endif

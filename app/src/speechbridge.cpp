// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "speechbridge.h"

#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <QLocale>

#include "languagebridge.h"
#include "devlog.h"
#include "language.h"
#ifdef HAVE_SHERPA
#include "sherpaprovider.h"
#endif
#ifdef FAKE_SPEECH
#include "fakespeech.h"
#endif

namespace {

// The provider of a build that has no engine.
class NoEngineProvider : public VoiceProvider
{
public:
    QList<VoiceInfo> voices() const override { return QList<VoiceInfo>(); }
    bool hasEngine() const override { return false; }
    SpeechEngine *createEngine() const override { return nullptr; }
};

}

SpeechBridge::SpeechBridge(LanguageBridge *languages, const QString &appDirectory, QObject *parent)
    : QObject(parent), languages_(languages), sentences_(languages)
{
#if defined(FAKE_SPEECH)
    FakeVoiceProvider *fake = new FakeVoiceProvider(Language::shipped());
    // The time each text takes can be set to show a slow engine: RT_FAKE_DELAY_MS.
    fake->delayMs = qEnvironmentVariableIsSet("RT_FAKE_DELAY_MS") ? qEnvironmentVariableIntValue("RT_FAKE_DELAY_MS") : 150;
    provider_.reset(fake);
    // The development build shows the flow without any sound.
    silent_ = true;
    Q_UNUSED(appDirectory)
#elif defined(HAVE_SHERPA)
    provider_.reset(new SherpaProvider(QDir(appDirectory).filePath(QStringLiteral("voices")), appDirectory));
#else
    provider_.reset(new NoEngineProvider);
    Q_UNUSED(appDirectory)
#endif
}

SpeechBridge::~SpeechBridge()
{
}

QString SpeechBridge::appLanguage() const
{
    const QString tag = languages_->language();
    return tag.isEmpty() ? QStringLiteral("en") : tag;
}

void SpeechBridge::setEnabled(bool on)
{
    if (enabled_ == on)
        return;
    enabled_ = on;
    emit enabledChanged();
}

QString SpeechBridge::status() const
{
    if (!provider_->hasEngine())
        return QStringLiteral("no-engine");
    if (provider_->voices().isEmpty())
        return QStringLiteral("no-voices");
    if (provider_->voiceFor(appLanguage()).isEmpty())
        return QStringLiteral("no-voice-for-app-language");
    return QStringLiteral("ok");
}

void SpeechBridge::setCues(bool on)
{
    if (cues_ == on)
        return;
    cues_ = on;
    emit cuesChanged();
}

void SpeechBridge::setPhaseNames(int value)
{
    const SpokenCues::PhaseNames names = SpokenCues::phaseNamesFromInt(value);
    if (phaseNames_ == names)
        return;
    phaseNames_ = names;
    emit phaseNamesChanged();
}

SpokenCues::Settings SpeechBridge::cueSettings() const
{
    SpokenCues::Settings settings;
    settings.cues = cues_;
    settings.phaseNames = phaseNames_;
    return settings;
}

QStringList SpeechBridge::voiceLanguages() const
{
    QStringList tags = provider_->languages().toList();
    tags.sort();
    return tags;
}

namespace {

// How long a text that the thread has not begun is waited for once the player has run dry, in milliseconds.
const int skipAfterMs = 300;

}

SpeechRun *SpeechBridge::beginRun(const QList<PlanStep> &plan, const QHash<QString, SpokenExercise> &exercises, QObject *parent)
{
    if (!enabled_ || !available())
        return nullptr;
    SpeechRun *run = new SpeechRun(this, exercises, parent);
    const QList<SpeechGroup> groups = run->planner()->groups(plan, cueSettings());
    bool speakable = false;
    for (const SpeechGroup &g : groups) {
        for (const Utterance &u : g.utterances)
            speakable = speakable || !provider_->voiceFor(u.language).isEmpty();
    }
    if (!speakable) {
        delete run;
        return nullptr;
    }
    run->prepare(groups);
    return run;
}

SpeechRun::SpeechRun(SpeechBridge *bridge, const QHash<QString, SpokenExercise> &exercises, QObject *parent)
    : QObject(parent), bridge_(bridge),
      planner_(bridge->sentences(), exercises, bridge->appLanguage()),
      prep_(bridge->provider()),
      director_(&planner_, bridge->provider(), [this](const Utterance &u) { return !prep_.isFailed(u); }, bridge->cueSettings())
{
    player_.setSilent(bridge->silent());
    connect(&player_, &SpeechPlayer::finished, this, &SpeechRun::onPlayerFinished);
    cuePlayer_.setSilent(bridge->silent());
    connect(&cuePlayer_, &SpeechPlayer::finished, this, [this]() { director_.nameFinished(); });
    feeder_.setInterval(20);
    connect(&feeder_, &QTimer::timeout, this, &SpeechRun::feed);
}

SpeechRun::~SpeechRun()
{
#ifdef FAKE_SPEECH
    devLog(QStringLiteral("run ends: most audio kept ahead %1 bytes, first audio after %2 ms").arg(prep_.peakAheadBytes()).arg(prep_.firstAudioMs()));
#endif
    feeder_.stop();
    player_.stop();
    cuePlayer_.stop();
    prep_.finish();
}

void SpeechRun::prepare(const QList<SpeechGroup> &groups)
{
    prep_.enqueue(groups);
}

bool SpeechRun::ready() const
{
    return prep_.firstAnnouncementReady();
}

double SpeechRun::progress() const
{
    const int total = prep_.total();
    return total > 0 ? double(prep_.finished()) / double(total) : 1.0;
}

bool SpeechRun::announce(const EngineEvent &event, const QList<PlanStep> &plan)
{
    const QList<Utterance> list = director_.onAnnounce(event, plan);
    if (list.isEmpty())
        return false;
    // The texts of an announcement that was not planned (a step was skipped) are queued now.
    if (!event.repeated)
        prep_.ensure(event.stepIndex, event.announceIndex >= 0 ? event.announceIndex : event.stepIndex, list);
    startFeeding(list);
    return true;
}

void SpeechRun::stepStarted(int step)
{
    prep_.stepStarted(step);
}

void SpeechRun::planChanged(int fromStep, const QList<PlanStep> &plan)
{
    prep_.replaceFrom(fromStep, planner_.groups(plan));
}

void SpeechRun::stop()
{
    feeder_.stop();
    player_.stop();
    cuePlayer_.stop();
    director_.setAnnouncing(false);
    director_.nameFinished();
    ++generation_;
}

void SpeechRun::cue(const EngineEvent &event, const QList<PlanStep> &plan)
{
    speakCue(director_.onCue(event, plan));
}

void SpeechRun::phaseStarted(const EngineEvent &event, const QList<PlanStep> &plan)
{
    speakCue(director_.onPhase(event, plan));
}

// The audio of a cue was prepared in advance; one that is not ready is skipped silently (R-ALR-10).
void SpeechRun::speakCue(const SpeechDirector::CueOrder &order)
{
    if (order.cutName)
        cuePlayer_.stop();
    const Utterance &u = order.utterance;
    if (u.isEmpty())
        return;
    const int n = prep_.chunkCount(u);
    if (n == 0 || !prep_.isComplete(u)) {
        director_.nameFinished();
        return;
    }
    cuePlayer_.begin();
    for (int i = 0; i < n; ++i)
        cuePlayer_.append(prep_.chunk(u, i), 0);
    cuePlayer_.end();
}

void SpeechRun::startFeeding(const QList<Utterance> &list)
{
    // An announcement has the voice: a cue that is being spoken is stopped and none starts meanwhile.
    cuePlayer_.stop();
    director_.setAnnouncing(true);
    director_.nameFinished();
    player_.begin();
    ++generation_;
    list_ = list;
    index_ = 0;
    chunk_ = 0;
    appended_ = false;
    dry_ = false;
    feeder_.start();
    feed();
}

// Hands the pieces of the texts to the player as they come, in order. A text that is not there when
// the player has run dry is skipped silently, unless the thread is making it (R-ALR-8, R-ALR-10).
void SpeechRun::feed()
{
    while (index_ < list_.size()) {
        const Utterance &u = list_.at(index_);
        const int n = prep_.chunkCount(u);
        while (chunk_ < n) {
#ifdef FAKE_SPEECH
            devLog(QStringLiteral("piece %1 of [%2] %3").arg(chunk_ + 1).arg(u.language, u.text.left(60)));
#endif
            player_.append(prep_.chunk(u, chunk_), chunk_ == 0 && appended_ ? SpeechTiming::betweenTextsMs : 0);
            appended_ = true;
            ++chunk_;
        }
        if (prep_.isComplete(u)) {
            ++index_;
            chunk_ = 0;
            dry_ = false;
            continue;
        }
        if (n == 0 && !prep_.isInFlight(u) && player_.queuedMs() <= 0) {
            // The thread has not begun the text and nothing is left to play: a short grace for it to
            // pick the text up, then the text is skipped.
            if (!dry_) {
                dry_ = true;
                dryTimer_.start();
            }
            if (dryTimer_.elapsed() > skipAfterMs) {
                ++index_;
                chunk_ = 0;
                dry_ = false;
                continue;
            }
        } else {
            dry_ = false;
        }
        // Waiting for the thread: it hurries while the speech has little left.
        prep_.setUrgent(player_.queuedMs() < 1500);
        return;
    }
    feeder_.stop();
    prep_.setUrgent(false);
    player_.end();
}

void SpeechRun::onPlayerFinished()
{
    const int generation = generation_;
    director_.setAnnouncing(false);
#ifdef FAKE_SPEECH
    devLog(QStringLiteral("voice finished"));
#endif
    // After the handler of the player has returned, and after the gap between the end of the voice and the timer.
    QTimer::singleShot(SpeechTiming::voiceToTimerGapMs, this, [this, generation]() {
        if (generation == generation_)
            emit voiceFinished();
    });
}

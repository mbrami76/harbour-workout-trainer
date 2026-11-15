// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "speechprep.h"
#include "speechtext.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QMutexLocker>
#include <QSet>
#include <memory>

SpeechPrep::SpeechPrep(const VoiceProvider *provider, qint64 maxAheadBytes, QObject *parent)
    : QThread(parent), provider_(provider), maxAhead_(maxAheadBytes)
{
}

SpeechPrep::~SpeechPrep()
{
    finish();
}

QString SpeechPrep::keyOf(const QString &voice, const QString &text)
{
    return voice + QLatin1Char('\n') + text;
}

QString SpeechPrep::keyFor(const Utterance &u) const
{
    return keyOf(provider_->voiceFor(u.language), u.text);
}

void SpeechPrep::append(const SpeechGroup &group)
{
    for (const Utterance &u : group.utterances) {
        const QString voice = provider_->voiceFor(u.language);
        if (voice.isEmpty() || u.isEmpty())
            continue;
        Item item;
        item.planIndex = group.planIndex;
        item.targetIndex = group.targetIndex;
        item.utterance = u;
        item.voice = voice;
        item.key = keyOf(voice, u.text);
        queue_.append(item);
    }
}

void SpeechPrep::enqueue(const QList<SpeechGroup> &groups)
{
    bool work;
    {
        QMutexLocker lock(&mutex_);
        for (const SpeechGroup &g : groups)
            append(g);
        work = !queue_.isEmpty();
    }
    changed_.wakeAll();
    // The engine is made only when there is something to say.
    if (work && !isRunning()) {
        startedAt_ = QDateTime::currentMSecsSinceEpoch();
        start(QThread::LowestPriority);
    }
}

void SpeechPrep::replaceFrom(int fromStep, const QList<SpeechGroup> &groups)
{
    {
        QMutexLocker lock(&mutex_);
        QList<int> drop;
        for (int i = 0; i < queue_.size(); ++i) {
            if (!queue_.at(i).released && queue_.at(i).planIndex >= fromStep)
                drop.append(i);
        }
        release(drop);
        for (const SpeechGroup &g : groups) {
            if (g.planIndex >= fromStep)
                append(g);
        }
    }
    changed_.wakeAll();
    bool work;
    {
        QMutexLocker lock(&mutex_);
        work = !queue_.isEmpty();
    }
    if (work && !isRunning()) {
        startedAt_ = QDateTime::currentMSecsSinceEpoch();
        start(QThread::LowestPriority);
    }
}

void SpeechPrep::ensure(int planIndex, int targetIndex, const QList<Utterance> &texts)
{
    {
        QMutexLocker lock(&mutex_);
        SpeechGroup g;
        g.planIndex = planIndex;
        g.targetIndex = targetIndex;
        for (const Utterance &u : texts) {
            bool present = false;
            const QString key = keyFor(u);
            for (const Item &item : queue_) {
                if (!item.released && item.targetIndex == targetIndex && item.key == key)
                    present = true;
            }
            if (!present)
                g.utterances.append(u);
        }
        append(g);
    }
    changed_.wakeAll();
}

void SpeechPrep::release(const QList<int> &indexes)
{
    QSet<QString> keys;
    for (int i : indexes) {
        if (!queue_.at(i).released) {
            queue_[i].released = true;
            keys.insert(queue_.at(i).key);
        }
    }
    for (const QString &key : keys)
        freeUnused(key);
}

// The audio of a text goes when no unreleased item inside the window needs it; a later use of the
// same text is made again when its time comes. Only the sentence of a rest and the cues come back and stay
// (R-ALR-10).
void SpeechPrep::freeUnused(const QString &key)
{
    for (const Item &item : queue_) {
        if (!item.released && item.key == key && item.planIndex <= startedStep_ + SpeechTiming::lookaheadSteps)
            return;
    }
    auto it = entries_.find(key);
    if (it == entries_.end() || it->rest || key == inFlight_)
        return;
    ahead_ -= it->bytes;
    entries_.erase(it);
}

void SpeechPrep::stepStarted(int step)
{
    {
        QMutexLocker lock(&mutex_);
        startedStep_ = step;
        QList<int> done;
        for (int i = 0; i < queue_.size(); ++i) {
            if (!queue_.at(i).released && queue_.at(i).targetIndex <= step)
                done.append(i);
        }
        release(done);
        // Released items are dropped from the front so that the queue stays short.
        while (!queue_.isEmpty() && queue_.first().released)
            queue_.removeFirst();
    }
    changed_.wakeAll();
}

int SpeechPrep::chunkCount(const Utterance &u) const
{
    QMutexLocker lock(&mutex_);
    return entries_.value(keyFor(u)).chunks.size();
}

Audio SpeechPrep::chunk(const Utterance &u, int index) const
{
    QMutexLocker lock(&mutex_);
    const Entry e = entries_.value(keyFor(u));
    return index >= 0 && index < e.chunks.size() ? e.chunks.at(index) : Audio();
}

bool SpeechPrep::isComplete(const Utterance &u) const
{
    QMutexLocker lock(&mutex_);
    const auto it = entries_.constFind(keyFor(u));
    return it != entries_.constEnd() && (it->complete || it->failed);
}

bool SpeechPrep::isFailed(const Utterance &u) const
{
    QMutexLocker lock(&mutex_);
    const auto it = entries_.constFind(keyFor(u));
    return it != entries_.constEnd() && it->failed && it->chunks.isEmpty();
}

bool SpeechPrep::isInFlight(const Utterance &u) const
{
    QMutexLocker lock(&mutex_);
    return keyFor(u) == inFlight_;
}

bool SpeechPrep::firstAnnouncementReady() const
{
    QMutexLocker lock(&mutex_);
    if (engineFailed_ || queue_.isEmpty())
        return true;
    const auto it = entries_.constFind(queue_.first().key);
    return it != entries_.constEnd() && (!it->chunks.isEmpty() || it->failed);
}

int SpeechPrep::finished() const
{
    QMutexLocker lock(&mutex_);
    int done = 0;
    for (const Item &item : queue_) {
        if (item.planIndex > startedStep_ + SpeechTiming::lookaheadSteps)
            continue;
        const auto it = entries_.constFind(item.key);
        if (it != entries_.constEnd() && (it->complete || it->failed))
            ++done;
    }
    return done;
}

int SpeechPrep::total() const
{
    QMutexLocker lock(&mutex_);
    int n = 0;
    for (const Item &item : queue_) {
        if (item.planIndex <= startedStep_ + SpeechTiming::lookaheadSteps)
            ++n;
    }
    return n;
}

int SpeechPrep::failedCount() const
{
    QMutexLocker lock(&mutex_);
    return failed_;
}

// The first unreleased item inside the window whose text has not been started.
int SpeechPrep::pick() const
{
    for (int i = 0; i < queue_.size(); ++i) {
        const Item &item = queue_.at(i);
        if (item.released || item.planIndex > startedStep_ + SpeechTiming::lookaheadSteps)
            continue;
        if (!entries_.contains(item.key))
            return i;
    }
    return -1;
}

// Over the cap, the thread rests as long as the audio the speech needs next is complete.
bool SpeechPrep::capBlocked() const
{
    if (ahead_ < maxAhead_)
        return false;
    int needed = -1;
    for (const Item &item : queue_) {
        if (!item.released && (needed < 0 || item.targetIndex < needed))
            needed = item.targetIndex;
    }
    for (const Item &item : queue_) {
        if (item.released || item.targetIndex != needed)
            continue;
        const auto it = entries_.constFind(item.key);
        if (it == entries_.constEnd() || !(it->complete || it->failed))
            return false;
    }
    return true;
}

bool SpeechPrep::windowDone() const
{
    return pick() < 0 && inFlight_.isEmpty();
}

bool SpeechPrep::isIdle() const
{
    QMutexLocker lock(&mutex_);
    return windowDone();
}

qint64 SpeechPrep::aheadBytes() const
{
    QMutexLocker lock(&mutex_);
    return ahead_;
}

qint64 SpeechPrep::peakAheadBytes() const
{
    QMutexLocker lock(&mutex_);
    return peak_;
}

bool SpeechPrep::engineFailed() const
{
    QMutexLocker lock(&mutex_);
    return engineFailed_;
}

qint64 SpeechPrep::firstAudioMs() const
{
    QMutexLocker lock(&mutex_);
    return firstAudioAt_ < 0 ? -1 : firstAudioAt_ - startedAt_;
}

void SpeechPrep::setUrgent(bool urgent)
{
    if (isRunning())
        setPriority(urgent ? QThread::NormalPriority : QThread::LowestPriority);
}

bool SpeechPrep::waitForFirstAnnouncement(int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    QMutexLocker lock(&mutex_);
    for (;;) {
        bool ready = engineFailed_ || queue_.isEmpty();
        if (!ready) {
            const auto it = entries_.constFind(queue_.first().key);
            ready = it != entries_.constEnd() && (!it->chunks.isEmpty() || it->failed);
        }
        if (ready)
            return true;
        const qint64 left = timeoutMs - timer.elapsed();
        if (left <= 0)
            return false;
        changed_.wait(&mutex_, ulong(left));
    }
}

bool SpeechPrep::waitUntilIdle(int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    QMutexLocker lock(&mutex_);
    while (!(windowDone() || engineFailed_)) {
        const qint64 left = timeoutMs - timer.elapsed();
        if (left <= 0)
            return false;
        changed_.wait(&mutex_, ulong(left));
    }
    return true;
}

void SpeechPrep::finish()
{
    {
        QMutexLocker lock(&mutex_);
        stop_ = true;
    }
    changed_.wakeAll();
    wait();
    QMutexLocker lock(&mutex_);
    entries_.clear();
    queue_.clear();
    inFlight_.clear();
    ahead_ = 0;
    startedStep_ = -1;
    stop_ = false;
}

void SpeechPrep::run()
{
    std::unique_ptr<SpeechEngine> engine(provider_->createEngine());
    if (!engine) {
        QMutexLocker lock(&mutex_);
        engineFailed_ = true;
        changed_.wakeAll();
        return;
    }
    for (;;) {
        Item item;
        {
            QMutexLocker lock(&mutex_);
            int index = -1;
            while (!stop_) {
                index = pick();
                if (index >= 0 && !capBlocked())
                    break;
                changed_.wait(&mutex_);
            }
            if (stop_)
                break;
            item = queue_.at(index);
            Entry entry;
            entry.rest = item.utterance.kind == UtteranceKind::RestSentence || item.utterance.kind == UtteranceKind::Cue;
            entries_.insert(item.key, entry);
            inFlight_ = item.key;
        }
        // The text is made part by part, cut where the voice pauses (R-ALR-14); the silence of each
        // pause is put into the audio after the part. The piece that ends a part is held back until
        // the next one comes, so that the silence that the engine adds after it can be cut off.
        bool first = true;
        double gain = 1.0;
        bool stopped = false;
        bool ok = true;
        Audio pending;
        auto store = [&](Audio chunk) {
            QMutexLocker lock(&mutex_);
            if (stop_) {
                stopped = true;
                return false;
            }
            auto it = entries_.find(item.key);
            if (it == entries_.end())
                return true;
            if (!chunk.isEmpty()) {
                it->chunks.append(chunk);
                it->bytes += chunk.bytes();
                ahead_ += chunk.bytes();
                peak_ = qMax(peak_, ahead_);
                if (firstAudioAt_ < 0)
                    firstAudioAt_ = QDateTime::currentMSecsSinceEpoch();
            }
            changed_.wakeAll();
            return true;
        };
        auto flush = [&](bool partEnds, int pauseMs) {
            if (pending.isEmpty())
                return true;
            Audio chunk = pending;
            pending = Audio();
            if (partEnds)
                SpeechAudio::trimTrailingSilence(&chunk);
            const int rate = chunk.sampleRate;
            if (!store(chunk))
                return false;
            return pauseMs <= 0 || store(SpeechAudio::silence(rate, pauseMs));
        };
        // A cue is spoken as one piece: no pause constant applies to it (R-ALR-15).
        const QList<SpeechText::Segment> parts = item.utterance.kind == UtteranceKind::Cue
            ? QList<SpeechText::Segment>() << SpeechText::Segment(item.utterance.text.trimmed(), 0)
            : SpeechText::segments(item.utterance.text, SpeechTiming::sentencePauseMs, SpeechTiming::clausePauseMs);
        for (const SpeechText::Segment &part : parts) {
            bool partFirst = true;
            const bool made = engine->synthesise(item.voice, part.text, [&](const Audio &piece) {
                if (!flush(false, 0))
                    return false;
                Audio chunk = piece;
                if (partFirst) {
                    SpeechAudio::trimLeadingSilence(&chunk);
                    if (first)
                        gain = SpeechAudio::gainFor(chunk);
                }
                SpeechAudio::applyGain(&chunk, gain);
                partFirst = false;
                first = false;
                pending = chunk;
                return true;
            });
            if (stopped || !flush(true, part.pauseMs)) {
                ok = false;
                break;
            }
            if (!made) {
                ok = false;
                break;
            }
        }
        if (parts.isEmpty())
            ok = false;
        QMutexLocker lock(&mutex_);
        inFlight_.clear();
        auto it = entries_.find(item.key);
        if (it != entries_.end()) {
            if (ok || !it->chunks.isEmpty()) {
                it->complete = true;
            } else {
                it->failed = true;
                ++failed_;
            }
        }
        // The step it was for may have started while the text was being made.
        freeUnused(item.key);
        changed_.wakeAll();
    }
    engine->release();
}

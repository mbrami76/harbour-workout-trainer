// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef SPEECHPREP_H
#define SPEECHPREP_H

#include <QHash>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include "speech.h"
#include "speechengine.h"

// Prepares the audio of a run in a window (R-ALR-10): one background thread synthesises the texts
// in the order of need and stays ahead of the speech by the next steps of the plan (the window of
// SpeechTiming::lookaheadSteps positions) and by a cap on the audio kept ahead. The audio of a text
// is available piece by piece while it is being made. Audio is kept in memory until the step it
// was for has started and is then freed; only the sentence of a rest and the texts of the cues stay
// until the end of the run.
// The engine is made in the thread and released when the thread ends, so nothing runs outside a
// session (R-NFR-2).
class SpeechPrep : public QThread
{
    Q_OBJECT

public:
    // The audio kept ahead of the speech, apart from what the speech needs next: about a minute and a
    // half at 22.05 kHz, 16 bit mono, which is more than two announcements with their descriptions
    // and pauses.
    static const qint64 defaultMaxAheadBytes = 8LL * 1024 * 1024;

    explicit SpeechPrep(const VoiceProvider *provider, qint64 maxAheadBytes = defaultMaxAheadBytes, QObject *parent = nullptr);
    ~SpeechPrep() override;

    // Queues the groups in order of need and starts the thread if it is not running. A group that
    // has no text with a voice is left out. The first group is the first announcement (see
    // firstAnnouncementReady()).
    void enqueue(const QList<SpeechGroup> &groups);
    // The plan changed from a step on (a round was added or removed): the groups that begin at or
    // after the step and were not spoken are replaced by the given ones.
    void replaceFrom(int fromStep, const QList<SpeechGroup> &groups);
    // Makes sure the texts of an announcement that is due are queued, for the case that the run did
    // not follow the plan (a step was skipped).
    void ensure(int planIndex, int targetIndex, const QList<Utterance> &texts);
    // The run reached the step: the window moves, and the audio of the groups whose step has started
    // is freed.
    void stepStarted(int step);

    // The audio of a text, piece by piece. A text that was not started has no pieces.
    int chunkCount(const Utterance &u) const;
    Audio chunk(const Utterance &u, int index) const;
    // All the pieces are there, or the text failed.
    bool isComplete(const Utterance &u) const;
    // The text failed before any audio came.
    bool isFailed(const Utterance &u) const;
    // The thread is making this text now.
    bool isInFlight(const Utterance &u) const;

    // The first text of the first announcement has audio to start with, or failed, or the engine
    // cannot be used: the run can start.
    bool firstAnnouncementReady() const;
    // Share of the texts of the window that are done, for the progress indication.
    int finished() const;
    int total() const;
    int failedCount() const;
    // Nothing more to do inside the window.
    bool isIdle() const;
    // The audio kept ahead, in bytes, apart from the rest sentences.
    qint64 aheadBytes() const;
    // The most that was kept ahead during the run.
    qint64 peakAheadBytes() const;
    // The engine could not be made or a model failed to load: no text will be spoken.
    bool engineFailed() const;
    // Time from the start of the thread to the first audio, in milliseconds; -1 before it.
    qint64 firstAudioMs() const;
    // Lets the thread run at a higher priority when texts are needed sooner than it delivers them.
    void setUrgent(bool urgent);
    // Waits until the first announcement can start or the time is up.
    bool waitForFirstAnnouncement(int timeoutMs);
    bool waitUntilIdle(int timeoutMs);
    // Stops the thread, releases the engine and forgets the audio.
    void finish();

protected:
    void run() override;

private:
    struct Item {
        int planIndex = 0;
        int targetIndex = 0;
        Utterance utterance;
        QString voice;
        QString key;
        bool released = false;
    };
    struct Entry {
        QList<Audio> chunks;
        bool complete = false;
        bool failed = false;
        bool rest = false;
        double gain = 1.0;
        qint64 bytes = 0;
    };

    static QString keyOf(const QString &voice, const QString &text);
    void append(const SpeechGroup &group);
    // Marks the items released and frees the audio nobody needs any more.
    void release(const QList<int> &indexes);
    void freeUnused(const QString &key);
    int pick() const;
    bool capBlocked() const;
    bool windowDone() const;
    QString keyFor(const Utterance &u) const;

    const VoiceProvider *provider_;
    qint64 maxAhead_;
    mutable QMutex mutex_;
    QWaitCondition changed_;
    QList<Item> queue_;
    QHash<QString, Entry> entries_;
    int startedStep_ = -1;
    bool stop_ = false;
    bool engineFailed_ = false;
    QString inFlight_;
    int failed_ = 0;
    qint64 ahead_ = 0;
    qint64 peak_ = 0;
    qint64 startedAt_ = 0;
    qint64 firstAudioAt_ = -1;
};

#endif

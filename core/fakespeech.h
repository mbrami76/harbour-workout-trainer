// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef FAKESPEECH_H
#define FAKESPEECH_H

#include <QMutex>
#include <QSet>
#include <QSharedPointer>
#include <QStringList>

#include "speechengine.h"

// What the fake engines of a provider were asked, shared with the provider because the engines
// belong to the worker thread and are gone when the run ends.
struct FakeSpeechStats {
    QMutex mutex;
    // The texts synthesised, in order, as "voice|text".
    QStringList synthesised;
    int engines = 0;
    int released = 0;
};

// A speech engine that returns tones whose length depends on the text, one piece for each
// sentence (a sentence ends with ". ", "! " or "? "): for the host tests and for the development
// build that shows the flow without a real engine.
class FakeSpeechEngine : public SpeechEngine
{
public:
    // Milliseconds of tone for each character of the text.
    static const int msPerCharacter = 60;
    static const int sampleRate = 16000;

    FakeSpeechEngine(const QSharedPointer<FakeSpeechStats> &stats, const QSet<QString> &failing, int delayMs = 0)
        : stats_(stats), failing_(failing), delayMs_(delayMs) {}

    bool synthesise(const QString &voiceId, const QString &text, const ChunkSink &sink) override;
    void release() override;

    // The sentences of a text, as the engine cuts it.
    static QStringList sentencesOf(const QString &text);

private:
    QSharedPointer<FakeSpeechStats> stats_;
    QSet<QString> failing_;
    int delayMs_;
    bool released_ = false;
};

class FakeVoiceProvider : public VoiceProvider
{
public:
    explicit FakeVoiceProvider(const QStringList &languages = QStringList() << QStringLiteral("en") << QStringLiteral("it"));

    QList<VoiceInfo> voices() const override { return voices_; }
    bool hasEngine() const override { return hasEngine_; }
    SpeechEngine *createEngine() const override;

    bool hasEngine_ = true;
    // Texts that the engines made by this provider fail to synthesise.
    QSet<QString> failing;
    // Time each sentence takes to synthesise, to show a run that waits for its first announcement.
    int delayMs = 0;
    QSharedPointer<FakeSpeechStats> stats = QSharedPointer<FakeSpeechStats>(new FakeSpeechStats);

private:
    QList<VoiceInfo> voices_;
};

#endif

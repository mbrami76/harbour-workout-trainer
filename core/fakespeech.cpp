// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "fakespeech.h"

#include <QRegularExpression>
#include <QThread>
#include <cmath>

QStringList FakeSpeechEngine::sentencesOf(const QString &text)
{
    QStringList list;
    static const QRegularExpression end(QStringLiteral("(?<=[.!?])\\s+"));
    for (const QString &s : text.split(end, QString::SkipEmptyParts))
        list.append(s);
    return list;
}

bool FakeSpeechEngine::synthesise(const QString &voiceId, const QString &text, const ChunkSink &sink)
{
    if (voiceId.isEmpty() || text.isEmpty() || failing_.contains(text))
        return false;
    {
        QMutexLocker lock(&stats_->mutex);
        stats_->synthesised.append(voiceId + QLatin1Char('|') + text);
    }
    // A tone whose pitch depends on the voice, so that two voices can be told apart.
    const double pitch = 300.0 + 100.0 * double(qAbs(qHash(voiceId)) % 4);
    for (const QString &sentence : sentencesOf(text)) {
        if (delayMs_ > 0)
            QThread::msleep(ulong(delayMs_));
        Audio chunk;
        chunk.sampleRate = sampleRate;
        const int count = sentence.length() * msPerCharacter * sampleRate / 1000;
        chunk.samples.resize(count);
        for (int i = 0; i < count; ++i)
            chunk.samples[i] = qint16(8000.0 * std::sin(6.283185307 * pitch * double(i) / double(sampleRate)));
        if (!sink(chunk))
            return false;
    }
    return true;
}

void FakeSpeechEngine::release()
{
    if (released_)
        return;
    released_ = true;
    QMutexLocker lock(&stats_->mutex);
    ++stats_->released;
}

FakeVoiceProvider::FakeVoiceProvider(const QStringList &languages)
{
    for (const QString &tag : languages)
        voices_.append(VoiceInfo{tag + QStringLiteral("-fake"), tag, QStringLiteral("Fake ") + tag});
}

SpeechEngine *FakeVoiceProvider::createEngine() const
{
    if (!hasEngine_)
        return nullptr;
    {
        QMutexLocker lock(&stats->mutex);
        ++stats->engines;
    }
    return new FakeSpeechEngine(stats, failing, delayMs);
}

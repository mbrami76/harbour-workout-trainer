// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "speechplayer.h"

#include <QAudioFormat>

namespace {

// How often the data is handed to the audio output, in milliseconds.
const int pumpMs = 15;

// How long after the audio should be over the player waits for the output to say that it is idle, in milliseconds.
const int idleWaitMs = 200;

// An output that takes no data for this long cannot be used: the speech ends, so that the run does not wait for it.
const int stallMs = 2000;

}

SpeechPlayer::SpeechPlayer(QObject *parent) : QObject(parent)
{
    timer_.setInterval(pumpMs);
    connect(&timer_, &QTimer::timeout, this, &SpeechPlayer::pump);
}

SpeechPlayer::~SpeechPlayer()
{
    closeOutput();
}

void SpeechPlayer::closeOutput()
{
    timer_.stop();
    if (output_)
        output_->stop();
    output_.reset();
    device_ = nullptr;
    pending_.clear();
    endAtMs_ = 0;
    rate_ = 0;
}

void SpeechPlayer::begin()
{
    clock_.start();
    closeOutput();
    clips_.clear();
    open_ = true;
    ended_ = false;
}

void SpeechPlayer::append(const Audio &clip, int silenceBeforeMs)
{
    if (!open_ || clip.isEmpty())
        return;
    Clip c;
    c.sampleRate = clip.sampleRate;
    c.bytes = QByteArray(clip.sampleRate * silenceBeforeMs / 1000 * int(sizeof(qint16)), '\0');
    c.bytes.append(reinterpret_cast<const char *>(clip.samples.constData()), int(clip.bytes()));
    clips_.append(c);
    if (!timer_.isActive())
        timer_.start();
}

void SpeechPlayer::end()
{
    if (!open_)
        return;
    ended_ = true;
    if (!timer_.isActive())
        timer_.start();
}

void SpeechPlayer::stop()
{
    open_ = false;
    ended_ = false;
    clips_.clear();
    closeOutput();
}

qint64 SpeechPlayer::queuedMs() const
{
    qint64 bytes = pending_.size();
    int rate = rate_;
    qint64 ms = 0;
    if (rate > 0)
        ms += bytes * 1000 / (qint64(rate) * int(sizeof(qint16)));
    for (const Clip &c : clips_)
        ms += qint64(c.bytes.size()) * 1000 / (qint64(c.sampleRate) * int(sizeof(qint16)));
    return ms;
}

void SpeechPlayer::startOutput(int sampleRate)
{
    closeOutput();
    QAudioFormat format;
    format.setSampleRate(sampleRate);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec(QStringLiteral("audio/pcm"));
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    output_.reset(new QAudioOutput(format));
    output_->setVolume(silent_ ? 0.0 : 1.0);
    device_ = output_->start();
    rate_ = sampleRate;
    progressAtMs_ = clock_.elapsed();
    timer_.start();
}

void SpeechPlayer::finishNow()
{
    open_ = false;
    ended_ = false;
    closeOutput();
    emit finished();
}

// Hands the clips to the output as it has room, and ends the stream once the last one is played.
void SpeechPlayer::pump()
{
    if (!open_) {
        timer_.stop();
        return;
    }
    if (pending_.isEmpty() && !clips_.isEmpty()) {
        // A clip of another rate waits until the output has played what it has.
        if (output_ && clips_.first().sampleRate != rate_) {
            if (output_->state() != QAudio::IdleState && output_->state() != QAudio::StoppedState)
                return;
            closeOutput();
        }
        if (!output_)
            startOutput(clips_.first().sampleRate);
        pending_ = clips_.takeFirst().bytes;
    }
    if (device_ && !pending_.isEmpty()) {
        const int room = output_->bytesFree();
        if (room > 0) {
            const qint64 written = device_->write(pending_.constData(), qMin<qint64>(room, pending_.size()));
            if (written > 0) {
                progressAtMs_ = clock_.elapsed();
                pending_.remove(0, int(written));
                // What is written plays at the speed of the sound, after what was written before it.
                const qint64 now = clock_.elapsed();
                endAtMs_ = qMax(endAtMs_, now) + written * 1000 / (qint64(rate_) * int(sizeof(qint16)));
            }
        }
        if ((output_->error() != QAudio::NoError && output_->error() != QAudio::UnderrunError)
            || (!pending_.isEmpty() && clock_.elapsed() - progressAtMs_ > stallMs)) {
            // An output that cannot be used ends the speech at once, so that the run does not wait for it.
            finishNow();
            return;
        }
    }
    if (ended_ && pending_.isEmpty() && clips_.isEmpty()) {
        // The last sample has been played: the time the audio takes is over and the output has no more data,
        // or, when it does not say so, a short time more.
        const bool idle = !output_ || output_->state() == QAudio::IdleState || output_->state() == QAudio::StoppedState;
        const qint64 now = clock_.elapsed();
        if (now >= endAtMs_ && (idle || now >= endAtMs_ + idleWaitMs))
            finishNow();
    }
}

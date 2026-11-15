// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef SPEECHAUDIO_H
#define SPEECHAUDIO_H

#include <QVector>
#include <QtGlobal>

// Synthesised speech kept in memory: 16 bit mono samples (R-ALR-10).
struct Audio {
    QVector<qint16> samples;
    int sampleRate = 0;

    bool isEmpty() const { return samples.isEmpty() || sampleRate <= 0; }
    qint64 durationMs() const { return sampleRate > 0 ? qint64(samples.size()) * 1000 / sampleRate : 0; }
    qint64 bytes() const { return qint64(samples.size()) * qint64(sizeof(qint16)); }
};

namespace SpeechAudio {

// Level every text is brought to, as a share of full scale, and the silence kept before the speech.
const int targetPeak = 20000;
const int silenceThreshold = 300;
const int keptLeadMs = 15;

// Removes the silence before the first sample above the threshold, except a short lead.
void trimLeadingSilence(Audio *audio);
// Removes the silence after the last sample above the threshold, so that the pauses of the speech
// are the ones that are put in (R-ALR-14) and no silence ends an announcement.
void trimTrailingSilence(Audio *audio);
// Silence of that many milliseconds at the sample rate.
Audio silence(int sampleRate, int ms);
// Scales the samples so that the loudest one reaches targetPeak; a quieter text is raised and a
// louder one lowered. Silence stays silence.
void normalise(Audio *audio);
// trimLeadingSilence() then normalise(): the form in which audio is kept.
void prepare(Audio *audio);

// The gain that brings the loudest sample of the audio to targetPeak; 1 for silence. The pieces of
// one text are scaled with the gain of the first, so that the level does not jump inside a text.
double gainFor(const Audio &audio);
void applyGain(Audio *audio, double gain);

// Memory a text of that many characters is expected to need at the sample rate, before it is
// synthesised: speech of about 15 characters a second at 16 bit mono.
qint64 estimateBytes(int characters, int sampleRate);

}

#endif

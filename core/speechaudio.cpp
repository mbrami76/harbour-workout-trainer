// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "speechaudio.h"

#include <cstdlib>

void SpeechAudio::trimLeadingSilence(Audio *audio)
{
    const QVector<qint16> &s = audio->samples;
    int first = 0;
    while (first < s.size() && std::abs(int(s.at(first))) < silenceThreshold)
        ++first;
    if (first >= s.size()) {
        audio->samples.clear();
        return;
    }
    const int keep = audio->sampleRate * keptLeadMs / 1000;
    const int from = qMax(0, first - keep);
    if (from > 0)
        audio->samples = s.mid(from);
}

void SpeechAudio::trimTrailingSilence(Audio *audio)
{
    int last = audio->samples.size();
    while (last > 0 && std::abs(int(audio->samples.at(last - 1))) < silenceThreshold)
        --last;
    if (last == 0) {
        audio->samples.clear();
        return;
    }
    // A short tail is kept, so that the last sound is not cut.
    const int keep = audio->sampleRate * keptLeadMs / 1000;
    audio->samples.resize(qMin(int(audio->samples.size()), last + keep));
}

Audio SpeechAudio::silence(int sampleRate, int ms)
{
    Audio a;
    a.sampleRate = sampleRate;
    a.samples = QVector<qint16>(int(qint64(sampleRate) * ms / 1000), 0);
    return a;
}

void SpeechAudio::normalise(Audio *audio)
{
    int peak = 0;
    for (qint16 v : audio->samples)
        peak = qMax(peak, std::abs(int(v)));
    if (peak == 0)
        return;
    const double gain = double(targetPeak) / double(peak);
    for (qint16 &v : audio->samples)
        v = qint16(qBound(-32767.0, double(v) * gain, 32767.0));
}

double SpeechAudio::gainFor(const Audio &audio)
{
    int peak = 0;
    for (qint16 v : audio.samples)
        peak = qMax(peak, std::abs(int(v)));
    return peak == 0 ? 1.0 : double(targetPeak) / double(peak);
}

void SpeechAudio::applyGain(Audio *audio, double gain)
{
    for (qint16 &v : audio->samples)
        v = qint16(qBound(-32767.0, double(v) * gain, 32767.0));
}

void SpeechAudio::prepare(Audio *audio)
{
    trimLeadingSilence(audio);
    normalise(audio);
}

qint64 SpeechAudio::estimateBytes(int characters, int sampleRate)
{
    return qint64(characters) * sampleRate * qint64(sizeof(qint16)) / 15;
}

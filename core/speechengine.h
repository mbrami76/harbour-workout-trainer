// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef SPEECHENGINE_H
#define SPEECHENGINE_H

#include <QList>
#include <QSet>
#include <QString>
#include <functional>

#include "speechaudio.h"

// A voice: the folder name it is known by, the language tag of the texts it reads (R-EXR-12) and
// a name to show.
struct VoiceInfo {
    QString id;
    QString language;
    QString name;
};

// Receives the audio of a text as the engine makes it, one piece at a time (a sentence for the
// internal engine). Returns false to stop the synthesis of that text.
typedef std::function<bool(const Audio &chunk)> ChunkSink;

// Turns text into audio with one voice at a time (R-ALR-7). An engine belongs to the thread that
// made it; it keeps the models it loaded until release().
class SpeechEngine
{
public:
    virtual ~SpeechEngine() {}
    // Synthesises the text with the voice and hands the audio to the sink in pieces, in order, as
    // each is made, so that the speech can begin before the whole text is done. True when the whole
    // text was made; false when the voice or the text cannot be used or the sink stopped it.
    virtual bool synthesise(const QString &voiceId, const QString &text, const ChunkSink &sink) = 0;
    // Frees the models and the memory of the engine.
    virtual void release() = 0;
};

// Which voices exist and how to get an engine that uses them. The internal provider has the voices
// of the package; providers for engines installed on the device can be added without a change of
// the callers (R-ALR-7).
class VoiceProvider
{
public:
    // Why no speech is offered.
    enum class Reason { None, NoEngine, NoVoices };

    virtual ~VoiceProvider() {}
    virtual QList<VoiceInfo> voices() const = 0;
    // True when this build has an engine at all.
    virtual bool hasEngine() const = 0;
    // A new engine for the calling thread, or nullptr when none can be made.
    virtual SpeechEngine *createEngine() const = 0;

    // The voice for the language tag of a text, or an empty id: an exact tag first, then the first
    // part of the tag ("en-gb" is read by an "en" voice). No voice means the text is not spoken
    // (R-ALR-8).
    QString voiceFor(const QString &language) const;
    // The languages that have a voice.
    QSet<QString> languages() const;
    Reason reason() const;
};

#endif

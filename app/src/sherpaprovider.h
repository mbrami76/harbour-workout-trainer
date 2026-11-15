// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef SHERPAPROVIDER_H
#define SHERPAPROVIDER_H

#include "speechengine.h"

// The internal engine (R-ALR-7): sherpa-onnx runs Piper voices; eSpeak NG, linked statically, turns
// the text into phonemes. The voices are the folders under the voices directory; each has a
// voice.conf with the language tag, the eSpeak voice name and the files of the model. Built only for
// the phone; elsewhere the provider of the app reports that there is no engine.
class SherpaProvider : public VoiceProvider
{
public:
    // voicesDir holds one folder per voice; dataDir is the folder that contains espeak-ng-data.
    SherpaProvider(const QString &voicesDir, const QString &dataDir);

    QList<VoiceInfo> voices() const override { return voices_; }
    bool hasEngine() const override { return true; }
    SpeechEngine *createEngine() const override;

    // What a voice folder says about itself.
    struct Description {
        QString language;
        QString espeakVoice;
        QString model;
        QString tokens;
        int speaker = 0;
    };
    static bool readDescription(const QString &folder, Description *out);

private:
    QString dataDir_;
    QString voicesDir_;
    QList<VoiceInfo> voices_;
};

#endif

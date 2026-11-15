// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef SPEECHPLAYER_H
#define SPEECHPLAYER_H

#include <QAudioOutput>
#include <QByteArray>
#include <QElapsedTimer>
#include <QIODevice>
#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QTimer>

#include "speechaudio.h"

// Plays speech from memory, clip after clip, while more clips are still being added (R-ALR-6): the
// first sentence of a text is played while the next ones are made. Nothing is written to a file.
// The audio goes to the system's audio output like the other sounds, at the system volume.
class SpeechPlayer : public QObject
{
    Q_OBJECT

public:
    explicit SpeechPlayer(QObject *parent = nullptr);
    ~SpeechPlayer() override;

    // Opens a stream: what was playing is stopped.
    void begin();
    // Adds a clip to the stream, after `silenceBeforeMs` of silence.
    void append(const Audio &clip, int silenceBeforeMs = 0);
    // No more clips come: finished() follows when the last one has been played.
    void end();
    // Stops at once without emitting finished().
    void stop();
    bool isPlaying() const { return open_; }
    // Milliseconds of audio added and not played yet.
    qint64 queuedMs() const;
    // Plays at no volume: the timing is the same. For the development build with a fake engine.
    void setSilent(bool silent) { silent_ = silent; }

signals:
    void finished();

private slots:
    void pump();

private:
    struct Clip {
        QByteArray bytes;
        int sampleRate = 0;
    };
    void startOutput(int sampleRate);
    void closeOutput();
    void finishNow();

    QList<Clip> clips_;
    bool open_ = false;
    bool ended_ = false;
    bool silent_ = false;
    int rate_ = 0;
    QScopedPointer<QAudioOutput> output_;
    QIODevice *device_ = nullptr;
    QByteArray pending_;
    // The moment, on clock_, at which what was written so far has been played if the output takes it at
    // the speed of the sound: the end of the audio without asking the output (which may not say).
    qint64 endAtMs_ = 0;
    // The last time the output took data.
    qint64 progressAtMs_ = 0;
    QElapsedTimer clock_;
    QTimer timer_;
};

#endif

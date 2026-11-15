// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "sherpaprovider.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QMutex>
#include <QTextStream>
#include <cstring>
#include <vector>

#include "sherpa-onnx/c-api/c-api.h"
#include <espeak-ng/speak_lib.h>

namespace {

// Two threads of computation at most (R-ALR-10).
const int computeThreads = 2;

// How the voices sound. The three scales of the VITS model are set when a voice is loaded, so a change
// needs a new load. They are tuned by listening; the pauses are not made here (R-ALR-14).
//
// Randomness of the sound: lower is flatter and more regular, higher is livelier. The Piper default
// is 0.667.
const float voiceNoiseScale = 0.15f;
// Randomness of the length of the sounds: lower gives a steadier rhythm. The Piper defaults are
// 0.8 and, for the English voice, 0.333.
const float voiceNoiseScaleW = 0.2f;
// Speed of the speech: above 1 is slower, below 1 is faster.
const float voiceLengthScale = 1.0f;
// Scale of the silence that the library adds after a sentence. For these voices it made no steady
// difference, and what ends a piece is cut off before the pauses of the speech are put in.
const float voiceSilenceScale = 0.2f;

// eSpeak NG keeps its state in the process, so one engine at a time may use it.
QMutex espeakMutex;

// Flags of the terminator that eSpeak NG reports with a clause.
const int clauseIntonation = 0x00007000;
const int clauseTypeSentence = 0x00080000;
const int clauseIntComma = 0x00001000;
const int clauseIntQuestion = 0x00002000;
const int clauseIntExclamation = 0x00003000;
const int clauseTypeClause = 0x00040000;

struct Phonemes {
    std::vector<int32_t> codepoints;
    std::vector<int32_t> sentences;
};

void addText(Phonemes *p, const char *utf8)
{
    const QString text = QString::fromUtf8(utf8);
    const QVector<uint> points = text.toUcs4();
    for (uint c : points)
        p->codepoints.push_back(int32_t(c));
}

// The way of Piper: the IPA of each clause, then the punctuation of the clause; a sentence ends at a
// clause that ends a sentence.
void phonemise(const QByteArray &text, Phonemes *out)
{
    const void *cursor = text.constData();
    size_t sentenceStart = 0;
    while (cursor != nullptr) {
        int terminator = 0;
        const char *ipa = espeak_TextToPhonemesWithTerminator(&cursor, espeakCHARS_UTF8, 0x02, &terminator);
        if (ipa)
            addText(out, ipa);
        const int intonation = terminator & clauseIntonation;
        const char *punctuation = nullptr;
        if (intonation == clauseIntQuestion)
            punctuation = "?";
        else if (intonation == clauseIntExclamation)
            punctuation = "!";
        else if (intonation == clauseIntComma)
            punctuation = ",";
        else if ((terminator & clauseTypeClause) && intonation == 0)
            punctuation = ":";
        else if (terminator & clauseTypeSentence)
            punctuation = ".";
        if (punctuation && ipa && *ipa)
            addText(out, punctuation);
        if ((terminator & clauseTypeSentence) || cursor == nullptr) {
            const size_t length = out->codepoints.size() - sentenceStart;
            if (length > 0) {
                out->sentences.push_back(int32_t(length));
                sentenceStart = out->codepoints.size();
            }
        }
    }
}

class SherpaEngine : public SpeechEngine
{
public:
    SherpaEngine(const QString &voicesDir, const QString &dataDir) : voicesDir_(voicesDir)
    {
        QMutexLocker lock(&espeakMutex);
        ready_ = espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0, dataDir.toUtf8().constData(), espeakINITIALIZE_DONT_EXIT) >= 0;
    }
    ~SherpaEngine() override { release(); }

    bool synthesise(const QString &voiceId, const QString &text, const ChunkSink &sink) override
    {
        if (!ready_)
            return false;
        Voice *voice = load(voiceId);
        if (!voice)
            return false;
        Phonemes phonemes;
        {
            QMutexLocker lock(&espeakMutex);
            if (espeak_SetVoiceByName(voice->espeakVoice.constData()) != EE_OK)
                return false;
            phonemise(text.toUtf8(), &phonemes);
        }
        SherpaOnnxGenerationConfig generation;
        std::memset(&generation, 0, sizeof generation);
        generation.sid = voice->speaker;
        generation.speed = 1.0f;
        generation.silence_scale = voiceSilenceScale;
        if (!phonemes.codepoints.empty()) {
            generation.phoneme_codepoints = phonemes.codepoints.data();
            generation.phoneme_codepoints_lens = phonemes.sentences.data();
            generation.phoneme_codepoints_num_sentences = int32_t(phonemes.sentences.size());
        }
        const QByteArray utf8 = text.toUtf8();
        // The library calls back after each sentence with the audio of it, so that the speech can begin
        // before the whole text is made.
        struct Context {
            const ChunkSink *sink;
            int rate;
            int pieces;
            bool stopped;
        } context;
        context.sink = &sink;
        context.rate = voice->sampleRate;
        context.pieces = 0;
        context.stopped = false;
        auto callback = [](const float *samples, int32_t n, float, void *arg) -> int32_t {
            Context *c = static_cast<Context *>(arg);
            Audio piece;
            piece.sampleRate = c->rate;
            piece.samples.resize(n);
            for (int i = 0; i < n; ++i)
                piece.samples[i] = qint16(qBound(-1.0f, samples[i], 1.0f) * 32767.0f);
            ++c->pieces;
            if (!(*c->sink)(piece)) {
                c->stopped = true;
                return 0;
            }
            return 1;
        };
        const SherpaOnnxGeneratedAudio *audio = SherpaOnnxOfflineTtsGenerateWithConfig(voice->tts, utf8.constData(), &generation, callback, &context);
        // A library that made the audio without calling back hands it over in one piece.
        bool handedOver = true;
        if (context.pieces == 0 && audio && audio->n > 0) {
            Audio piece;
            piece.sampleRate = audio->sample_rate;
            piece.samples.resize(audio->n);
            for (int i = 0; i < audio->n; ++i)
                piece.samples[i] = qint16(qBound(-1.0f, audio->samples[i], 1.0f) * 32767.0f);
            handedOver = sink(piece);
            ++context.pieces;
        }
        if (audio)
            SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
        if (!handedOver)
            return false;
        return !context.stopped && context.pieces > 0;
    }

    void release() override
    {
        for (Voice &v : voices_) {
            if (v.tts)
                SherpaOnnxDestroyOfflineTts(v.tts);
        }
        voices_.clear();
        if (ready_) {
            QMutexLocker lock(&espeakMutex);
            espeak_Terminate();
            ready_ = false;
        }
    }

private:
    struct Voice {
        const SherpaOnnxOfflineTts *tts = nullptr;
        int sampleRate = 0;
        QByteArray espeakVoice;
        int speaker = 0;
    };

    Voice *load(const QString &id)
    {
        auto it = voices_.find(id);
        if (it != voices_.end())
            return it->tts ? &it.value() : nullptr;
        Voice voice;
        SherpaProvider::Description d;
        const QString folder = QDir(voicesDir_).filePath(id);
        if (SherpaProvider::readDescription(folder, &d)) {
            const QByteArray model = QDir(folder).filePath(d.model).toUtf8();
            const QByteArray tokens = QDir(folder).filePath(d.tokens).toUtf8();
            SherpaOnnxOfflineTtsConfig config;
            std::memset(&config, 0, sizeof config);
            config.model.num_threads = computeThreads;
            config.model.provider = "cpu";
            config.model.vits.model = model.constData();
            config.model.vits.tokens = tokens.constData();
            config.model.vits.noise_scale = voiceNoiseScale;
            config.model.vits.noise_scale_w = voiceNoiseScaleW;
            config.model.vits.length_scale = voiceLengthScale;
            voice.tts = SherpaOnnxCreateOfflineTts(&config);
            voice.sampleRate = voice.tts ? SherpaOnnxOfflineTtsSampleRate(voice.tts) : 0;
            voice.espeakVoice = d.espeakVoice.toUtf8();
            voice.speaker = d.speaker;
        }
        voices_.insert(id, voice);
        return voice.tts ? &voices_[id] : nullptr;
    }

    QString voicesDir_;
    bool ready_ = false;
    QHash<QString, Voice> voices_;
};

}

bool SherpaProvider::readDescription(const QString &folder, Description *out)
{
    QFile file(QDir(folder).filePath(QStringLiteral("voice.conf")));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    QTextStream in(&file);
    in.setCodec("UTF-8");
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        const int eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0)
            continue;
        const QString key = line.left(eq).trimmed();
        const QString value = line.mid(eq + 1).trimmed();
        if (key == QLatin1String("language"))
            out->language = value;
        else if (key == QLatin1String("espeak-voice"))
            out->espeakVoice = value;
        else if (key == QLatin1String("model"))
            out->model = value;
        else if (key == QLatin1String("tokens"))
            out->tokens = value;
        else if (key == QLatin1String("speaker"))
            out->speaker = value.toInt();
    }
    return !out->language.isEmpty() && !out->espeakVoice.isEmpty() && !out->model.isEmpty() && !out->tokens.isEmpty()
        && QFile::exists(QDir(folder).filePath(out->model)) && QFile::exists(QDir(folder).filePath(out->tokens));
}

SherpaProvider::SherpaProvider(const QString &voicesDir, const QString &dataDir)
    : dataDir_(dataDir), voicesDir_(voicesDir)
{
    if (!QDir(dataDir).exists(QStringLiteral("espeak-ng-data")))
        return;
    const QStringList folders = QDir(voicesDir).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &id : folders) {
        Description d;
        if (readDescription(QDir(voicesDir).filePath(id), &d))
            voices_.append(VoiceInfo{id, d.language, id});
    }
}

SpeechEngine *SherpaProvider::createEngine() const
{
    return new SherpaEngine(voicesDir_, dataDir_);
}

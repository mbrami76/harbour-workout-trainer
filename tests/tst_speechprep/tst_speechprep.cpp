// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "fakespeech.h"
#include "speechprep.h"

// Preparing the audio of a run in a window (R-ALR-10): order, the first piece of the first text, the
// window of steps, freeing of played audio, the cap on the audio ahead, failures and the release of
// the engine.
namespace {

Utterance say(const QString &text, const QString &language = QStringLiteral("en"),
              UtteranceKind kind = UtteranceKind::Announcement)
{
    Utterance u;
    u.text = text;
    u.language = language;
    u.kind = kind;
    return u;
}

// One announcement: it begins at plan position `at` and is for the step `target`.
SpeechGroup group(int at, int target, const QList<Utterance> &texts)
{
    SpeechGroup g;
    g.planIndex = at;
    g.targetIndex = target;
    g.utterances = texts;
    return g;
}

QStringList synthesised(const FakeVoiceProvider &provider)
{
    QMutexLocker lock(&provider.stats->mutex);
    return provider.stats->synthesised;
}

}

class TstSpeechPrep : public QObject
{
    Q_OBJECT

private slots:
    void audioIsNormalisedAndTrimmed();
    void piecesOfOneTextKeepOneLevel();
    void textsAreSynthesisedInTheOrderOfNeed();
    void identicalTextsInTheWindowAreSynthesisedOnce();
    void runCanStartWithTheFirstPieceOfTheFirstText();
    void theWindowLimitsTheWorkToTheNextSteps();
    void audioIsFreedWhenItsStepHasStarted();
    void theRestSentenceStaysUntilTheEnd();
    void aCueStaysUntilTheEndAndHasNoPauses();
    void aTextThatComesBackIsMadeAgain();
    void capOnTheAudioAheadHoldsTheThreadBack();
    void aFailedTextIsSkippedAndTheRestGoesOn();
    void textsWithoutAVoiceAreNotQueued();
    void engineAndAudioAreReleasedAtTheEnd();
    void noEngineIsReported();
    void nothingIsSynthesisedWithoutTexts();
    void aChangeOfThePlanReplacesTheTextsAhead();
    void aTextNeededLateIsQueuedByEnsure();
    void timeToTheFirstAudioIsReported();
    void pausesAreSilenceInsideATextAndNoneAtTheEnd();
    void aTextIsMadeInPartsAtThePunctuation();
    void pausesCountForTheCapAndFreeWithTheText();
};

void TstSpeechPrep::audioIsNormalisedAndTrimmed()
{
    Audio a;
    a.sampleRate = 16000;
    a.samples.fill(0, 8000);
    for (int i = 0; i < 1000; ++i)
        a.samples.append(qint16((i % 2) ? 3000 : -3000));
    SpeechAudio::prepare(&a);
    // Half a second of silence became a short lead.
    QCOMPARE(a.samples.size(), 1000 + 16000 * SpeechAudio::keptLeadMs / 1000);
    int peak = 0;
    for (qint16 v : a.samples)
        peak = qMax(peak, qAbs(int(v)));
    QCOMPARE(peak, SpeechAudio::targetPeak);
    // A louder text comes down to the same level.
    Audio loud;
    loud.sampleRate = 16000;
    loud.samples << 30000 << -30000;
    SpeechAudio::normalise(&loud);
    QCOMPARE(int(loud.samples.first()), SpeechAudio::targetPeak);
    // Silence stays silence, and an empty result is empty.
    Audio quiet;
    quiet.sampleRate = 16000;
    quiet.samples.fill(10, 100);
    SpeechAudio::trimLeadingSilence(&quiet);
    QVERIFY(quiet.samples.isEmpty());
    QCOMPARE(SpeechAudio::gainFor(quiet), 1.0);
}

void TstSpeechPrep::piecesOfOneTextKeepOneLevel()
{
    FakeVoiceProvider provider;
    SpeechPrep prep(&provider);
    const Utterance u = say("One sentence here. Another sentence follows. A third one ends it.");
    prep.enqueue(QList<SpeechGroup>() << group(0, 0, QList<Utterance>() << u));
    QVERIFY(prep.waitUntilIdle(5000));
    // Three sentences and the two pauses between them.
    QCOMPARE(prep.chunkCount(u), 5);
    // All pieces are scaled with the gain of the first one: the same tone, the same peak.
    int peaks[3];
    for (int k = 0; k < 3; ++k) {
        peaks[k] = 0;
        for (qint16 v : prep.chunk(u, k * 2).samples)
            peaks[k] = qMax(peaks[k], qAbs(int(v)));
        QCOMPARE(prep.chunk(u, k * 2).sampleRate, int(FakeSpeechEngine::sampleRate));
    }
    QVERIFY(qAbs(peaks[0] - peaks[1]) < 50 && qAbs(peaks[1] - peaks[2]) < 50);
    prep.finish();
}

void TstSpeechPrep::textsAreSynthesisedInTheOrderOfNeed()
{
    FakeVoiceProvider provider;
    SpeechPrep prep(&provider);
    prep.enqueue(QList<SpeechGroup>() << group(0, 0, QList<Utterance>() << say("one") << say("two", "it"))
                                      << group(1, 2, QList<Utterance>() << say("three")));
    QVERIFY(prep.waitUntilIdle(5000));
    QCOMPARE(synthesised(provider), QStringList() << "en-fake|one" << "it-fake|two" << "en-fake|three");
    QVERIFY(prep.isComplete(say("two", "it")));
    QCOMPARE(prep.chunk(say("one"), 0).sampleRate, int(FakeSpeechEngine::sampleRate));
    prep.finish();
}

void TstSpeechPrep::identicalTextsInTheWindowAreSynthesisedOnce()
{
    FakeVoiceProvider provider;
    SpeechPrep prep(&provider);
    prep.enqueue(QList<SpeechGroup>() << group(0, 0, QList<Utterance>() << say("same") << say("same") << say("same", "it"))
                                      << group(1, 1, QList<Utterance>() << say("same")));
    QVERIFY(prep.waitUntilIdle(5000));
    QCOMPARE(synthesised(provider).size(), 2);
    prep.finish();
}

void TstSpeechPrep::runCanStartWithTheFirstPieceOfTheFirstText()
{
    FakeVoiceProvider provider;
    provider.delayMs = 80;
    SpeechPrep prep(&provider);
    const Utterance first = say("First sentence. Second sentence. Third sentence.");
    prep.enqueue(QList<SpeechGroup>() << group(0, 0, QList<Utterance>() << first << say("Next text.")));
    QVERIFY(prep.waitForFirstAnnouncement(5000));
    // One sentence is there; the text is not done and the next one has not begun.
    QVERIFY(prep.chunkCount(first) >= 1);
    QVERIFY(prep.chunkCount(first) < 5);
    QVERIFY(!prep.isComplete(first));
    QVERIFY(prep.waitUntilIdle(5000));
    // Three sentences and the two pauses between them.
    QCOMPARE(prep.chunkCount(first), 5);
    QVERIFY(prep.isComplete(first));
    prep.finish();
}

void TstSpeechPrep::theWindowLimitsTheWorkToTheNextSteps()
{
    FakeVoiceProvider provider;
    SpeechPrep prep(&provider);
    QList<SpeechGroup> groups;
    for (int i = 0; i < 8; ++i)
        groups << group(i, i, QList<Utterance>() << say(QStringLiteral("text %1").arg(i)));
    prep.enqueue(groups);
    QVERIFY(prep.waitUntilIdle(5000));
    // The announcement at position 0 and the two after it: the next steps are covered.
    QCOMPARE(synthesised(provider).size(), SpeechTiming::lookaheadSteps);
    QVERIFY(prep.isComplete(say("text 2")));
    QVERIFY(!prep.isComplete(say("text 3")));
    // The run reaches a step: the window moves with it.
    prep.stepStarted(1);
    QVERIFY(prep.waitUntilIdle(5000));
    QCOMPARE(synthesised(provider).size(), SpeechTiming::lookaheadSteps + 2);
    QVERIFY(prep.isComplete(say("text 4")));
    QVERIFY(!prep.isComplete(say("text 5")));
    prep.finish();
}

void TstSpeechPrep::audioIsFreedWhenItsStepHasStarted()
{
    FakeVoiceProvider provider;
    SpeechPrep prep(&provider);
    prep.enqueue(QList<SpeechGroup>() << group(0, 1, QList<Utterance>() << say("for step one") << say("more", "en", UtteranceKind::Description))
                                      << group(2, 3, QList<Utterance>() << say("for step three")));
    QVERIFY(prep.waitUntilIdle(5000));
    QVERIFY(prep.aheadBytes() > 0);
    prep.stepStarted(0);
    QVERIFY(prep.chunkCount(say("for step one")) > 0);
    // Step 1 has started: its announcement is over.
    prep.stepStarted(1);
    QCOMPARE(prep.chunkCount(say("for step one")), 0);
    QCOMPARE(prep.chunkCount(say("more", "en", UtteranceKind::Description)), 0);
    QVERIFY(prep.chunkCount(say("for step three")) > 0);
    prep.stepStarted(3);
    QCOMPARE(prep.aheadBytes(), qint64(0));
    prep.finish();
}

void TstSpeechPrep::aCueStaysUntilTheEndAndHasNoPauses()
{
    FakeVoiceProvider provider;
    SpeechPrep prep(&provider);
    const Utterance cue = say("Hold. Now", "en", UtteranceKind::Cue);
    const Utterance plain = say("Stand. Go");
    prep.enqueue(QList<SpeechGroup>() << group(0, 0, QList<Utterance>() << cue << plain));
    QVERIFY(prep.waitUntilIdle(5000));
    prep.stepStarted(0);
    prep.stepStarted(1);
    // The text of a cue is kept after its step has started; the other is freed.
    QVERIFY(prep.chunkCount(cue) > 0);
    QCOMPARE(prep.chunkCount(plain), 0);
    // Cut at the full stop a text has a pause of silence; a cue has none.
    FakeVoiceProvider other;
    SpeechPrep prepOther(&other);
    prepOther.enqueue(QList<SpeechGroup>() << group(0, 5, QList<Utterance>() << cue << plain));
    QVERIFY(prepOther.waitUntilIdle(5000));
    QVERIFY(prepOther.chunkCount(cue) < prepOther.chunkCount(plain));
    prep.finish();
    prepOther.finish();
}

void TstSpeechPrep::theRestSentenceStaysUntilTheEnd()
{
    FakeVoiceProvider provider;
    SpeechPrep prep(&provider);
    const Utterance rest = say("Now you can rest for 10 seconds.", "en", UtteranceKind::RestSentence);
    prep.enqueue(QList<SpeechGroup>() << group(0, 1, QList<Utterance>() << rest << say("announcement")));
    QVERIFY(prep.waitUntilIdle(5000));
    prep.stepStarted(1);
    QVERIFY(prep.chunkCount(rest) > 0);
    QCOMPARE(prep.chunkCount(say("announcement")), 0);
    // It is not made again when it comes back.
    prep.enqueue(QList<SpeechGroup>() << group(2, 3, QList<Utterance>() << rest));
    prep.stepStarted(2);
    QVERIFY(prep.waitUntilIdle(5000));
    int times = 0;
    for (const QString &s : synthesised(provider))
        times += s.endsWith(rest.text) ? 1 : 0;
    QCOMPARE(times, 1);
    prep.finish();
    QCOMPARE(prep.chunkCount(rest), 0);
}

void TstSpeechPrep::aTextThatComesBackIsMadeAgain()
{
    FakeVoiceProvider provider;
    SpeechPrep prep(&provider);
    const Utterance again = say("Get ready to perform Squat.");
    QList<SpeechGroup> groups;
    groups << group(0, 0, QList<Utterance>() << again);
    for (int i = 1; i < 5; ++i)
        groups << group(i, i, QList<Utterance>() << say(QStringLiteral("other %1").arg(i)));
    groups << group(5, 5, QList<Utterance>() << again);
    prep.enqueue(groups);
    QVERIFY(prep.waitUntilIdle(5000));
    prep.stepStarted(0);
    prep.stepStarted(1);
    QCOMPARE(prep.chunkCount(again), 0);
    prep.stepStarted(3);
    QVERIFY(prep.waitUntilIdle(5000));
    QVERIFY(prep.chunkCount(again) > 0);
    int times = 0;
    for (const QString &s : synthesised(provider))
        times += s.endsWith(again.text) ? 1 : 0;
    QCOMPARE(times, 2);
    prep.finish();
}

void TstSpeechPrep::capOnTheAudioAheadHoldsTheThreadBack()
{
    FakeVoiceProvider provider;
    // 12 characters are about 23 KB of the fake audio: the cap allows two of them.
    SpeechPrep prep(&provider, 30 * 1000);
    prep.enqueue(QList<SpeechGroup>() << group(0, 0, QList<Utterance>() << say("Hello there."))
                                      << group(1, 1, QList<Utterance>() << say("Second one."))
                                      << group(2, 2, QList<Utterance>() << say("Third text.")));
    QVERIFY(prep.waitForFirstAnnouncement(5000));
    // The first announcement is complete and the cap is reached by the second: the third waits.
    QTest::qWait(300);
    QCOMPARE(synthesised(provider).size(), 2);
    QVERIFY(prep.aheadBytes() >= 30 * 1000 || prep.aheadBytes() > 20 * 1000);
    // The step that was spoken has started: its audio is freed and the thread goes on.
    prep.stepStarted(0);
    QVERIFY(prep.waitUntilIdle(5000));
    QCOMPARE(synthesised(provider).size(), 3);
    QVERIFY(prep.peakAheadBytes() <= 30 * 1000 + 30 * 1000);
    prep.finish();
}

void TstSpeechPrep::aFailedTextIsSkippedAndTheRestGoesOn()
{
    FakeVoiceProvider provider;
    provider.failing.insert("bad");
    SpeechPrep prep(&provider);
    prep.enqueue(QList<SpeechGroup>() << group(0, 0, QList<Utterance>() << say("good one") << say("bad") << say("good two")));
    QVERIFY(prep.waitUntilIdle(5000));
    QVERIFY(prep.isComplete(say("good one")));
    QVERIFY(prep.isFailed(say("bad")));
    QVERIFY(prep.isComplete(say("bad")));
    QVERIFY(prep.isComplete(say("good two")));
    QCOMPARE(prep.chunkCount(say("bad")), 0);
    QCOMPARE(prep.failedCount(), 1);
    prep.finish();
}

void TstSpeechPrep::textsWithoutAVoiceAreNotQueued()
{
    FakeVoiceProvider provider(QStringList() << "en");
    SpeechPrep prep(&provider);
    prep.enqueue(QList<SpeechGroup>() << group(0, 0, QList<Utterance>() << say("hello") << say("ciao", "it") << say("")));
    QVERIFY(prep.waitUntilIdle(5000));
    QCOMPARE(prep.total(), 1);
    QCOMPARE(prep.chunkCount(say("ciao", "it")), 0);
    prep.finish();
}

void TstSpeechPrep::engineAndAudioAreReleasedAtTheEnd()
{
    FakeVoiceProvider provider;
    SpeechPrep prep(&provider);
    prep.enqueue(QList<SpeechGroup>() << group(0, 0, QList<Utterance>() << say("one") << say("two")));
    QVERIFY(prep.waitUntilIdle(5000));
    prep.finish();
    QCOMPARE(provider.stats->engines, 1);
    QCOMPARE(provider.stats->released, 1);
    QCOMPARE(prep.chunkCount(say("one")), 0);
    QCOMPARE(prep.aheadBytes(), qint64(0));
    QVERIFY(!prep.isRunning());
}

void TstSpeechPrep::noEngineIsReported()
{
    FakeVoiceProvider provider;
    provider.hasEngine_ = false;
    SpeechPrep prep(&provider);
    prep.enqueue(QList<SpeechGroup>() << group(0, 0, QList<Utterance>() << say("one")));
    QVERIFY(prep.waitForFirstAnnouncement(5000));
    QVERIFY(prep.engineFailed());
    QCOMPARE(prep.chunkCount(say("one")), 0);
    prep.finish();
}

void TstSpeechPrep::nothingIsSynthesisedWithoutTexts()
{
    FakeVoiceProvider provider;
    SpeechPrep prep(&provider);
    prep.enqueue(QList<SpeechGroup>());
    prep.enqueue(QList<SpeechGroup>() << group(0, 0, QList<Utterance>() << say("ciao", "de")));
    QVERIFY(!prep.isRunning());
    prep.finish();
    QCOMPARE(provider.stats->engines, 0);
    QCOMPARE(synthesised(provider).size(), 0);
}

void TstSpeechPrep::aChangeOfThePlanReplacesTheTextsAhead()
{
    FakeVoiceProvider provider;
    SpeechPrep prep(&provider);
    prep.enqueue(QList<SpeechGroup>() << group(0, 0, QList<Utterance>() << say("first"))
                                      << group(1, 1, QList<Utterance>() << say("old second"))
                                      << group(4, 4, QList<Utterance>() << say("old third")));
    QVERIFY(prep.waitUntilIdle(5000));
    QVERIFY(prep.isComplete(say("old second")));
    // A round was added: from step 1 on the plan is another one.
    prep.replaceFrom(1, QList<SpeechGroup>() << group(1, 1, QList<Utterance>() << say("new second")) << group(2, 2, QList<Utterance>() << say("new third")));
    QVERIFY(prep.waitUntilIdle(5000));
    QCOMPARE(prep.chunkCount(say("old second")), 0);
    QVERIFY(prep.isComplete(say("new second")));
    QVERIFY(prep.isComplete(say("new third")));
    QVERIFY(!synthesised(provider).contains("en-fake|old third"));
    prep.finish();
}

void TstSpeechPrep::aTextNeededLateIsQueuedByEnsure()
{
    FakeVoiceProvider provider;
    SpeechPrep prep(&provider);
    prep.enqueue(QList<SpeechGroup>() << group(0, 0, QList<Utterance>() << say("planned")));
    QVERIFY(prep.waitUntilIdle(5000));
    prep.ensure(0, 0, QList<Utterance>() << say("planned") << say("not planned"));
    QVERIFY(prep.waitUntilIdle(5000));
    QVERIFY(prep.isComplete(say("not planned")));
    QCOMPARE(synthesised(provider).size(), 2);
    prep.finish();
}

void TstSpeechPrep::timeToTheFirstAudioIsReported()
{
    FakeVoiceProvider provider;
    provider.delayMs = 50;
    SpeechPrep prep(&provider);
    QCOMPARE(prep.firstAudioMs(), qint64(-1));
    prep.enqueue(QList<SpeechGroup>() << group(0, 0, QList<Utterance>() << say("Some text. And more text.")));
    QVERIFY(prep.waitForFirstAnnouncement(5000));
    QVERIFY(prep.firstAudioMs() >= 40);
    QVERIFY(prep.firstAudioMs() < 2000);
    prep.finish();
}

// The silence chunks of a text have only zeros.
static bool isSilence(const Audio &a)
{
    for (qint16 v : a.samples) {
        if (v != 0)
            return false;
    }
    return !a.samples.isEmpty();
}

void TstSpeechPrep::pausesAreSilenceInsideATextAndNoneAtTheEnd()
{
    FakeVoiceProvider provider;
    SpeechPrep prep(&provider);
    const Utterance u = say("Stand tall, chest up. Go.");
    prep.enqueue(QList<SpeechGroup>() << group(0, 0, QList<Utterance>() << u));
    QVERIFY(prep.waitUntilIdle(5000));
    // Sound, 300 ms, sound, 600 ms, sound; the text ends with sound, not with silence.
    QCOMPARE(prep.chunkCount(u), 5);
    QVERIFY(!isSilence(prep.chunk(u, 0)));
    QVERIFY(isSilence(prep.chunk(u, 1)));
    QCOMPARE(prep.chunk(u, 1).durationMs(), qint64(SpeechTiming::clausePauseMs));
    QVERIFY(!isSilence(prep.chunk(u, 2)));
    QVERIFY(isSilence(prep.chunk(u, 3)));
    QCOMPARE(prep.chunk(u, 3).durationMs(), qint64(SpeechTiming::sentencePauseMs));
    QVERIFY(!isSilence(prep.chunk(u, 4)));
    // The last sample is part of the sound: nothing silent was added or left at the end.
    const Audio last = prep.chunk(u, 4);
    int tailQuiet = 0;
    for (int i = last.samples.size() - 1; i >= 0 && qAbs(int(last.samples.at(i))) < SpeechAudio::silenceThreshold; --i)
        ++tailQuiet;
    QVERIFY(tailQuiet <= last.sampleRate * SpeechAudio::keptLeadMs / 1000 + 1);
    // A text of one part has no pause at all, whatever it ends with.
    const Utterance one = say("Just this.");
    prep.ensure(0, 0, QList<Utterance>() << one);
    QVERIFY(prep.waitUntilIdle(5000));
    QCOMPARE(prep.chunkCount(one), 1);
    prep.finish();
}

void TstSpeechPrep::aTextIsMadeInPartsAtThePunctuation()
{
    FakeVoiceProvider provider;
    SpeechPrep prep(&provider);
    const Utterance u = say("Hold 1.5 seconds, e.g. slowly. Then go.");
    prep.enqueue(QList<SpeechGroup>() << group(0, 0, QList<Utterance>() << u));
    QVERIFY(prep.waitUntilIdle(5000));
    // The numbers and the abbreviation are not cut.
    QCOMPARE(synthesised(provider), QStringList() << "en-fake|Hold 1.5 seconds," << "en-fake|e.g. slowly." << "en-fake|Then go.");
    prep.finish();
}

void TstSpeechPrep::pausesCountForTheCapAndFreeWithTheText()
{
    FakeVoiceProvider provider;
    SpeechPrep prep(&provider, 1024 * 1024);
    const Utterance u = say("One, two, three, four. Five.");
    prep.enqueue(QList<SpeechGroup>() << group(0, 0, QList<Utterance>() << u));
    QVERIFY(prep.waitUntilIdle(5000));
    qint64 bytes = 0;
    for (int i = 0; i < prep.chunkCount(u); ++i)
        bytes += prep.chunk(u, i).bytes();
    // The silence is part of what is kept ahead.
    QCOMPARE(prep.aheadBytes(), bytes);
    QVERIFY(bytes > 3 * SpeechTiming::clausePauseMs * FakeSpeechEngine::sampleRate * 2 / 1000);
    // It goes with the text when the step has started.
    prep.stepStarted(0);
    QCOMPARE(prep.aheadBytes(), qint64(0));
    prep.finish();
}

QTEST_GUILESS_MAIN(TstSpeechPrep)

#include "tst_speechprep.moc"

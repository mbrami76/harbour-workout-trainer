// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "engine.h"
#include "fakeclock.h"
#include "fakespeech.h"
#include "speech.h"
#include "speechtext.h"
#include "testsupport.h"

// Spoken announcements (R-ALR-6, R-ALR-8, R-ALR-10, R-ALR-11, R-ALR-13): the texts and their order, the
// first-time rule, and the wait of a step for its announcement on the time line of the engine.
namespace {

// English sentences; any other language gets them with its tag in front, which shows that the
// sentence was taken for the language of the text.
class TestSentences : public Sentences
{
public:
    QString text(const QString &language, Sentence sentence, int n) const override
    {
        QString s;
        switch (sentence) {
        case Sentence::GetReadyToPerform: s = QStringLiteral("Get ready to perform %1"); break;
        case Sentence::GetReady: s = QStringLiteral("Get ready"); break;
        case Sentence::Repetitions: s = n == 1 ? QStringLiteral("1 repetition") : QStringLiteral("%1 repetitions").arg(n); break;
        case Sentence::Seconds: s = n == 1 ? QStringLiteral("1 second") : QStringLiteral("%1 seconds").arg(n); break;
        case Sentence::Minutes: s = n == 1 ? QStringLiteral("1 minute") : QStringLiteral("%1 minutes").arg(n); break;
        case Sentence::RestFor: s = QStringLiteral("Now you can rest for %1"); break;
        case Sentence::Halfway: s = QStringLiteral("Halfway"); break;
        case Sentence::TenSecondsLeft: s = QStringLiteral("Ten seconds left"); break;
        case Sentence::ToGo: s = QStringLiteral("%1 to go"); break;
        case Sentence::ThreeToGo: s = QStringLiteral("Three to go"); break;
        }
        return language == QLatin1String("en") ? s : QStringLiteral("[%1] ").arg(language) + s;
    }
};

QHash<QString, SpokenExercise> exercises()
{
    QHash<QString, SpokenExercise> map;
    map.insert("squat", SpokenExercise{"Squat", "Stand tall.", "en"});
    map.insert("plank", SpokenExercise{"Plank", "Hold still.", "xx"});
    map.insert("walk", SpokenExercise{"Walk", "", "en"});
    map.insert("tune", SpokenExercise{"Tune", "Move.", "en"});
    map.insert("long", SpokenExercise{"Long", "A long description that goes on for a while. It has two sentences.", "en"});
    return map;
}

QList<PlanStep> planOf(const QList<Step> &steps, int rounds = 1)
{
    return expandPlan(workout(QList<Block>() << block(steps, rounds)), tempoSnapshot());
}

QStringList texts(const QList<Utterance> &list)
{
    QStringList out;
    for (const Utterance &u : list)
        out << u.text;
    return out;
}

}

class TstSpeech : public QObject
{
    Q_OBJECT

private slots:
    void announcementHasNameAndAmount();
    void descriptionOnlyWhenFirstAndNoTips();
    void exerciseWithoutDescriptionSpeaksWhatItHas();
    void amountForRepetitionsMinutesAndSeconds();
    void textsAreInTheLanguageOfTheExercise();
    void restSentenceIsInTheLanguageOfTheNextStep();
    void restSentenceHasItsLengthOnlyFromFiveSeconds();
    void aShortRestStillHasTheAnnouncementOfTheNextStep();
    void stepWithoutExerciseSpeaksGetReady();
    void groupsFollowTheOrderOfNeed();
    void nothingIsSpokenForPhasesOfARhythm();
    void directorSpeaksDescriptionOnce();
    void directorSkipsTextsWithoutVoiceOrAudio();
    void directorRepeatsWithoutTheRestSentence();
    void voiceChoiceByTag();

    void restIsNotShortenedAndTheStepWaits();
    void voiceFinishedInTheRestMeansNoWait();
    void firstStepWaitsAfterTheLeadIn();
    void pauseStopsAndResumeRepeats();
    void pauseWhileTheStepWaits();
    void pausesLengthenTheVoiceAndTheStepStillWaitsForTheLastWord();
    void noVoiceMeansNoWait();
    void noEventsWithoutAnnouncements();
    void skippingAnnouncesTheStep();
    void timingValuesAreThoseOfTheRequirements();

private:
    TestSentences sentences_;
};

void TstSpeech::announcementHasNameAndAmount()
{
    const SpeechPlanner planner(&sentences_, exercises(), "en");
    const QList<PlanStep> plan = planOf(QList<Step>() << work("squat", 30));
    const QList<Utterance> list = planner.announcement(plan.first(), false);
    QCOMPARE(list.size(), 1);
    QCOMPARE(list.first().text, QStringLiteral("Get ready to perform Squat. 30 seconds."));
    QCOMPARE(int(list.first().kind), int(UtteranceKind::Announcement));
    QCOMPARE(list.first().language, QStringLiteral("en"));
}

void TstSpeech::descriptionOnlyWhenFirstAndNoTips()
{
    const SpeechPlanner planner(&sentences_, exercises(), "en");
    const QList<PlanStep> plan = planOf(QList<Step>() << repStep("squat", 10));
    const QList<Utterance> first = planner.announcement(plan.first(), true);
    QCOMPARE(texts(first), QStringList() << "Get ready to perform Squat. 10 repetitions." << "Stand tall.");
    QCOMPARE(int(first.at(1).kind), int(UtteranceKind::Description));
    QCOMPARE(planner.announcement(plan.first(), false).size(), 1);
}

void TstSpeech::exerciseWithoutDescriptionSpeaksWhatItHas()
{
    const SpeechPlanner planner(&sentences_, exercises(), "en");
    const QList<PlanStep> plan = planOf(QList<Step>() << work("plank", 20) << work("walk", 20));
    // With a description; and without.
    QCOMPARE(planner.announcement(plan.at(0), true).size(), 2);
    QCOMPARE(planner.announcement(plan.at(1), true).size(), 1);
}

void TstSpeech::amountForRepetitionsMinutesAndSeconds()
{
    const SpeechPlanner planner(&sentences_, exercises(), "en");
    const QList<PlanStep> plan = planOf(QList<Step>() << work("squat", 1) << work("squat", 60) << work("squat", 90) << work("squat", 120) << repStep("squat", 1));
    QCOMPARE(planner.amount(plan.at(0), "en"), QStringLiteral("1 second"));
    QCOMPARE(planner.amount(plan.at(1), "en"), QStringLiteral("1 minute"));
    QCOMPARE(planner.amount(plan.at(2), "en"), QStringLiteral("1 minute 30 seconds"));
    QCOMPARE(planner.amount(plan.at(3), "en"), QStringLiteral("2 minutes"));
    QCOMPARE(planner.amount(plan.at(4), "en"), QStringLiteral("1 repetition"));
}

void TstSpeech::textsAreInTheLanguageOfTheExercise()
{
    const SpeechPlanner planner(&sentences_, exercises(), "en");
    const QList<PlanStep> plan = planOf(QList<Step>() << work("plank", 20));
    const QList<Utterance> list = planner.announcement(plan.first(), true);
    QCOMPARE(list.first().text, QStringLiteral("[xx] Get ready to perform Plank. [xx] 20 seconds."));
    for (const Utterance &u : list)
        QCOMPARE(u.language, QStringLiteral("xx"));
}

void TstSpeech::restSentenceIsInTheLanguageOfTheNextStep()
{
    const SpeechPlanner planner(&sentences_, exercises(), "en");
    const QList<PlanStep> plan = planOf(QList<Step>() << rest(10) << work("plank", 20));
    const Utterance u = planner.restSentence(&plan.at(1), plan.at(0));
    QCOMPARE(u.text, QStringLiteral("[xx] Now you can rest for [xx] 10 seconds."));
    QCOMPARE(u.language, QStringLiteral("xx"));
    QCOMPARE(int(u.kind), int(UtteranceKind::RestSentence));
    // Without a step after it the language of the app is used.
    QCOMPARE(planner.restSentence(nullptr, plan.at(0)).language, QStringLiteral("en"));
}

void TstSpeech::restSentenceHasItsLengthOnlyFromFiveSeconds()
{
    const SpeechPlanner planner(&sentences_, exercises(), "en");
    const QList<PlanStep> plan = planOf(QList<Step>() << rest(4) << rest(5) << rest(60) << rest(89) << rest(90) << rest(120) << rest(150) << work("squat", 5));
    // A rest of 4 s has no sentence; from 5 s the sentence names the length.
    QVERIFY(planner.restSentence(nullptr, plan.at(0)).isEmpty());
    QCOMPARE(planner.restSentence(nullptr, plan.at(1)).text, QStringLiteral("Now you can rest for 5 seconds."));
    QCOMPARE(planner.restSentence(nullptr, plan.at(2)).text, QStringLiteral("Now you can rest for 60 seconds."));
    QCOMPARE(planner.restSentence(nullptr, plan.at(3)).text, QStringLiteral("Now you can rest for 89 seconds."));
    // From 90 s on: minutes and seconds.
    QCOMPARE(planner.restSentence(nullptr, plan.at(4)).text, QStringLiteral("Now you can rest for 1 minute 30 seconds."));
    QCOMPARE(planner.restSentence(nullptr, plan.at(5)).text, QStringLiteral("Now you can rest for 2 minutes."));
    QCOMPARE(planner.restSentence(nullptr, plan.at(6)).text, QStringLiteral("Now you can rest for 2 minutes 30 seconds."));
    QCOMPARE(planner.restAmount(75, "en"), QStringLiteral("75 seconds"));
}

void TstSpeech::aShortRestStillHasTheAnnouncementOfTheNextStep()
{
    const SpeechPlanner planner(&sentences_, exercises(), "en");
    const QList<PlanStep> plan = planOf(QList<Step>() << work("walk", 5) << rest(3) << work("squat", 5));
    const QList<SpeechGroup> groups = planner.groups(plan);
    QCOMPARE(groups.size(), 2);
    // The group of the rest begins with the announcement: no sentence for 3 s.
    QCOMPARE(groups.at(1).planIndex, 1);
    QCOMPARE(groups.at(1).targetIndex, 2);
    QCOMPARE(int(groups.at(1).utterances.first().kind), int(UtteranceKind::Announcement));
}

void TstSpeech::stepWithoutExerciseSpeaksGetReady()
{
    const SpeechPlanner planner(&sentences_, exercises(), "en");
    const QList<PlanStep> plan = planOf(QList<Step>() << work(QString(), 45));
    const QList<Utterance> list = planner.announcement(plan.first(), true);
    QCOMPARE(texts(list), QStringList() << "Get ready. 45 seconds.");
}

void TstSpeech::groupsFollowTheOrderOfNeed()
{
    const SpeechPlanner planner(&sentences_, exercises(), "en");
    const QList<PlanStep> plan = planOf(QList<Step>() << work("squat", 30) << rest(10) << work("plank", 20) << rest(10) << work("squat", 30));
    const QList<SpeechGroup> groups = planner.groups(plan);
    QCOMPARE(groups.size(), 3);
    // The first step: its announcement with its description.
    QCOMPARE(groups.at(0).planIndex, 0);
    QCOMPARE(groups.at(0).targetIndex, 0);
    QCOMPARE(texts(groups.at(0).utterances), QStringList()
        << "Get ready to perform Squat. 30 seconds." << "Stand tall.");
    // The rest, then the next step, in the language of that step.
    QCOMPARE(groups.at(1).planIndex, 1);
    QCOMPARE(groups.at(1).targetIndex, 2);
    QCOMPARE(texts(groups.at(1).utterances), QStringList()
        << "[xx] Now you can rest for [xx] 10 seconds."
        << "[xx] Get ready to perform Plank. [xx] 20 seconds." << "Hold still.");
    // The exercise that comes back is announced again without its description.
    QCOMPARE(groups.at(2).planIndex, 3);
    QCOMPARE(groups.at(2).targetIndex, 4);
    QCOMPARE(texts(groups.at(2).utterances), QStringList()
        << "Now you can rest for 10 seconds." << "Get ready to perform Squat. 30 seconds.");
}

void TstSpeech::nothingIsSpokenForPhasesOfARhythm()
{
    tempoSnapshot().clear();
    const SpeechPlanner planner(&sentences_, exercises(), "en");
    const QList<PlanStep> plan = planOf(QList<Step>() << tempoStep("tune", Tempo::phases(QList<int>() << 4 << 4 << 2, 3, QStringList() << "Down" << "" << "Up")));
    QVERIFY(plan.first().hasTempo);
    // The announcement and nothing else: the names of the phases are not spoken (R-ALR-13).
    const QList<SpeechGroup> groups = planner.groups(plan);
    QCOMPARE(groups.size(), 1);
    for (const Utterance &u : groups.first().utterances) {
        QVERIFY(!u.text.contains("Down"));
        QVERIFY(!u.text.contains("Up"));
    }
    tempoSnapshot().clear();
}

namespace {

struct Direct {
    TestSentences sentences;
    SpeechPlanner planner{&sentences, exercises(), "en"};
    FakeVoiceProvider voices;
    bool usable = true;
    SpeechDirector director{&planner, &voices, [this](const Utterance &) { return usable; }};
};

// The event of the engine: `index` is the step announced, `restIndex` the rest that begins (the event
// is at that step), -1 for none.
EngineEvent announce(int index, bool restSentence = false, bool repeated = false, int restIndex = -1)
{
    EngineEvent e;
    e.type = EventType::Announce;
    e.stepIndex = restIndex;
    e.announceIndex = index;
    e.restSentence = restSentence;
    e.repeated = repeated;
    return e;
}

}

void TstSpeech::directorSpeaksDescriptionOnce()
{
    Direct d;
    const QList<PlanStep> plan = planOf(QList<Step>() << work("squat", 30) << work("squat", 30));
    QCOMPARE(d.director.onAnnounce(announce(0), plan).size(), 2);
    QCOMPARE(d.director.onAnnounce(announce(1), plan).size(), 1);
    QCOMPARE(d.director.described(), 1);
}

void TstSpeech::directorSkipsTextsWithoutVoiceOrAudio()
{
    Direct d;
    const QList<PlanStep> plan = planOf(QList<Step>() << work("plank", 30));
    d.voices = FakeVoiceProvider(QStringList() << "en");
    QVERIFY(d.director.onAnnounce(announce(0), plan).isEmpty());
    Direct e;
    e.usable = false;
    const QList<PlanStep> plan2 = planOf(QList<Step>() << work("squat", 30));
    QVERIFY(e.director.onAnnounce(announce(0), plan2).isEmpty());
}

void TstSpeech::directorRepeatsWithoutTheRestSentence()
{
    Direct d;
    const QList<PlanStep> plan = planOf(QList<Step>() << rest(10) << work("squat", 30));
    const QList<Utterance> first = d.director.onAnnounce(announce(1, true, false, 0), plan);
    QCOMPARE(first.size(), 3);
    QCOMPARE(int(first.first().kind), int(UtteranceKind::RestSentence));
    const QList<Utterance> again = d.director.onAnnounce(announce(1, false, true, 0), plan);
    QCOMPARE(again.size(), 2);
    QCOMPARE(int(again.first().kind), int(UtteranceKind::Announcement));
    // The description is part of the announcement that is repeated.
    QCOMPARE(int(again.at(1).kind), int(UtteranceKind::Description));
}

void TstSpeech::voiceChoiceByTag()
{
    FakeVoiceProvider provider(QStringList() << "en" << "it");
    QCOMPARE(provider.voiceFor("it"), QStringLiteral("it-fake"));
    QCOMPARE(provider.voiceFor("IT"), QStringLiteral("it-fake"));
    QCOMPARE(provider.voiceFor("en-gb"), QStringLiteral("en-fake"));
    QVERIFY(provider.voiceFor("de").isEmpty());
    QVERIFY(provider.voiceFor("").isEmpty());
    QVERIFY(int(provider.reason()) == int(VoiceProvider::Reason::None));
    provider.hasEngine_ = false;
    QVERIFY(int(provider.reason()) == int(VoiceProvider::Reason::NoEngine));
    FakeVoiceProvider none{QStringList()};
    QVERIFY(int(none.reason()) == int(VoiceProvider::Reason::NoVoices));
}

namespace {

// A run on a fake clock with a speaker that takes as long as the tones of the texts: what the
// bridge does with the events of the engine.
class Run
{
public:
    Run(const QList<PlanStep> &plan, int leadInSec, bool announcements = true, const QString &app = "en")
        : plan_(plan), planner_(&sentences_, exercises(), app),
          director_(&planner_, &voices_, [](const Utterance &) { return true; })
    {
        EngineConfig config;
        config.leadInSec = leadInSec;
        config.announcements = announcements;
        engine_.reset(new SessionEngine(&clock_, plan, config));
    }

    void start() { apply(engine_->start()); }
    void at(qint64 ms)
    {
        // The speaker finishes when its time comes, before the engine is asked about that moment.
        if (voiceEnd_ >= 0 && ms >= voiceEnd_) {
            clock_.set(voiceEnd_);
            voiceEnd_ = -1;
            apply(engine_->voiceFinished());
        }
        clock_.set(ms);
        apply(engine_->update());
    }
    void pause()
    {
        voiceEnd_ = -1;
        ++stopped_;
        apply(engine_->pause());
    }
    void resume() { apply(engine_->resume()); }
    // Skips to the next step: the voice of the step that is left stops.
    void skip()
    {
        voiceEnd_ = -1;
        ++stopped_;
        apply(engine_->next());
    }
    qint64 timeOf(EventType type, int step) const
    {
        for (const EngineEvent &e : log_) {
            if (e.type == type && e.stepIndex == step)
                return e.atMs;
        }
        return -1;
    }
    int count(EventType type) const
    {
        int n = 0;
        for (const EngineEvent &e : log_)
            n += e.type == type ? 1 : 0;
        return n;
    }

    FakeClock clock_;
    QScopedPointer<SessionEngine> engine_;
    FakeVoiceProvider voices_;
    TestSentences sentences_;
    QList<PlanStep> plan_;
    SpeechPlanner planner_;
    SpeechDirector director_;
    QList<EngineEvent> log_;
    QList<QStringList> spoken_;
    qint64 voiceEnd_ = -1;
    int stopped_ = 0;

private:
    void apply(const QList<EngineEvent> &events)
    {
        log_ += events;
        for (const EngineEvent &e : events) {
            if (e.type != EventType::Announce)
                continue;
            // While the session is paused nothing is spoken; the resume asks again.
            if (engine_->state() == SessionState::Paused)
                continue;
            const QList<Utterance> list = director_.onAnnounce(e, engine_->plan());
            spoken_.append(texts(list));
            qint64 ms = 0;
            for (int i = 0; i < list.size(); ++i) {
                const Utterance &u = list.at(i);
                ms += qint64(u.text.length()) * FakeSpeechEngine::msPerCharacter;
                // The pauses inside a text, and the one between two texts; none after the last.
                for (const SpeechText::Segment &part : SpeechText::segments(u.text, SpeechTiming::sentencePauseMs, SpeechTiming::clausePauseMs))
                    ms += part.pauseMs;
                if (i + 1 < list.size())
                    ms += SpeechTiming::betweenTextsMs;
            }
            if (list.isEmpty()) {
                apply(engine_->voiceFinished());
            } else {
                voiceEnd_ = clock_.nowMs() + ms;
            }
        }
    }
};

}

void TstSpeech::restIsNotShortenedAndTheStepWaits()
{
    tempoSnapshot().clear();
    Run run(planOf(QList<Step>() << rest(5) << work("long", 5)), 0);
    run.start();
    // The rest sentence, then the announcement with its description.
    QCOMPARE(run.spoken_.size(), 1);
    QCOMPARE(run.spoken_.first().size(), 3);
    const qint64 voiceMs = qint64(run.voiceEnd_);
    QVERIFY(voiceMs > 5000);
    run.at(5000);
    QVERIFY(run.engine_->state() == SessionState::WaitingForVoice);
    QCOMPARE(run.timeOf(EventType::StepStarted, 1), qint64(-1));
    run.at(voiceMs - 1);
    QVERIFY(run.engine_->state() == SessionState::WaitingForVoice);
    run.at(voiceMs);
    QVERIFY(run.engine_->state() == SessionState::Running);
    QCOMPARE(run.timeOf(EventType::StepStarted, 1), voiceMs);
    // The rest itself kept its set time; the step runs its full time from the end of the voice.
    run.at(voiceMs + 5000);
    QVERIFY(run.engine_->state() == SessionState::Finished);
    QCOMPARE(run.engine_->records().first().activeMs, qint64(5000));
}

void TstSpeech::voiceFinishedInTheRestMeansNoWait()
{
    // The second step is announced in a rest that is longer than the voice.
    Run run(planOf(QList<Step>() << rest(20) << work("walk", 5)), 0);
    run.start();
    QVERIFY(run.voiceEnd_ < 20000);
    run.at(20000);
    QCOMPARE(run.timeOf(EventType::StepStarted, 1), qint64(20000));
    QVERIFY(run.engine_->state() == SessionState::Running);
}

void TstSpeech::firstStepWaitsAfterTheLeadIn()
{
    Run run(planOf(QList<Step>() << work("squat", 5)), 2);
    run.start();
    QCOMPARE(run.spoken_.size(), 1);
    QCOMPARE(run.spoken_.first().size(), 2);
    const qint64 voiceMs = run.voiceEnd_;
    QVERIFY(voiceMs > 2000);
    run.at(2000);
    QVERIFY(run.engine_->state() == SessionState::WaitingForVoice);
    run.at(voiceMs);
    QCOMPARE(run.timeOf(EventType::StepStarted, 0), voiceMs);
}

void TstSpeech::pauseStopsAndResumeRepeats()
{
    Run run(planOf(QList<Step>() << rest(5) << work("long", 5)), 0);
    run.start();
    run.at(1000);
    run.pause();
    QCOMPARE(run.stopped_, 1);
    QCOMPARE(run.voiceEnd_, qint64(-1));
    QVERIFY(run.engine_->state() == SessionState::Paused);
    run.at(5000);
    // Nothing is spoken while paused.
    QCOMPARE(run.spoken_.size(), 1);
    run.clock_.set(5000);
    run.resume();
    // The announcement of the next step again from its start: no rest sentence.
    QCOMPARE(run.spoken_.size(), 2);
    QCOMPARE(run.spoken_.last().size(), 2);
    QVERIFY(run.spoken_.last().first().startsWith("Get ready"));
    QVERIFY(run.voiceEnd_ > 5000);
}

void TstSpeech::pausesLengthenTheVoiceAndTheStepStillWaitsForTheLastWord()
{
    // "Long" has a description of two sentences: 600 ms inside it, 600 ms between it and the head,
    // 600 ms inside the head; nothing after the last word.
    Run run(planOf(QList<Step>() << work("long", 5)), 0);
    run.start();
    qint64 speech = 0;
    for (const QString &text : run.spoken_.first())
        speech += qint64(text.length()) * FakeSpeechEngine::msPerCharacter;
    const qint64 pauses = 600 /* head: after its first sentence */ + 600 /* between head and description */
        + 600 /* inside the description */;
    QCOMPARE(run.voiceEnd_, speech + pauses);
    run.at(speech + pauses - 1);
    QVERIFY(run.engine_->state() == SessionState::WaitingForVoice);
    run.at(speech + pauses);
    QVERIFY(run.engine_->state() == SessionState::Running);
    QCOMPARE(run.timeOf(EventType::StepStarted, 0), speech + pauses);
}

void TstSpeech::pauseWhileTheStepWaits()
{
    Run run(planOf(QList<Step>() << work("squat", 5)), 0);
    run.start();
    run.at(100);
    QVERIFY(run.engine_->state() == SessionState::WaitingForVoice);
    run.pause();
    QVERIFY(run.engine_->state() == SessionState::Paused);
    run.clock_.set(10000);
    run.resume();
    QVERIFY(run.engine_->state() == SessionState::WaitingForVoice);
    QCOMPARE(run.spoken_.size(), 2);
    QCOMPARE(run.spoken_.last().size(), 2);
    const qint64 end = run.voiceEnd_;
    QVERIFY(end > 10000);
    run.at(end);
    QVERIFY(run.engine_->state() == SessionState::Running);
    QCOMPARE(run.timeOf(EventType::StepStarted, 0), end);
    QCOMPARE(run.count(EventType::StepStarted), 1);
}

void TstSpeech::noVoiceMeansNoWait()
{
    // A language with no voice: the announcement is skipped and the step starts on time.
    Run run(planOf(QList<Step>() << rest(3) << work("plank", 5)), 0);
    run.voices_ = FakeVoiceProvider(QStringList() << "en");
    run.start();
    run.at(3000);
    QCOMPARE(run.timeOf(EventType::StepStarted, 1), qint64(3000));
    QVERIFY(run.engine_->state() == SessionState::Running);
}

void TstSpeech::noEventsWithoutAnnouncements()
{
    Run run(planOf(QList<Step>() << rest(3) << work("squat", 5)), 2, false);
    run.start();
    run.at(3000);
    run.at(5000);
    QCOMPARE(run.count(EventType::Announce), 0);
    QVERIFY(run.engine_->state() != SessionState::WaitingForVoice);
    QCOMPARE(run.timeOf(EventType::StepStarted, 1), qint64(5000));
}

void TstSpeech::skippingAnnouncesTheStep()
{
    Run run(planOf(QList<Step>() << work("walk", 30) << work("squat", 30)), 0);
    run.start();
    run.at(10);
    QCOMPARE(run.spoken_.size(), 1);
    run.skip();
    QCOMPARE(run.stopped_, 1);
    QCOMPARE(run.spoken_.size(), 2);
    QVERIFY(run.spoken_.last().first().contains("Squat"));
    QVERIFY(run.engine_->state() == SessionState::WaitingForVoice);
    QCOMPARE(run.engine_->stepIndex(), 1);
}

void TstSpeech::timingValuesAreThoseOfTheRequirements()
{
    // The gap between the end of the voice and the start of the timer is one named value, 0 by default.
    QCOMPARE(SpeechTiming::voiceToTimerGapMs, 0);
    QCOMPARE(SpeechTiming::minRestSentenceSec, 5);
    QCOMPARE(SpeechTiming::restInMinutesSec, 90);
    // The window covers the announcement being spoken and at least the whole of the next step.
    QVERIFY(SpeechTiming::lookaheadSteps >= 2);
}

QTEST_APPLESS_MAIN(TstSpeech)

#include "tst_speech.moc"

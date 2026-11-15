// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "engine.h"
#include "fakeclock.h"
#include "fakespeech.h"
#include "speech.h"
#include "spokencues.h"
#include "testsupport.h"

// Cues spoken during a step (R-ALR-15): which cues a step has and when they fall, the selection of the
// phase names, and what is skipped when the voice is busy or absent.
namespace {

using SpokenCues::Cue;
using SpokenCues::Kind;
using SpokenCues::PhaseNames;

class TestSentences : public Sentences
{
public:
    QString text(const QString &language, Sentence sentence, int) const override
    {
        QString s;
        switch (sentence) {
        case Sentence::GetReadyToPerform: s = QStringLiteral("Get ready to perform %1"); break;
        case Sentence::GetReady: s = QStringLiteral("Get ready"); break;
        case Sentence::Repetitions: s = QStringLiteral("repetitions"); break;
        case Sentence::Seconds: s = QStringLiteral("seconds"); break;
        case Sentence::Minutes: s = QStringLiteral("minutes"); break;
        case Sentence::RestFor: s = QStringLiteral("Rest for %1"); break;
        case Sentence::Halfway: s = QStringLiteral("Halfway"); break;
        case Sentence::TenSecondsLeft: s = QStringLiteral("Ten seconds left"); break;
        case Sentence::ToGo: s = QStringLiteral("%1 to go"); break;
        case Sentence::ThreeToGo: s = QStringLiteral("Three to go"); break;
        }
        return language == QLatin1String("en") ? s : QStringLiteral("[%1] ").arg(language) + s;
    }
};

QString name(Kind kind)
{
    switch (kind) {
    case Kind::Halfway: return QStringLiteral("half");
    case Kind::TenSecondsLeft: return QStringLiteral("ten");
    case Kind::ToGo: return QStringLiteral("togo");
    case Kind::ThreeToGo: return QStringLiteral("three");
    }
    return QString();
}

// "half@22500 ten@35000" for the period cues of a length in seconds.
QStringList periodLines(int seconds)
{
    QStringList out;
    for (const Cue &c : SpokenCues::periodCues(qint64(seconds) * 1000))
        out << QStringLiteral("%1@%2").arg(name(c.kind)).arg(c.atMs);
    return out;
}

// "togo r6 n7@5000": the call, the repetition it falls at, the repetitions left, the time.
QStringList tempoLines(const Tempo &tempo)
{
    QStringList out;
    for (const Cue &c : SpokenCues::tempoCues(tempo))
        out << QStringLiteral("%1 r%2 n%3@%4").arg(name(c.kind)).arg(c.repetition).arg(c.count).arg(c.atMs);
    return out;
}

// An engine with the spoken cues on a fake clock, with the events it reported.
struct Rig {
    Rig() { tempoSnapshot().clear(); }
    ~Rig() { tempoSnapshot().clear(); }
    FakeClock clock;
    QScopedPointer<SessionEngine> engine;
    QList<EngineEvent> log;

    void make(const Workout &w, bool cues = true)
    {
        EngineConfig config;
        config.leadInSec = 0;
        config.spokenCues = cues;
        engine.reset(new SessionEngine(&clock, w, tempoSnapshot(), config));
        clock.set(0);
        log = engine->start();
    }
    void at(qint64 ms)
    {
        clock.set(ms);
        log += engine->update();
    }
    // Runs to ms in steps of 50 ms.
    void runTo(qint64 ms)
    {
        while (clock.nowMs() < ms)
            at(qMin(clock.nowMs() + 50, ms));
    }
    QStringList cues() const
    {
        QStringList out;
        for (const EngineEvent &e : log) {
            if (e.type != EventType::Cue)
                continue;
            QString line = QStringLiteral("%1@%2").arg(name(e.cue)).arg(e.atMs);
            if (e.cue == Kind::ToGo || e.cue == Kind::ThreeToGo)
                line += QStringLiteral(" r%1 n%2").arg(e.repetition).arg(e.cueCount);
            out << line;
        }
        return out;
    }
    int count(EventType type) const
    {
        int n = 0;
        for (const EngineEvent &e : log)
            n += e.type == type ? 1 : 0;
        return n;
    }
};

Workout single(const QList<Step> &steps)
{
    return workout(QList<Block>() << block(steps));
}

QList<PlanStep> planOf(const QList<Step> &steps)
{
    return expandPlan(single(steps), tempoSnapshot());
}

EngineEvent cueEvent(int step, Kind kind, int repetition = 0, int count = 0)
{
    EngineEvent e;
    e.type = EventType::Cue;
    e.stepIndex = step;
    e.cue = kind;
    e.repetition = repetition;
    e.cueCount = count;
    return e;
}

EngineEvent phaseEvent(int step, int phase, int repetition)
{
    EngineEvent e;
    e.type = EventType::Beat;
    e.stepIndex = step;
    e.phase = phase;
    e.repetition = repetition;
    return e;
}

// The phases of a rhythm: three named phases of one second each.
Tempo namedRhythm(int reps)
{
    return Tempo::phases(QList<int>() << 2 << 2 << 2, reps, QStringList() << "Down" << "Hold" << "Up");
}

struct Direct {
    explicit Direct(const SpokenCues::Settings &settings = SpokenCues::Settings())
        : director(&planner, &voices, [this](const Utterance &) { return usable; }, settings) {}
    TestSentences sentences;
    SpeechPlanner planner{&sentences, QHash<QString, SpokenExercise>(), "en"};
    FakeVoiceProvider voices;
    bool usable = true;
    SpeechDirector director;
};

}

class TstSpokenCues : public QObject
{
    Q_OBJECT

private slots:
    void periodCuesOfDurationSteps_data();
    void periodCuesOfDurationSteps();
    void tempoCuesOfTempoSteps();
    void aShortTempoStepHasNoLastCall();
    void settingValuesSelectThePhaseNames();
    void unknownSettingValueIsTheDefault();
    void engineReportsPeriodCuesAtTheHaloTimes();
    void aRestHasTheCuesOfItsLength();
    void aTempoStepHasNoPeriodCues();
    void engineReportsRepetitionCallsAtTheirRepetition();
    void noCueEventsUnlessAsked();
    void tickEventsAreTheSameWithCues();
    void pauseDoesNotRepeatOrMoveACueThatWasReported();
    void pauseBeforeACueMovesItWithTheEnd();
    void aCuePointJustAfterAPauseKeepsItsPlaceOnTheRunningTime();
    void aRepetitionRunAgainAfterAPauseIsNotCalledAgain();
    void arbiterSkipsCuesDuringAnAnnouncement();
    void aNameIsCutByTheNextPhase();
    void aNameWinsOverTheRepetitionCall();
    void aCallFallingInsideANameIsSkippedNotDelayed();
    void callIsSpokenWhenTheNameIsNotSelected();
    void plannerTextsOfTheCues();
    void groupsCarryTheCueTextsOnlyWithTheSettings();
    void directorSpeaksTheCueOfAnEvent();
    void directorSkipsCuesWhenTheSwitchIsOff();
    void directorSkipsCuesWithoutAVoice();
    void directorSkipsACueWhoseTextFailed();
    void directorSpeaksPhaseNamesBySetting();
    void directorSpeaksNoNameOfAnUnnamedPhase();
    void directorSkipsNamesDuringAnAnnouncement();
};

void TstSpokenCues::periodCuesOfDurationSteps_data()
{
    QTest::addColumn<int>("seconds");
    QTest::addColumn<QStringList>("expected");
    QTest::newRow("8 s") << 8 << QStringList();
    QTest::newRow("10 s") << 10 << QStringList();
    QTest::newRow("12 s") << 12 << (QStringList() << "ten@2000");
    QTest::newRow("44 s") << 44 << (QStringList() << "ten@34000");
    QTest::newRow("45 s") << 45 << (QStringList() << "half@22500" << "ten@35000");
    QTest::newRow("90 s") << 90 << (QStringList() << "half@45000" << "ten@80000");
}

void TstSpokenCues::periodCuesOfDurationSteps()
{
    QFETCH(int, seconds);
    QFETCH(QStringList, expected);
    QCOMPARE(periodLines(seconds), expected);
}

void TstSpokenCues::tempoCuesOfTempoSteps()
{
    // 4 repetitions: only the last call, at the second repetition.
    QCOMPARE(tempoLines(Tempo::timePerRep(1000, 4)), QStringList() << "three r2 n3@1000");
    // 6: the middle at the third repetition with four left, the last call at the fourth.
    QCOMPARE(tempoLines(Tempo::timePerRep(1000, 6)), QStringList() << "togo r3 n4@2000" << "three r4 n3@3000");
    // 12: the middle at the sixth with seven left, the last call at the tenth.
    QCOMPARE(tempoLines(Tempo::timePerRep(1000, 12)), QStringList() << "togo r6 n7@5000" << "three r10 n3@9000");
    // An odd number: the middle repetition is the one after the half.
    QCOMPARE(tempoLines(Tempo::timePerRep(1000, 7)), QStringList() << "togo r4 n4@3000" << "three r5 n3@4000");
}

void TstSpokenCues::aShortTempoStepHasNoLastCall()
{
    QVERIFY(tempoLines(Tempo::timePerRep(1000, 3)).isEmpty());
    QVERIFY(tempoLines(Tempo::timePerRep(1000, 1)).isEmpty());
    // The middle call of a step of 5 would meet the last call: it needs 6.
    QCOMPARE(tempoLines(Tempo::timePerRep(1000, 5)), QStringList() << "three r3 n3@2000");
}

void TstSpokenCues::settingValuesSelectThePhaseNames()
{
    const int reps[] = { 1, 2, 3, 4, 10 };
    const bool none[] = { false, false, false, false, false };
    const bool first[] = { true, false, false, false, false };
    const bool firstThree[] = { true, true, true, false, false };
    const bool all[] = { true, true, true, true, true };
    for (int i = 0; i < 5; ++i) {
        QCOMPARE(SpokenCues::phaseNameSpoken(PhaseNames::None, reps[i]), none[i]);
        QCOMPARE(SpokenCues::phaseNameSpoken(PhaseNames::First, reps[i]), first[i]);
        QCOMPARE(SpokenCues::phaseNameSpoken(PhaseNames::FirstThree, reps[i]), firstThree[i]);
        QCOMPARE(SpokenCues::phaseNameSpoken(PhaseNames::All, reps[i]), all[i]);
    }
    QVERIFY(!SpokenCues::phaseNameSpoken(PhaseNames::All, 0));
    QVERIFY(SpokenCues::defaultPhaseNames == PhaseNames::FirstThree);
    QVERIFY(SpokenCues::Settings().cues);
}

void TstSpokenCues::unknownSettingValueIsTheDefault()
{
    QVERIFY(SpokenCues::phaseNamesFromInt(0) == PhaseNames::None);
    QVERIFY(SpokenCues::phaseNamesFromInt(1) == PhaseNames::First);
    QVERIFY(SpokenCues::phaseNamesFromInt(2) == PhaseNames::FirstThree);
    QVERIFY(SpokenCues::phaseNamesFromInt(3) == PhaseNames::All);
    QVERIFY(SpokenCues::phaseNamesFromInt(-1) == SpokenCues::defaultPhaseNames);
    QVERIFY(SpokenCues::phaseNamesFromInt(9) == SpokenCues::defaultPhaseNames);
}

void TstSpokenCues::engineReportsPeriodCuesAtTheHaloTimes()
{
    const struct { int seconds; QStringList expected; } cases[] = {
        { 8, QStringList() },
        { 12, QStringList() << "ten@2000" },
        { 45, QStringList() << "half@22500" << "ten@35000" },
        { 90, QStringList() << "half@45000" << "ten@80000" },
    };
    for (const auto &c : cases) {
        Rig rig;
        rig.make(single(QList<Step>() << work("a", c.seconds)));
        rig.runTo(qint64(c.seconds) * 1000 + 500);
        QCOMPARE(rig.cues(), c.expected);
    }
    // The ten-second cue falls with the tick of 10 s.
    Rig rig;
    rig.make(single(QList<Step>() << work("a", 90)));
    rig.runTo(91000);
    qint64 tickAt = -1;
    for (const EngineEvent &e : rig.log) {
        if (e.type == EventType::Tick && e.remainingSec == 10)
            tickAt = e.atMs;
    }
    QCOMPARE(tickAt, qint64(80000));
}

void TstSpokenCues::aRestHasTheCuesOfItsLength()
{
    Rig rig;
    rig.make(single(QList<Step>() << rest(60) << work("a", 8)));
    rig.runTo(61000);
    QCOMPARE(rig.cues(), QStringList() << "half@30000" << "ten@50000");
}

void TstSpokenCues::aTempoStepHasNoPeriodCues()
{
    Rig rig;
    // 60 repetitions of one second: a minute, with no halfway and no ten seconds.
    rig.make(single(QList<Step>() << tempoStep("t", Tempo::timePerRep(1000, 60))));
    rig.runTo(61000);
    for (const EngineEvent &e : rig.log) {
        if (e.type == EventType::Cue)
            QVERIFY(e.cue == Kind::ToGo || e.cue == Kind::ThreeToGo);
    }
    QCOMPARE(rig.count(EventType::Tick), 0);
}

void TstSpokenCues::engineReportsRepetitionCallsAtTheirRepetition()
{
    const struct { int reps; QStringList expected; } cases[] = {
        { 4, QStringList() << "three@1000 r2 n3" },
        { 6, QStringList() << "togo@2000 r3 n4" << "three@3000 r4 n3" },
        { 12, QStringList() << "togo@5000 r6 n7" << "three@9000 r10 n3" },
    };
    for (const auto &c : cases) {
        Rig rig;
        rig.make(single(QList<Step>() << tempoStep("t", Tempo::timePerRep(1000, c.reps))));
        rig.runTo(qint64(c.reps) * 1000 + 500);
        QCOMPARE(rig.cues(), c.expected);
    }
    // The call comes right after the beat that starts the repetition, at the same time.
    Rig rig;
    rig.make(single(QList<Step>() << tempoStep("t", Tempo::timePerRep(1000, 6))));
    rig.runTo(7000);
    for (int i = 0; i < rig.log.size(); ++i) {
        if (rig.log.at(i).type != EventType::Cue)
            continue;
        QVERIFY(i > 0);
        QCOMPARE(int(rig.log.at(i - 1).type), int(EventType::Beat));
        QCOMPARE(rig.log.at(i - 1).atMs, rig.log.at(i).atMs);
        QCOMPARE(rig.log.at(i - 1).repetition, rig.log.at(i).repetition);
    }
}

void TstSpokenCues::noCueEventsUnlessAsked()
{
    Rig rig;
    rig.make(single(QList<Step>() << work("a", 90) << tempoStep("t", Tempo::timePerRep(1000, 12))), false);
    rig.runTo(110000);
    QCOMPARE(rig.count(EventType::Cue), 0);
}

void TstSpokenCues::tickEventsAreTheSameWithCues()
{
    Rig with;
    with.make(single(QList<Step>() << work("a", 90)), true);
    with.runTo(91000);
    Rig without;
    without.make(single(QList<Step>() << work("a", 90)), false);
    without.runTo(91000);
    QList<EngineEvent> a;
    QList<EngineEvent> b;
    for (const EngineEvent &e : with.log) {
        if (e.type != EventType::Cue)
            a.append(e);
    }
    b = without.log;
    QCOMPARE(a.size(), b.size());
    for (int i = 0; i < a.size(); ++i) {
        QCOMPARE(int(a.at(i).type), int(b.at(i).type));
        QCOMPARE(a.at(i).atMs, b.at(i).atMs);
    }
}

void TstSpokenCues::pauseDoesNotRepeatOrMoveACueThatWasReported()
{
    Rig rig;
    rig.make(single(QList<Step>() << work("a", 90)));
    rig.runTo(46000);
    QCOMPARE(rig.cues(), QStringList() << "half@45000");
    rig.log += rig.engine->pause();
    rig.clock.set(60000);
    rig.log += rig.engine->resume();
    rig.runTo(130000);
    // The step has 44 s left at the resume; ten seconds left comes at 80 s of the step, 14 s later.
    QCOMPARE(rig.cues(), QStringList() << "half@45000" << "ten@94000");
}

void TstSpokenCues::pauseBeforeACueMovesItWithTheEnd()
{
    Rig rig;
    rig.make(single(QList<Step>() << work("a", 90)));
    rig.runTo(20000);
    rig.log += rig.engine->pause();
    rig.clock.set(50000);
    rig.log += rig.engine->resume();
    rig.runTo(130000);
    // 30 s of the pause: halfway at 75 s, ten seconds left at 110 s.
    QCOMPARE(rig.cues(), QStringList() << "half@75000" << "ten@110000");
}

void TstSpokenCues::aCuePointJustAfterAPauseKeepsItsPlaceOnTheRunningTime()
{
    Rig rig;
    rig.make(single(QList<Step>() << work("a", 90)));
    rig.runTo(44900);
    rig.log += rig.engine->pause();
    // The clock goes on while the step is paused: that time is not run, so the middle is 100 ms after the resume.
    rig.clock.set(70000);
    rig.log += rig.engine->resume();
    rig.runTo(150000);
    QCOMPARE(rig.cues().first(), QStringLiteral("half@70100"));
    QCOMPARE(rig.cues().size(), 2);
}

void TstSpokenCues::aRepetitionRunAgainAfterAPauseIsNotCalledAgain()
{
    Rig rig;
    rig.make(single(QList<Step>() << tempoStep("t", Tempo::timePerRep(1000, 6))));
    rig.runTo(3200);
    rig.log += rig.engine->pause();
    rig.clock.set(10000);
    rig.log += rig.engine->resume();
    rig.runTo(20000);
    // The fourth repetition (the last call) started before the pause and starts again after it.
    int three = 0;
    for (const EngineEvent &e : rig.log)
        three += e.type == EventType::Cue && e.cue == Kind::ThreeToGo ? 1 : 0;
    QCOMPARE(three, 1);
}

void TstSpokenCues::arbiterSkipsCuesDuringAnAnnouncement()
{
    CueArbiter arbiter;
    arbiter.setAnnouncing(true);
    QVERIFY(!arbiter.onPeriodCue().speak);
    QVERIFY(!arbiter.onCountCue(false).speak);
    QVERIFY(!arbiter.onPhaseStart(true).speak);
    // Skipped, not delayed: when the announcement is over nothing comes back.
    arbiter.setAnnouncing(false);
    QVERIFY(arbiter.onPeriodCue().speak);
    QVERIFY(arbiter.onCountCue(false).speak);
}

void TstSpokenCues::aNameIsCutByTheNextPhase()
{
    CueArbiter arbiter;
    CueArbiter::Order first = arbiter.onPhaseStart(true);
    QVERIFY(first.speak && !first.cutName);
    QVERIFY(arbiter.nameSpeaking());
    // The next phase starts while the name is spoken: it is cut, also when the next phase has no name.
    CueArbiter::Order second = arbiter.onPhaseStart(false);
    QVERIFY(!second.speak && second.cutName);
    QVERIFY(!arbiter.nameSpeaking());
    CueArbiter::Order third = arbiter.onPhaseStart(true);
    QVERIFY(third.speak && !third.cutName);
    CueArbiter::Order fourth = arbiter.onPhaseStart(true);
    QVERIFY(fourth.speak && fourth.cutName);
    // A name that was finished is not cut.
    arbiter.nameEnded();
    QVERIFY(!arbiter.onPhaseStart(true).cutName);
}

void TstSpokenCues::aNameWinsOverTheRepetitionCall()
{
    CueArbiter arbiter;
    // The phase that starts with the call has a name to speak.
    arbiter.onPhaseStart(true);
    const CueArbiter::Order call = arbiter.onCountCue(true);
    QVERIFY(!call.speak);
    // The name is not cut by the skipped call.
    QVERIFY(!call.cutName);
    QVERIFY(arbiter.nameSpeaking());
}

void TstSpokenCues::aCallFallingInsideANameIsSkippedNotDelayed()
{
    CueArbiter arbiter;
    arbiter.onPhaseStart(true);
    QVERIFY(!arbiter.onCountCue(false).speak);
    arbiter.nameEnded();
    // The same call asked again after the name ended is a new call; the one that fell is gone.
    QVERIFY(arbiter.onCountCue(false).speak);
}

void TstSpokenCues::callIsSpokenWhenTheNameIsNotSelected()
{
    CueArbiter arbiter;
    arbiter.onPhaseStart(false);
    QVERIFY(arbiter.onCountCue(false).speak);
}

void TstSpokenCues::plannerTextsOfTheCues()
{
    tempoSnapshot().clear();
    TestSentences sentences;
    const SpeechPlanner planner(&sentences, QHash<QString, SpokenExercise>(), "it");
    QCOMPARE(planner.cueUtterance(Kind::Halfway, 0).text, QStringLiteral("[it] Halfway"));
    QCOMPARE(planner.cueUtterance(Kind::TenSecondsLeft, 0).text, QStringLiteral("[it] Ten seconds left"));
    QCOMPARE(planner.cueUtterance(Kind::ToGo, 7).text, QStringLiteral("[it] 7 to go"));
    QCOMPARE(planner.cueUtterance(Kind::ThreeToGo, 3).text, QStringLiteral("[it] Three to go"));
    // The language of a cue is that of the interface, not of the exercise.
    QCOMPARE(planner.cueUtterance(Kind::Halfway, 0).language, QStringLiteral("it"));
    QVERIFY(planner.cueUtterance(Kind::Halfway, 0).kind == UtteranceKind::Cue);
    QCOMPARE(planner.phaseUtterance(" Down ").text, QStringLiteral("Down"));
    QVERIFY(planner.phaseUtterance("  ").isEmpty());
}

void TstSpokenCues::groupsCarryTheCueTextsOnlyWithTheSettings()
{
    tempoSnapshot().clear();
    TestSentences sentences;
    const SpeechPlanner planner(&sentences, QHash<QString, SpokenExercise>(), "en");
    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 45) << tempoStep("t", namedRhythm(6)));
    QVERIFY(plan.at(1).hasTempo);
    auto cueTexts = [&](const SpokenCues::Settings &s) {
        QStringList out;
        for (const SpeechGroup &g : planner.groups(plan, s)) {
            for (const Utterance &u : g.utterances) {
                if (u.kind == UtteranceKind::Cue)
                    out << QStringLiteral("%1:%2").arg(g.planIndex).arg(u.text);
            }
        }
        return out;
    };
    QVERIFY(cueTexts(SpokenCues::Settings::off()).isEmpty());
    QVERIFY(planner.groups(plan).size() == planner.groups(plan, SpokenCues::Settings::off()).size());
    QCOMPARE(cueTexts(SpokenCues::Settings()),
             QStringList() << "0:Halfway" << "0:Ten seconds left" << "1:4 to go" << "1:Three to go"
                           << "1:Down" << "1:Hold" << "1:Up");
    SpokenCues::Settings noCues;
    noCues.cues = false;
    QCOMPARE(cueTexts(noCues), QStringList() << "1:Down" << "1:Hold" << "1:Up");
    SpokenCues::Settings noNames;
    noNames.phaseNames = PhaseNames::None;
    QCOMPARE(cueTexts(noNames), QStringList() << "0:Halfway" << "0:Ten seconds left" << "1:4 to go" << "1:Three to go");
    tempoSnapshot().clear();
}

void TstSpokenCues::directorSpeaksTheCueOfAnEvent()
{
    tempoSnapshot().clear();
    Direct d;
    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 90) << tempoStep("t", Tempo::timePerRep(1000, 12)));
    QCOMPARE(d.director.onCue(cueEvent(0, Kind::Halfway), plan).utterance.text, QStringLiteral("Halfway"));
    QCOMPARE(d.director.onCue(cueEvent(0, Kind::TenSecondsLeft), plan).utterance.text, QStringLiteral("Ten seconds left"));
    QCOMPARE(d.director.onCue(cueEvent(1, Kind::ToGo, 6, 7), plan).utterance.text, QStringLiteral("7 to go"));
    QCOMPARE(d.director.onCue(cueEvent(1, Kind::ThreeToGo, 10, 3), plan).utterance.text, QStringLiteral("Three to go"));
    tempoSnapshot().clear();
}

void TstSpokenCues::directorSkipsCuesWhenTheSwitchIsOff()
{
    tempoSnapshot().clear();
    SpokenCues::Settings settings;
    settings.cues = false;
    Direct d(settings);
    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 90));
    QVERIFY(d.director.onCue(cueEvent(0, Kind::Halfway), plan).utterance.isEmpty());
    tempoSnapshot().clear();
}

void TstSpokenCues::directorSkipsCuesWithoutAVoice()
{
    tempoSnapshot().clear();
    // No voice reads the language of the interface.
    Direct noVoice;
    noVoice.voices = FakeVoiceProvider(QStringList() << "it");
    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 90) << tempoStep("t", namedRhythm(6)));
    QVERIFY(noVoice.director.onCue(cueEvent(0, Kind::Halfway), plan).utterance.isEmpty());
    QVERIFY(noVoice.director.onPhase(phaseEvent(1, 1, 1), plan).utterance.isEmpty());
    // Nothing is left spoken: the next name is not cut and the next call is not held back.
    QVERIFY(!noVoice.director.nameSpeaking());
    QVERIFY(!noVoice.director.onCue(cueEvent(1, Kind::ThreeToGo, 4, 3), plan).cutName);
    tempoSnapshot().clear();
}

void TstSpokenCues::directorSkipsACueWhoseTextFailed()
{
    tempoSnapshot().clear();
    Direct d;
    d.usable = false;
    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 90));
    QVERIFY(d.director.onCue(cueEvent(0, Kind::Halfway), plan).utterance.isEmpty());
    tempoSnapshot().clear();
}

void TstSpokenCues::directorSpeaksPhaseNamesBySetting()
{
    tempoSnapshot().clear();
    const QList<PlanStep> plan = planOf(QList<Step>() << tempoStep("t", namedRhythm(5)));
    const struct { PhaseNames names; QList<int> repetitions; } cases[] = {
        { PhaseNames::None, QList<int>() },
        { PhaseNames::First, QList<int>() << 1 },
        { PhaseNames::FirstThree, QList<int>() << 1 << 2 << 3 },
        { PhaseNames::All, QList<int>() << 1 << 2 << 3 << 4 << 5 },
    };
    for (const auto &c : cases) {
        SpokenCues::Settings settings;
        settings.phaseNames = c.names;
        Direct d(settings);
        QList<int> spoken;
        QStringList firstRepetition;
        for (int rep = 1; rep <= 5; ++rep) {
            for (int phase = 1; phase <= 3; ++phase) {
                const SpeechDirector::CueOrder order = d.director.onPhase(phaseEvent(0, phase, rep), plan);
                if (order.utterance.isEmpty())
                    continue;
                // Each name is finished before the next phase starts.
                d.director.nameFinished();
                if (!spoken.contains(rep))
                    spoken.append(rep);
                if (rep == 1)
                    firstRepetition << order.utterance.text;
            }
        }
        QCOMPARE(spoken, c.repetitions);
        if (!c.repetitions.isEmpty())
            QCOMPARE(firstRepetition, QStringList() << "Down" << "Hold" << "Up");
    }
    tempoSnapshot().clear();
}

void TstSpokenCues::directorSpeaksNoNameOfAnUnnamedPhase()
{
    tempoSnapshot().clear();
    const QList<PlanStep> plan = planOf(QList<Step>() << tempoStep("t", Tempo::phases(QList<int>() << 2 << 2, 3, QStringList() << "Down" << "")));
    Direct d;
    QCOMPARE(d.director.onPhase(phaseEvent(0, 1, 1), plan).utterance.text, QStringLiteral("Down"));
    // The second phase has no name: nothing is spoken, and the first name is cut.
    const SpeechDirector::CueOrder second = d.director.onPhase(phaseEvent(0, 2, 1), plan);
    QVERIFY(second.utterance.isEmpty());
    QVERIFY(second.cutName);
    // A step without phases has no phase name at all.
    const QList<PlanStep> plain = planOf(QList<Step>() << tempoStep("u", Tempo::timePerRep(1000, 6)));
    QVERIFY(d.director.onPhase(phaseEvent(0, 0, 1), plain).utterance.isEmpty());
    tempoSnapshot().clear();
}

void TstSpokenCues::directorSkipsNamesDuringAnAnnouncement()
{
    tempoSnapshot().clear();
    const QList<PlanStep> plan = planOf(QList<Step>() << work("a", 90) << tempoStep("t", namedRhythm(6)));
    SpokenCues::Settings all;
    all.phaseNames = PhaseNames::All;
    Direct d(all);
    d.director.setAnnouncing(true);
    QVERIFY(d.director.onPhase(phaseEvent(1, 1, 1), plan).utterance.isEmpty());
    QVERIFY(d.director.onCue(cueEvent(0, Kind::Halfway), plan).utterance.isEmpty());
    QVERIFY(d.director.onCue(cueEvent(1, Kind::ThreeToGo, 4, 3), plan).utterance.isEmpty());
    d.director.setAnnouncing(false);
    QVERIFY(!d.director.onCue(cueEvent(0, Kind::Halfway), plan).utterance.isEmpty());
    // A call at the start of a phase whose name is spoken is skipped and the name is not cut.
    const SpeechDirector::CueOrder name = d.director.onPhase(phaseEvent(1, 1, 4), plan);
    QCOMPARE(name.utterance.text, QStringLiteral("Down"));
    d.director.nameFinished();
    const SpeechDirector::CueOrder call = d.director.onCue(cueEvent(1, Kind::ThreeToGo, 4, 3), plan);
    QVERIFY(call.utterance.isEmpty());
    QVERIFY(!call.cutName);
    tempoSnapshot().clear();
}

QTEST_APPLESS_MAIN(TstSpokenCues)
#include "tst_spokencues.moc"

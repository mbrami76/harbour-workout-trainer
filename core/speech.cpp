// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "speech.h"

QString SpeechPlanner::languageOf(const PlanStep &step) const
{
    const QString tag = exercises_.value(step.exerciseKey).language;
    return tag.isEmpty() ? appLanguage_ : tag;
}

QString SpeechPlanner::amount(const PlanStep &step, const QString &language) const
{
    if (step.isTimed()) {
        const int minutes = step.durationSec / 60;
        const int seconds = step.durationSec % 60;
        if (minutes == 0)
            return sentences_->text(language, Sentence::Seconds, seconds);
        if (seconds == 0)
            return sentences_->text(language, Sentence::Minutes, minutes);
        return sentences_->text(language, Sentence::Minutes, minutes) + QLatin1Char(' ')
            + sentences_->text(language, Sentence::Seconds, seconds);
    }
    if (step.reps > 0)
        return sentences_->text(language, Sentence::Repetitions, step.reps);
    return QString();
}

QString SpeechPlanner::restAmount(int seconds, const QString &language) const
{
    if (seconds < SpeechTiming::restInMinutesSec)
        return sentences_->text(language, Sentence::Seconds, seconds);
    const int minutes = seconds / 60;
    const int rest = seconds % 60;
    const QString m = sentences_->text(language, Sentence::Minutes, minutes);
    return rest == 0 ? m : m + QLatin1Char(' ') + sentences_->text(language, Sentence::Seconds, rest);
}

Utterance SpeechPlanner::restSentence(const PlanStep *following, const PlanStep &rest) const
{
    Utterance u;
    u.kind = UtteranceKind::RestSentence;
    if (rest.durationSec < SpeechTiming::minRestSentenceSec)
        return u;
    u.language = following ? languageOf(*following) : appLanguage_;
    u.text = sentences_->text(u.language, Sentence::RestFor).arg(restAmount(rest.durationSec, u.language)) + QLatin1Char('.');
    return u;
}

QList<Utterance> SpeechPlanner::announcement(const PlanStep &step, bool first) const
{
    QList<Utterance> list;
    const SpokenExercise exercise = exercises_.value(step.exerciseKey);
    const QString language = languageOf(step);
    Utterance head;
    head.kind = UtteranceKind::Announcement;
    head.language = language;
    head.exerciseKey = step.exerciseKey;
    const QString amountText = amount(step, language);
    QString sentence = exercise.name.isEmpty() ? sentences_->text(language, Sentence::GetReady)
                                               : sentences_->text(language, Sentence::GetReadyToPerform).arg(exercise.name);
    head.text = amountText.isEmpty() ? sentence + QLatin1Char('.') : sentence + QStringLiteral(". ") + amountText + QLatin1Char('.');
    list.append(head);
    if (!first || exercise.name.isEmpty())
        return list;
    if (!exercise.instructions.trimmed().isEmpty()) {
        Utterance d;
        d.kind = UtteranceKind::Description;
        d.language = language;
        d.exerciseKey = step.exerciseKey;
        d.text = exercise.instructions.trimmed();
        list.append(d);
    }
    return list;
}

QList<SpeechGroup> SpeechPlanner::groups(const QList<PlanStep> &plan, const SpokenCues::Settings &cues) const
{
    QList<SpeechGroup> out;
    QSet<QString> described;
    auto announces = [](const PlanStep &s) { return s.type != StepType::Rest && !s.exerciseKey.isEmpty(); };
    auto addAnnouncement = [&](SpeechGroup *g, const PlanStep &step) {
        const bool first = !step.exerciseKey.isEmpty() && !described.contains(step.exerciseKey);
        described.insert(step.exerciseKey);
        g->utterances += announcement(step, first);
    };
    QSet<int> announced;
    for (int i = 0; i < plan.size(); ++i) {
        const PlanStep &step = plan.at(i);
        SpeechGroup g;
        g.planIndex = i;
        g.targetIndex = i;
        if (step.type == StepType::Rest) {
            const PlanStep *following = i + 1 < plan.size() && announces(plan.at(i + 1)) ? &plan.at(i + 1) : nullptr;
            const Utterance sentence = restSentence(following, step);
            if (!sentence.isEmpty())
                g.utterances.append(sentence);
            if (following) {
                addAnnouncement(&g, *following);
                g.targetIndex = i + 1;
                announced.insert(i + 1);
            }
        } else if (announces(step) && !announced.contains(i)) {
            addAnnouncement(&g, step);
        }
        if (!g.utterances.isEmpty())
            out.append(g);
        SpeechGroup c;
        c.planIndex = i;
        c.targetIndex = i;
        c.utterances = cueTexts(step, cues);
        if (!c.utterances.isEmpty())
            out.append(c);
    }
    return out;
}

Utterance SpeechPlanner::cueUtterance(SpokenCues::Kind kind, int count) const
{
    Utterance u;
    u.kind = UtteranceKind::Cue;
    u.language = appLanguage_;
    switch (kind) {
    case SpokenCues::Kind::Halfway:
        u.text = sentences_->text(appLanguage_, Sentence::Halfway);
        break;
    case SpokenCues::Kind::TenSecondsLeft:
        u.text = sentences_->text(appLanguage_, Sentence::TenSecondsLeft);
        break;
    case SpokenCues::Kind::ToGo:
        u.text = sentences_->text(appLanguage_, Sentence::ToGo).arg(count);
        break;
    case SpokenCues::Kind::ThreeToGo:
        u.text = sentences_->text(appLanguage_, Sentence::ThreeToGo);
        break;
    }
    return u;
}

Utterance SpeechPlanner::phaseUtterance(const QString &name) const
{
    Utterance u;
    u.kind = UtteranceKind::Cue;
    u.language = appLanguage_;
    u.text = name.trimmed();
    return u;
}

QList<Utterance> SpeechPlanner::cueTexts(const PlanStep &step, const SpokenCues::Settings &cues) const
{
    QList<Utterance> list;
    auto add = [&list](const Utterance &u) {
        if (u.isEmpty())
            return;
        for (const Utterance &have : list) {
            if (have == u)
                return;
        }
        list.append(u);
    };
    if (step.hasTempo) {
        if (cues.cues) {
            for (const SpokenCues::Cue &c : SpokenCues::tempoCues(step.tempo))
                add(cueUtterance(c.kind, c.count));
        }
        if (cues.phaseNames != SpokenCues::PhaseNames::None) {
            for (int phase = 1; phase <= step.tempo.phaseCount(); ++phase)
                add(phaseUtterance(step.tempo.phaseName(phase)));
        }
    } else if (cues.cues && step.isTimed()) {
        for (const SpokenCues::Cue &c : SpokenCues::periodCues(step.plannedMs()))
            add(cueUtterance(c.kind, c.count));
    }
    return list;
}

QList<Utterance> SpeechDirector::speakable(const QList<Utterance> &utterances) const
{
    QList<Utterance> out;
    for (const Utterance &u : utterances) {
        if (u.isEmpty() || voices_->voiceFor(u.language).isEmpty() || !usable_(u))
            continue;
        out.append(u);
    }
    return out;
}

QList<Utterance> SpeechDirector::onAnnounce(const EngineEvent &event, const QList<PlanStep> &plan)
{
    if (event.repeated) {
        QList<Utterance> again;
        for (const Utterance &u : last_) {
            if (u.kind != UtteranceKind::RestSentence)
                again.append(u);
        }
        return speakable(again);
    }
    QList<Utterance> wanted;
    const PlanStep *target = event.announceIndex >= 0 && event.announceIndex < plan.size() ? &plan.at(event.announceIndex) : nullptr;
    if (event.restSentence && event.stepIndex >= 0 && event.stepIndex < plan.size()) {
        const Utterance sentence = planner_->restSentence(target, plan.at(event.stepIndex));
        if (!sentence.isEmpty())
            wanted.append(sentence);
    }
    if (target) {
        const QString key = target->exerciseKey;
        const bool first = !key.isEmpty() && !described_.contains(key);
        described_.insert(key);
        wanted += planner_->announcement(*target, first);
    }
    last_ = wanted;
    return speakable(wanted);
}

QList<Utterance> SpeechDirector::speakableCue(const Utterance &u) const
{
    if (u.isEmpty() || voices_->voiceFor(u.language).isEmpty() || !usable_(u))
        return QList<Utterance>();
    return QList<Utterance>() << u;
}

SpeechDirector::CueOrder SpeechDirector::onCue(const EngineEvent &event, const QList<PlanStep> &plan)
{
    CueOrder order;
    if (!cues_.cues || event.stepIndex < 0 || event.stepIndex >= plan.size())
        return order;
    const PlanStep &step = plan.at(event.stepIndex);
    CueArbiter::Order decision;
    if (event.cue == SpokenCues::Kind::Halfway || event.cue == SpokenCues::Kind::TenSecondsLeft) {
        decision = arbiter_.onPeriodCue();
    } else {
        // The call falls at the start of a repetition; a name spoken at its first phase wins.
        const bool nameAtStart = step.hasTempo && !step.tempo.phaseName(1).trimmed().isEmpty()
            && SpokenCues::phaseNameSpoken(cues_.phaseNames, event.repetition);
        decision = arbiter_.onCountCue(nameAtStart);
    }
    order.cutName = decision.cutName;
    if (!decision.speak)
        return order;
    const QList<Utterance> list = speakableCue(planner_->cueUtterance(event.cue, event.cueCount));
    if (!list.isEmpty())
        order.utterance = list.first();
    return order;
}

SpeechDirector::CueOrder SpeechDirector::onPhase(const EngineEvent &event, const QList<PlanStep> &plan)
{
    CueOrder order;
    if (event.phase < 1 || event.stepIndex < 0 || event.stepIndex >= plan.size())
        return order;
    const PlanStep &step = plan.at(event.stepIndex);
    const QString name = step.tempo.phaseName(event.phase).trimmed();
    const bool wanted = !name.isEmpty() && SpokenCues::phaseNameSpoken(cues_.phaseNames, event.repetition);
    const CueArbiter::Order decision = arbiter_.onPhaseStart(wanted);
    order.cutName = decision.cutName;
    if (!decision.speak)
        return order;
    const QList<Utterance> list = speakableCue(planner_->phaseUtterance(name));
    if (list.isEmpty()) {
        // Nothing is spoken: no name is running either.
        arbiter_.nameEnded();
        return order;
    }
    order.utterance = list.first();
    return order;
}

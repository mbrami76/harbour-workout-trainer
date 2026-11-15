// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "engine.h"

#include <algorithm>
#include <functional>

SessionEngine::SessionEngine(const Clock *clock, const QList<PlanStep> &plan, const EngineConfig &config)
    : clock_(clock), plan_(plan)
{
    init(config);
}

SessionEngine::SessionEngine(const Clock *clock, const Workout &workout, const EngineConfig &config)
    : clock_(clock), plan_(expandPlan(workout))
{
    init(config);
}

SessionEngine::SessionEngine(const Clock *clock, const Workout &workout, const ExerciseTempos &tempos,
                             const EngineConfig &config)
    : clock_(clock), plan_(expandPlan(workout, tempos))
{
    init(config);
}

void SessionEngine::init(const EngineConfig &config)
{
    const int leadIn = qBound(Limits::minLeadInSec, config.leadInSec, Limits::maxLeadInSec);
    leadInMs_ = qint64(leadIn) * 1000;
    restResumeLeadInMs_ = qint64(qMax(config.restResumeLeadInSec, 0)) * 1000;
    announcements_ = config.announcements;
    spokenCues_ = config.spokenCues;

    for (int seconds : config.tickSeconds) {
        if (seconds >= 1 && !ticks_.contains(seconds))
            ticks_.append(seconds);
    }
    std::sort(ticks_.begin(), ticks_.end(), std::greater<int>());
}

bool SessionEngine::isLive() const
{
    return state_ == SessionState::LeadIn || state_ == SessionState::Running
        || state_ == SessionState::WaitingForReps || state_ == SessionState::Paused
        || state_ == SessionState::WaitingToStart || state_ == SessionState::WaitingForVoice;
}

bool SessionEngine::hasLeadIn(const PlanStep &step) const
{
    return step.type != StepType::Rest && leadInMs_ > 0;
}

bool SessionEngine::resumeHasLeadIn() const
{
    const PlanStep &step = plan_.at(index_);
    if (hasLeadIn(step))
        return true;
    // A rest step that is nearly over gets the lead-in too, so the next step is not a surprise.
    return step.type == StepType::Rest && leadInMs_ > 0 && resuming_
        && resumeState_ == SessionState::Running && pendingRemaining_ <= restResumeLeadInMs_;
}

EngineEvent SessionEngine::event(EventType type, qint64 at) const
{
    EngineEvent e;
    e.type = type;
    e.atMs = at;
    e.stepIndex = index_;
    return e;
}

qint64 SessionEngine::stepRemainingMs() const
{
    switch (state_) {
    case SessionState::Running:
        return qMax<qint64>(0, stepEnd_ - clock_->nowMs());
    case SessionState::LeadIn:
    case SessionState::Paused:
        return plan_.at(index_).plannedMs() > 0 ? pendingRemaining_ : 0;
    case SessionState::WaitingToStart:
    case SessionState::WaitingForVoice:
        return plan_.at(index_).plannedMs();
    default:
        return 0;
    }
}

qint64 SessionEngine::leadInRemainingMs() const
{
    if (state_ != SessionState::LeadIn)
        return 0;
    return qMax<qint64>(0, leadEnd_ - clock_->nowMs());
}

qint64 SessionEngine::currentStepActiveMs() const
{
    if (records_.isEmpty())
        return 0;
    qint64 ms = records_.last().activeMs;
    if (state_ == SessionState::Running || state_ == SessionState::WaitingForReps)
        ms += qMax<qint64>(0, clock_->nowMs() - segmentStart_);
    return ms;
}

qint64 SessionEngine::elapsedActiveMs() const
{
    qint64 ms = 0;
    for (int i = 0; i + 1 < records_.size(); ++i)
        ms += records_.at(i).activeMs;
    return ms + currentStepActiveMs();
}

void SessionEngine::accrue(qint64 ms)
{
    if (!records_.isEmpty() && ms > 0)
        records_.last().activeMs += ms;
}

void SessionEngine::emitTicks(qint64 endMs, qint64 baseMs, QSet<int> &done, bool inLeadIn, qint64 now,
                              QList<EngineEvent> &events)
{
    for (int seconds : ticks_) {
        const qint64 remaining = qint64(seconds) * 1000;
        if (remaining >= baseMs || done.contains(seconds))
            continue;
        const qint64 at = endMs - remaining;
        if (at > now)
            break;
        EngineEvent e = event(EventType::Tick, at);
        e.remainingSec = seconds;
        e.inLeadIn = inLeadIn;
        events.append(e);
        done.insert(seconds);
    }
}

// The cues of a period at the times of its halo cues (R-ALR-15 a): the same remaining times as the
// ticks, each reported once; one whose time is behind a resumed step is not reported.
void SessionEngine::emitPeriodCues(qint64 now, QList<EngineEvent> &events)
{
    if (!spokenCues_)
        return;
    const qint64 total = plan_.at(index_).plannedMs();
    for (const SpokenCues::Cue &c : SpokenCues::periodCues(total)) {
        const qint64 remaining = total - c.atMs;
        if (remaining >= tickBase_ || cuedStep_.contains(int(c.kind)))
            continue;
        const qint64 at = stepEnd_ - remaining;
        if (at > now)
            continue;
        EngineEvent e = event(EventType::Cue, at);
        e.cue = c.kind;
        events.append(e);
        cuedStep_.insert(int(c.kind));
    }
}

void SessionEngine::advance(qint64 now, QList<EngineEvent> &events)
{
    for (;;) {
        if (state_ == SessionState::LeadIn) {
            emitTicks(leadEnd_, leadTotal_, tickedLead_, true, now, events);
            if (leadEnd_ > now)
                return;
            afterLeadIn(leadEnd_, events);
        } else if (state_ == SessionState::Running) {
            if (plan_.at(index_).hasTempo)
                emitBeats(now, events);
            else {
                emitTicks(stepEnd_, tickBase_, tickedStep_, false, now, events);
                emitPeriodCues(now, events);
            }
            if (stepEnd_ > now)
                return;
            const qint64 at = stepEnd_;
            accrue(at - segmentStart_);
            moveTo(index_ + 1, at, events);
        } else {
            return;
        }
    }
}

void SessionEngine::selectStep(int index)
{
    index_ = index;
    // An announcement made in the rest before the step stays pending for that step; any other is void.
    if (voiceStep_ != index)
        voicePending_ = false;
    resuming_ = false;
    resumeState_ = SessionState::Running;
    tapGiven_ = false;
    pendingRemaining_ = plan_.at(index).plannedMs();
    StepRecord record;
    record.stepIndex = index;
    records_.append(record);
}

void SessionEngine::moveTo(int index, qint64 at, QList<EngineEvent> &events)
{
    if (index >= plan_.size()) {
        finish(at, events);
        return;
    }
    selectStep(index);
    beginPhase(at, events);
}

bool SessionEngine::announces(const PlanStep &step)
{
    return step.type != StepType::Rest && !step.exerciseKey.isEmpty();
}

// At the start of a rest: its sentence and the announcement of the step after it, so that it is heard
// before the step begins (R-ALR-6, R-ALR-11). At any other step: its own announcement, unless the rest
// before it made it.
void SessionEngine::announce(qint64 at, QList<EngineEvent> &events)
{
    if (!announcements_)
        return;
    const PlanStep &step = plan_.at(index_);
    if (step.type == StepType::Rest) {
        const int following = index_ + 1;
        const bool introduces = following < plan_.size() && announces(plan_.at(following));
        EngineEvent e = event(EventType::Announce, at);
        e.restSentence = true;
        e.announceIndex = introduces ? following : -1;
        events.append(e);
        voicePending_ = introduces;
        voiceStep_ = introduces ? following : -1;
        announcedStep_ = voiceStep_;
    } else if (announces(step) && announcedStep_ != index_) {
        EngineEvent e = event(EventType::Announce, at);
        e.announceIndex = index_;
        events.append(e);
        voicePending_ = true;
        voiceStep_ = index_;
        announcedStep_ = index_;
    }
}

void SessionEngine::beginPhase(qint64 at, QList<EngineEvent> &events)
{
    announce(at, events);
    if (waitsForTap()) {
        state_ = SessionState::WaitingToStart;
        events.append(event(EventType::WaitingForStart, at));
        return;
    }
    startStep(at, events);
}

// The first step of the plan never waits: the Start that opens the session is its trigger.
bool SessionEngine::waitsForTap() const
{
    return index_ != 0 && plan_.at(index_).startOnTap && !tapGiven_;
}

void SessionEngine::startStep(qint64 at, QList<EngineEvent> &events)
{
    tapGiven_ = true;
    if (hasLeadIn(plan_.at(index_)))
        startLeadIn(at);
    else
        enterStep(at, events);
}

void SessionEngine::emitBeats(qint64 now, QList<EngineEvent> &events)
{
    const Tempo &tempo = plan_.at(index_).tempo;
    while (nextRep_ < tempo.reps()) {
        const qint64 at = tempoStart_ + tempo.repStartMs(nextRep_) + tempo.beatOffsetMs(nextBeat_);
        if (at > now)
            break;
        EngineEvent beat = event(EventType::Beat, at);
        beat.repetition = nextRep_ + 1;
        beat.phase = tempo.beatPhase(nextBeat_);
        beat.accent = Tempo::isAccent(nextBeat_);
        events.append(beat);
        if (spokenCues_ && nextBeat_ == 0 && nextRep_ + 1 > cuedRep_) {
            cuedRep_ = nextRep_ + 1;
            for (const SpokenCues::Cue &c : SpokenCues::tempoCues(tempo)) {
                if (c.repetition != cuedRep_)
                    continue;
                EngineEvent cue = event(EventType::Cue, at);
                cue.cue = c.kind;
                cue.repetition = c.repetition;
                cue.cueCount = c.count;
                events.append(cue);
            }
        }
        if (++nextBeat_ >= tempo.beatsPerRep()) {
            nextBeat_ = 0;
            ++nextRep_;
        }
    }
}

void SessionEngine::startLeadIn(qint64 at)
{
    state_ = SessionState::LeadIn;
    leadTotal_ = leadInMs_;
    leadEnd_ = at + leadInMs_;
    tickedLead_.clear();
}

void SessionEngine::afterLeadIn(qint64 at, QList<EngineEvent> &events)
{
    if (!resuming_) {
        enterStep(at, events);
        return;
    }
    resuming_ = false;
    segmentStart_ = at;
    if (resumeState_ == SessionState::Running) {
        state_ = SessionState::Running;
        stepEnd_ = at + pendingRemaining_;
        tickBase_ = pendingRemaining_;
        if (plan_.at(index_).hasTempo) {
            // The repetition that was running starts again at its first beat.
            const Tempo &tempo = plan_.at(index_).tempo;
            tempoStart_ = at - tempo.repStartMs(pausedRep_);
            nextRep_ = pausedRep_;
            nextBeat_ = 0;
        }
    } else {
        state_ = SessionState::WaitingForReps;
    }
}

void SessionEngine::enterStep(qint64 at, QList<EngineEvent> &events)
{
    if (voicePending_ && voiceStep_ == index_) {
        state_ = SessionState::WaitingForVoice;
        return;
    }
    if (announcedStep_ == index_)
        announcedStep_ = -1;
    const PlanStep &step = plan_.at(index_);
    if (hasStarted_ && (step.blockIndex != lastBlock_ || step.roundIndex != lastRound_))
        events.append(event(EventType::RoundChanged, at));
    hasStarted_ = true;
    lastBlock_ = step.blockIndex;
    lastRound_ = step.roundIndex;
    events.append(event(EventType::StepStarted, at));

    segmentStart_ = at;
    if (step.isTimed() || step.hasTempo) {
        state_ = SessionState::Running;
        stepEnd_ = at + pendingRemaining_;
        tickBase_ = pendingRemaining_;
        tickedStep_.clear();
        cuedStep_.clear();
        cuedRep_ = 0;
        tempoStart_ = at;
        nextRep_ = 0;
        nextBeat_ = 0;
    } else {
        state_ = SessionState::WaitingForReps;
    }
}

void SessionEngine::finish(qint64 at, QList<EngineEvent> &events)
{
    state_ = SessionState::Finished;
    events.append(event(EventType::Finished, at));
}

QList<EngineEvent> SessionEngine::update()
{
    QList<EngineEvent> events;
    advance(clock_->nowMs(), events);
    return events;
}

QList<EngineEvent> SessionEngine::voiceFinished()
{
    QList<EngineEvent> events;
    if (!voicePending_)
        return events;
    voicePending_ = false;
    if (state_ == SessionState::WaitingForVoice) {
        const qint64 now = clock_->nowMs();
        enterStep(now, events);
        advance(now, events);
    }
    return events;
}

QList<EngineEvent> SessionEngine::start()
{
    QList<EngineEvent> events;
    if (state_ == SessionState::WaitingToStart) {
        const qint64 now = clock_->nowMs();
        startStep(now, events);
        advance(now, events);
        return events;
    }
    if (state_ != SessionState::Ready)
        return events;
    const qint64 now = clock_->nowMs();
    if (plan_.isEmpty()) {
        finish(now, events);
        return events;
    }
    moveTo(0, now, events);
    advance(now, events);
    return events;
}

QList<EngineEvent> SessionEngine::pause()
{
    QList<EngineEvent> events;
    const qint64 now = clock_->nowMs();
    advance(now, events);

    if (state_ == SessionState::Running) {
        pendingRemaining_ = stepEnd_ - now;
        if (plan_.at(index_).hasTempo) {
            // The repetition in progress is run again after the pause.
            const Tempo &tempo = plan_.at(index_).tempo;
            pausedRep_ = tempo.repIndexAt(now - tempoStart_);
            pendingRemaining_ = tempo.setDurationMs() - tempo.repStartMs(pausedRep_);
        }
        accrue(now - segmentStart_);
        resuming_ = true;
        resumeState_ = SessionState::Running;
    } else if (state_ == SessionState::WaitingToStart) {
        resumeState_ = SessionState::WaitingToStart;
    } else if (state_ == SessionState::WaitingForVoice) {
        resumeState_ = SessionState::WaitingForVoice;
    } else if (state_ == SessionState::WaitingForReps) {
        accrue(now - segmentStart_);
        resuming_ = true;
        resumeState_ = SessionState::WaitingForReps;
    } else if (state_ != SessionState::LeadIn) {
        return events;
    }
    state_ = SessionState::Paused;
    return events;
}

QList<EngineEvent> SessionEngine::resume()
{
    QList<EngineEvent> events;
    if (state_ != SessionState::Paused)
        return events;
    const qint64 now = clock_->nowMs();
    if (waitsForTap()) {
        // The step still waits for its tap, also when the user skipped to it while paused.
        state_ = SessionState::WaitingToStart;
        if (resumeState_ != SessionState::WaitingToStart)
            events.append(event(EventType::WaitingForStart, now));
        resumeState_ = SessionState::Running;
        return events;
    }
    if (resumeState_ == SessionState::WaitingForVoice) {
        // The announcement is spoken again from its start, then the step begins.
        resumeState_ = SessionState::Running;
        state_ = SessionState::WaitingForVoice;
        if (voicePending_) {
            EngineEvent again = event(EventType::Announce, now);
            again.announceIndex = voiceStep_;
            again.repeated = true;
            events.append(again);
        } else {
            enterStep(now, events);
            advance(now, events);
        }
        return events;
    }
    if (resumeHasLeadIn())
        startLeadIn(now);
    else
        afterLeadIn(now, events);
    advance(now, events);
    if (voicePending_) {
        EngineEvent again = event(EventType::Announce, now);
        again.announceIndex = voiceStep_;
        again.repeated = true;
        events.append(again);
    }
    return events;
}

void SessionEngine::jump(int index, QList<EngineEvent> &events)
{
    const qint64 now = clock_->nowMs();
    if (state_ == SessionState::Running || state_ == SessionState::WaitingForReps)
        accrue(now - segmentStart_);

    if (index >= plan_.size()) {
        finish(now, events);
    } else if (state_ == SessionState::Paused) {
        selectStep(index);
        announce(now, events);
    } else {
        moveTo(index, now, events);
        advance(now, events);
    }
}

QList<EngineEvent> SessionEngine::next()
{
    QList<EngineEvent> events;
    advance(clock_->nowMs(), events);
    if (isLive())
        jump(index_ + 1, events);
    return events;
}

QList<EngineEvent> SessionEngine::nextBlock()
{
    QList<EngineEvent> events;
    advance(clock_->nowMs(), events);
    if (!isLive())
        return events;
    // The first step of a later block; the end of the plan when the running block is the last one.
    int target = index_ + 1;
    const int block = plan_.at(index_).blockIndex;
    while (target < plan_.size() && plan_.at(target).blockIndex == block)
        ++target;
    jump(target, events);
    return events;
}

QList<EngineEvent> SessionEngine::done()
{
    QList<EngineEvent> events;
    advance(clock_->nowMs(), events);
    // A repetition step ends on the user's word; a tempo step ends by itself, or early the same way.
    if (state_ == SessionState::WaitingForReps || (state_ == SessionState::Running && plan_.at(index_).hasTempo))
        jump(index_ + 1, events);
    return events;
}

QList<EngineEvent> SessionEngine::stop()
{
    QList<EngineEvent> events;
    const qint64 now = clock_->nowMs();
    advance(now, events);
    if (!isLive())
        return events;
    if (state_ == SessionState::Running || state_ == SessionState::WaitingForReps)
        accrue(now - segmentStart_);
    state_ = SessionState::Stopped;
    events.append(event(EventType::Stopped, now));
    return events;
}

qint64 SessionEngine::tempoElapsedMs() const
{
    if (plan_.isEmpty() || !plan_.at(index_).hasTempo)
        return 0;
    const Tempo &tempo = plan_.at(index_).tempo;
    switch (state_) {
    case SessionState::Running:
        return qBound<qint64>(0, clock_->nowMs() - tempoStart_, tempo.setDurationMs());
    case SessionState::Paused:
    case SessionState::LeadIn:
        return resuming_ && resumeState_ == SessionState::Running ? tempo.repStartMs(pausedRep_) : 0;
    default:
        return 0;
    }
}

qint64 SessionEngine::plannedRemainingMs(bool *open) const
{
    bool hasOpen = false;
    qint64 total = 0;
    int first = index_;
    switch (state_) {
    case SessionState::Finished:
    case SessionState::Stopped:
        first = plan_.size();
        break;
    case SessionState::Running:
        total += qMax<qint64>(0, stepEnd_ - clock_->nowMs());
        ++first;
        break;
    case SessionState::Paused:
        if (plan_.at(index_).plannedMs() > 0)
            total += pendingRemaining_;
        else
            hasOpen = true;
        ++first;
        break;
    case SessionState::WaitingForReps:
        hasOpen = true;
        ++first;
        break;
    default:
        // Ready, lead-in and waiting to start: the whole step is still ahead.
        break;
    }
    for (int i = first; i < plan_.size(); ++i) {
        const PlanStep &step = plan_.at(i);
        if (step.plannedMs() > 0)
            total += step.plannedMs();
        else
            hasOpen = true;
    }
    if (open)
        *open = hasOpen;
    return total;
}

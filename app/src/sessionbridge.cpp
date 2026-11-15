// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "sessionbridge.h"
#include "sessionexercise.h"

#include "services.h"
#include "devlog.h"
#include "speechbridge.h"

namespace {

PlanStep emptyStep;

}

SessionBridge::SessionBridge(Services *services, QObject *parent)
    : PolledBridge(parent), services_(services)
{
    prepareTimer_.setInterval(100);
    connect(&prepareTimer_, &QTimer::timeout, this, &SessionBridge::checkPrepared);
    waitTimer_.setInterval(200);
    connect(&waitTimer_, &QTimer::timeout, this, &SessionBridge::pollWaitCue);
}

SessionBridge::~SessionBridge()
{
}

bool SessionBridge::inProgress() const
{
    return state_ != Idle && state_ != Finished;
}

const PlanStep &SessionBridge::currentPlanStep() const
{
    return engine_ ? engine_->currentStep() : emptyStep;
}

int SessionBridge::stepNumber() const { return engine_ ? engine_->stepIndex() + 1 : 0; }
int SessionBridge::stepCount() const { return engine_ ? engine_->stepCount() : 0; }
int SessionBridge::blockNumber() const { return engine_ ? currentPlanStep().blockIndex + 1 : 0; }
int SessionBridge::blockCount() const { return engine_ ? currentPlanStep().blockCount : 0; }
int SessionBridge::roundNumber() const { return engine_ ? currentPlanStep().roundIndex + 1 : 0; }
int SessionBridge::roundCount() const { return engine_ ? currentPlanStep().roundCount : 0; }

QString SessionBridge::phaseKind() const
{
    if (state_ == Preparing)
        return QStringLiteral("preparing");
    if (state_ == LeadIn)
        return QStringLiteral("leadin");
    if (state_ == WaitingToStart)
        return QStringLiteral("waiting");
    // A paused session shows the step it paused in.
    const PlanStep &step = currentPlanStep();
    if (step.hasTempo)
        return QStringLiteral("tempo");
    return step.isTimed() ? QStringLiteral("timed") : QStringLiteral("reps");
}

qint64 SessionBridge::stepRemainingMs() const
{
    return engine_ ? engine_->stepRemainingMs() : 0;
}

qint64 SessionBridge::stepElapsedMs() const
{
    return engine_ ? engine_->currentStepActiveMs() : 0;
}

qint64 SessionBridge::stepPlannedMs() const
{
    return engine_ ? currentPlanStep().plannedMs() : 0;
}

qint64 SessionBridge::leadInRemainingMs() const
{
    return engine_ ? engine_->leadInRemainingMs() : 0;
}

qint64 SessionBridge::workoutRemainingMs() const
{
    return engine_ ? engine_->plannedRemainingMs() : 0;
}

bool SessionBridge::workoutRemainingOpen() const
{
    bool open = false;
    if (engine_)
        engine_->plannedRemainingMs(&open);
    return open;
}

int SessionBridge::repetitionCount() const
{
    return engine_ && currentPlanStep().hasTempo ? currentPlanStep().tempo.reps() : 0;
}

int SessionBridge::phaseCount() const
{
    return engine_ && currentPlanStep().hasTempo ? currentPlanStep().tempo.phaseCount() : 0;
}

int SessionBridge::repetition() const
{
    if (!engine_ || !currentPlanStep().hasTempo)
        return 0;
    return currentPlanStep().tempo.repIndexAt(engine_->tempoElapsedMs()) + 1;
}

int SessionBridge::phase() const
{
    if (!engine_ || !currentPlanStep().hasTempo)
        return 0;
    const Tempo &tempo = currentPlanStep().tempo;
    const qint64 elapsed = engine_->tempoElapsedMs();
    const int rep = tempo.repIndexAt(elapsed);
    return tempo.phaseAt(elapsed - tempo.repStartMs(rep));
}

QString SessionBridge::phaseName() const
{
    return engine_ && currentPlanStep().hasTempo ? currentPlanStep().tempo.phaseName(phase()) : QString();
}

qreal SessionBridge::repetitionFraction() const
{
    if (!engine_ || !currentPlanStep().hasTempo)
        return 0;
    const Tempo &tempo = currentPlanStep().tempo;
    const qint64 elapsed = engine_->tempoElapsedMs();
    const int rep = tempo.repIndexAt(elapsed);
    const qint64 length = qMax<qint64>(1, tempo.repDurationMs(rep));
    return qBound<qreal>(0, qreal(elapsed - tempo.repStartMs(rep)) / qreal(length), 1);
}

qreal SessionBridge::prepareProgress() const
{
    return speechRun_ ? speechRun_->progress() : 1.0;
}

qint64 SessionBridge::clockMs() const
{
    return services_->clock()->nowMs();
}

QVariantMap SessionBridge::stepMap(int index) const
{
    QVariantMap map;
    if (!engine_ || index < 0 || index >= engine_->stepCount())
        return map;
    const PlanStep &step = engine_->plan().at(index);
    map.insert(QStringLiteral("type"), stepTypeName(step.type));
    map.insert(QStringLiteral("exerciseKey"), step.exerciseKey);
    const Exercise e = exercises_.value(step.exerciseKey);
    map.insert(QStringLiteral("exerciseName"), e.name);
    map.insert(QStringLiteral("icon"), e.icon);
    map.insert(QStringLiteral("colour"), e.colour);
    map.insert(QStringLiteral("ending"), step.isTimed() ? QStringLiteral("duration")
                                         : (step.hasTempo ? QStringLiteral("tempo") : QStringLiteral("reps")));
    map.insert(QStringLiteral("durationSec"), step.durationSec);
    map.insert(QStringLiteral("reps"), step.reps);
    // How the repetitions are paced: "rhythm", "total" (a total time), "per-rep" (a time per
    // repetition) or "none"; repMs is the time of one repetition of a paced step.
    map.insert(QStringLiteral("pace"), !step.hasTempo ? QStringLiteral("none")
               : (step.pace == PaceKind::None ? QStringLiteral("rhythm") : paceKindName(step.pace)));
    map.insert(QStringLiteral("repMs"), step.hasTempo ? step.tempo.repDurationMs(0) : qint64(0));
    map.insert(QStringLiteral("startOnTap"), step.startOnTap && index != 0);
    map.insert(QStringLiteral("insertedRest"), step.insertedRest);
    return map;
}

// The step-level properties change when the position, the plan or the state of the wait does.
void SessionBridge::refreshStep()
{
    if (!engine_)
        return;
    shownStep_ = engine_->stepIndex();
    shownPlanSize_ = engine_->stepCount();
    current_ = stepMap(shownStep_);
    next_ = stepMap(shownStep_ + 1);
    const QList<PlanStep> &plan = engine_->plan();
    exerciseFileKey_ = SessionExercise::fileKey(shownStep_ >= 0 && shownStep_ < plan.size() ? &plan.at(shownStep_) : nullptr,
                                                shownStep_ + 1 < plan.size() ? &plan.at(shownStep_ + 1) : nullptr);
    waitingToStart_ = engine_->state() == SessionState::WaitingToStart;
    phaseStarts_.clear();
    const PlanStep &step = currentPlanStep();
    if (step.hasTempo) {
        const qint64 repMs = qMax<qint64>(1, step.tempo.repDurationMs(0));
        for (qint64 offset : step.tempo.beatOffsetsMs())
            phaseStarts_.append(double(offset) / double(repMs));
    }
    emit stepChanged();
    emit controlsChanged();
    updatePhaseKind();
}

bool SessionBridge::start(int workoutId)
{
    if (state_ != Idle || !services_->isReady())
        return false;
    Workout workout;
    if (!services_->workouts()->get(workoutId, &workout) || !validate(workout).isEmpty())
        return false;

    EngineConfig config;
    config.leadInSec = leadInSec_;
    if (!tickSeconds_.isEmpty()) {
        config.tickSeconds.clear();
        for (const QVariant &v : tickSeconds_)
            config.tickSeconds.append(v.toInt());
    }
    workout_ = workout;
    exercises_.clear();
    for (const Exercise &e : services_->exercises()->list())
        exercises_.insert(e.key, e);
    engine_.reset(new SessionEngine(services_->clock(), workout, services_->workouts()->tempos(), config));
    if (engine_->stepCount() == 0) {
        engine_.reset();
        return false;
    }
    startedAtMs_ = QDateTime::currentMSecsSinceEpoch();
    logged_ = false;
    totalSec_ = 0;

    // The voice: the texts are prepared in the background and the session begins when the first
    // announcement is ready (R-ALR-10).
    speechRun_.reset();
    if (speech_) {
        QHash<QString, SpokenExercise> spoken;
        for (const Exercise &e : exercises_)
            spoken.insert(e.key, SpokenExercise{e.name, e.instructions, e.language});
        speechRun_.reset(speech_->beginRun(engine_->plan(), spoken, this));
    }
    engine_->setAnnouncements(!speechRun_.isNull());
    engine_->setSpokenCues(!speechRun_.isNull());
    if (speechRun_) {
        connect(speechRun_.data(), &SpeechRun::voiceFinished, this, &SessionBridge::voiceFinished);
        if (!speechRun_->ready()) {
            refreshStep();
            setState(Preparing);
            prepareTimer_.start();
            emit prepareProgressChanged();
            return true;
        }
    }
    begin();
    return true;
}

void SessionBridge::begin()
{
    handle(engine_->start());
    sync();
    emit timeChanged();
}

// Waits for the first announcement; a voice that cannot be used lets the session begin without it.
void SessionBridge::checkPrepared()
{
    if (state_ != Preparing || !speechRun_) {
        prepareTimer_.stop();
        return;
    }
    emit prepareProgressChanged();
    if (!speechRun_->ready())
        return;
    prepareTimer_.stop();
    if (speechRun_->engineFailed())
        speechRun_.reset();
    engine_->setAnnouncements(!speechRun_.isNull());
    engine_->setSpokenCues(!speechRun_.isNull());
    begin();
}

void SessionBridge::startWithoutVoice()
{
    if (state_ != Preparing)
        return;
    prepareTimer_.stop();
    speechRun_.reset();
    engine_->setAnnouncements(false);
    engine_->setSpokenCues(false);
    begin();
}

// The announcement was spoken to its end: the step that waits for it starts.
void SessionBridge::voiceFinished()
{
    if (!engine_ || state_ == Paused)
        return;
    handle(engine_->voiceFinished());
    sync();
    emit timeChanged();
}

void SessionBridge::pause()
{
    if (!engine_ || !pausable())
        return;
    if (speechRun_)
        speechRun_->stop();
    handle(engine_->pause());
    sync();
    emit paused();
    emit timeChanged();
}

void SessionBridge::resume()
{
    if (!engine_ || state_ != Paused)
        return;
    handle(engine_->resume());
    sync();
    emit resumed();
    emit timeChanged();
}

void SessionBridge::skip()
{
    if (!engine_ || !controls().skipHold)
        return;
    if (speechRun_)
        speechRun_->stop();
    handle(engine_->next());
    sync();
    emit timeChanged();
}

void SessionBridge::skipBlock()
{
    if (!engine_ || !controls().skipHold)
        return;
    if (speechRun_)
        speechRun_->stop();
    handle(engine_->nextBlock());
    sync();
    emit timeChanged();
}

void SessionBridge::stop()
{
    if (!engine_ || !inProgress())
        return;
    if (state_ == Preparing) {
        // Leaving before the session began: nothing is logged.
        prepareTimer_.stop();
        speechRun_.reset();
        engine_.reset();
        setState(Idle);
        emit stopped();
        return;
    }
    if (speechRun_)
        speechRun_->stop();
    handle(engine_->stop());
    sync();
    emit timeChanged();
}

void SessionBridge::startWaitingStep()
{
    if (!engine_ || state_ != WaitingToStart)
        return;
    handle(engine_->start());
    sync();
    emit timeChanged();
}

void SessionBridge::close()
{
    if (state_ != Finished)
        return;
    speechRun_.reset();
    engine_.reset();
    setState(Idle);
}

void SessionBridge::poll()
{
    if (!engine_ || !isRunning())
        return;
    handle(engine_->update());
    sync();
    emit timeChanged();
}

void SessionBridge::setState(State state)
{
    if (state_ == state)
        return;
    state_ = state;
    updatePolling();
    // The cue calls the user while a step waits for the tap, and stops with the wait: a tap, a
    // pause and a stop all leave the state.
    if (state_ == WaitingToStart) {
        waitCue_.begin(services_->clock()->nowMs());
        waitTimer_.start();
        pollWaitCue();
    } else if (waitCue_.active()) {
        waitCue_.end();
        waitTimer_.stop();
    }
    emit stateChanged();
    emit controlsChanged();
    updatePhaseKind();
}

// Tells the page when what it shows (the lead-in, a wait, a timed step, repetitions or a paced step)
// is another one, whichever of the state and the step changed it.
void SessionBridge::updatePhaseKind()
{
    const QString kind = phaseKind();
    if (kind == shownKind_)
        return;
    shownKind_ = kind;
    emit phaseKindChanged();
}

// The bridge state follows the engine. A stopped session is logged and leaves the bridge idle; a
// finished one is logged and stays until the page closes it.
void SessionBridge::sync()
{
    if (!engine_) {
        setState(Idle);
        return;
    }
    switch (engine_->state()) {
    case SessionState::LeadIn: setState(LeadIn); break;
    // The lead-in is over and the voice is not: shown as the lead-in at zero.
    case SessionState::WaitingForVoice: setState(LeadIn); break;
    case SessionState::Running: setState(Running); break;
    case SessionState::WaitingForReps: setState(WaitingForReps); break;
    case SessionState::WaitingToStart: setState(WaitingToStart); break;
    case SessionState::Paused: setState(Paused); break;
    case SessionState::Ready: break;
    case SessionState::Finished:
        writeLog(EndState::Completed);
        speechRun_.reset();
        setState(Finished);
        break;
    case SessionState::Stopped:
        writeLog(EndState::Stopped);
        speechRun_.reset();
        engine_.reset();
        setState(Idle);
        emit stopped();
        return;
    }
    if (engine_->stepIndex() != shownStep_ || engine_->stepCount() != shownPlanSize_
        || (engine_->state() == SessionState::WaitingToStart) != waitingToStart_)
        refreshStep();
}

// One record per completed or stopped session, with a row per executed step (R-HST-1, R-HST-2).
void SessionBridge::writeLog(EndState endState)
{
    if (logged_ || !engine_)
        return;
    logged_ = true;
    const QHash<QString, Exercise> &known = exercises_;
    const LogEntry entry = buildLogEntry(workout_.name, startedAtMs_, engine_->plan(), engine_->records(), endState,
        [&known](const QString &key, QString *name, MuscleSet *muscles) {
            const auto it = known.constFind(key);
            if (it == known.constEnd())
                return false;
            *name = it->name;
            *muscles = it->muscles;
            return true;
        });
    totalSec_ = entry.totalSec;
    if (isLoggable(entry))
        services_->sessionLog()->add(entry);
}

void SessionBridge::handle(const QList<EngineEvent> &events)
{
    bool nothingToSay = false;
    for (const EngineEvent &e : events) {
#ifdef FAKE_SPEECH
        // The development build with the fake engine shows the events of the session next to what is spoken.
        devLog(QStringLiteral("event %1 step %2").arg(int(e.type)).arg(e.stepIndex));
#endif
        switch (e.type) {
        case EventType::StepStarted:
            emit stepStarted(stepTypeName(engine_->plan().at(e.stepIndex).type));
            break;
        case EventType::Tick:
            if (e.inLeadIn)
                emit leadInTick(e.remainingSec);
            else
                emit lastSecondsTick(e.remainingSec);
            break;
        case EventType::RoundChanged:
            emit roundChanged();
            break;
        case EventType::WaitingForStart:
            break;
        case EventType::Beat: {
            const int count = engine_->plan().at(e.stepIndex).tempo.phaseCount();
            QString kind = QStringLiteral("middle");
            if (e.accent)
                kind = QStringLiteral("first");
            else if (e.phase >= count)
                kind = QStringLiteral("last");
            emit beat(kind, e.phase, e.repetition, e.atMs);
            // Every phase of a rhythm starts with a beat: its name is spoken from the same time.
            if (speechRun_ && e.phase >= 1)
                speechRun_->phaseStarted(e, engine_->plan());
            break;
        }
        case EventType::Cue:
            if (speechRun_)
                speechRun_->cue(e, engine_->plan());
            break;
        case EventType::Announce:
            // Nothing is spoken while the session is paused; the resume asks again.
            if (engine_->state() == SessionState::Paused)
                break;
            if (!speechRun_ || !speechRun_->announce(e, engine_->plan()))
                nothingToSay = true;
            break;
        case EventType::Finished:
            emit finished();
            break;
        case EventType::Stopped:
            break;
        }
        if (e.type == EventType::StepStarted && speechRun_)
            speechRun_->stepStarted(e.stepIndex);
    }
    // The step does not wait for an announcement that cannot be spoken.
    if (nothingToSay && engine_ && engine_->voicePending())
        handle(engine_->voiceFinished());
}

void SessionBridge::pollWaitCue()
{
    if (waitCue_.due(services_->clock()->nowMs()))
        emit waitCue();
}

SessionControls::Controls SessionBridge::controls() const
{
    if (state_ == Preparing)
        return SessionControls::whilePreparing();
    if (!engine_)
        return SessionControls::Controls();
    return SessionControls::forState(engine_->state(), &engine_->currentStep());
}

QString SessionBridge::playAction() const
{
    switch (controls().play) {
    case SessionControls::Play::Pause: return QStringLiteral("pause");
    case SessionControls::Play::Resume: return QStringLiteral("resume");
    case SessionControls::Play::Start: return QStringLiteral("start");
    case SessionControls::Play::None: break;
    }
    return QStringLiteral("none");
}

QString SessionBridge::skipKind() const
{
    switch (controls().skipTap) {
    case SessionControls::Skip::NextStep: return QStringLiteral("next");
    case SessionControls::Skip::Done: return QStringLiteral("done");
    case SessionControls::Skip::EndSet: return QStringLiteral("end-set");
    case SessionControls::Skip::None: break;
    }
    return QStringLiteral("none");
}

qint64 SessionBridge::workoutElapsedMs() const
{
    return engine_ ? engine_->elapsedActiveMs() : 0;
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "metronomebridge.h"

#include "metronomeform.h"
#include "services.h"
#include "workoutlimits.h"

namespace {

QString kindName(BeatKind kind)
{
    switch (kind) {
    case BeatKind::First: return QStringLiteral("first");
    case BeatKind::Middle: return QStringLiteral("middle");
    case BeatKind::Last: return QStringLiteral("last");
    }
    return QStringLiteral("first");
}

}

MetronomeBridge::MetronomeBridge(Services *services, QObject *parent)
    : PolledBridge(parent), services_(services)
{
}

QString MetronomeBridge::section() const
{
    if (!run_)
        return QStringLiteral("set");
    if (run_->leadInRemainingMs() > 0 || run_->state() == MetronomeState::LeadIn)
        return QStringLiteral("leadin");
    if (run_->restRemainingMs() > 0 || run_->state() == MetronomeState::Resting)
        return QStringLiteral("rest");
    return QStringLiteral("set");
}

int MetronomeBridge::set() const { return run_ ? run_->setNumber() : 0; }
int MetronomeBridge::setCount() const { return run_ ? run_->setCount() : 0; }
int MetronomeBridge::repetition() const { return run_ ? run_->repetitionNumber() : 0; }
int MetronomeBridge::repetitionCount() const { return run_ ? run_->repetitionCount() : 0; }
int MetronomeBridge::phase() const { return run_ ? run_->phaseNumber() : 0; }
qint64 MetronomeBridge::remainingMs() const { return run_ ? run_->remainingMs() : 0; }
qint64 MetronomeBridge::restRemainingMs() const { return run_ ? run_->restRemainingMs() : 0; }
qint64 MetronomeBridge::leadInRemainingMs() const { return run_ ? run_->leadInRemainingMs() : 0; }

int MetronomeBridge::phaseCount() const { return phaseCount_; }

QString MetronomeBridge::phaseName() const
{
    const int number = run_ ? run_->phaseNumber() : 0;
    return (number >= 1 && number <= phaseNames_.size()) ? phaseNames_.at(number - 1) : QString();
}

qreal MetronomeBridge::repetitionFraction() const { return run_ ? run_->repetitionFraction() : 0; }

int MetronomeBridge::nameLength(const QString &name) const { return Tempo::nameLength(name); }

bool MetronomeBridge::canAddSet() const
{
    return run_ && state_ != Idle && run_->setCount() < Limits::maxSets;
}

bool MetronomeBridge::canRemoveSet() const
{
    return run_ && state_ != Idle && run_->setsLeft() > 0;
}

qint64 MetronomeBridge::clockMs() const
{
    return services_->clock()->nowMs();
}

bool MetronomeBridge::build(const QVariantMap &c, MetronomeConfig *out, QString *problem) const
{
    const Tempo tempo = MetronomeForm::tempoFromMap(c);
    MetronomeConfig config;
    config.tempo = tempo;
    config.leadInSec = c.value(QStringLiteral("leadInSec"), Limits::defaultLeadInSec).toInt();
    const QVariantList ticks = c.value(QStringLiteral("tickSeconds")).toList();
    if (!ticks.isEmpty()) {
        config.tickSeconds.clear();
        for (const QVariant &v : ticks)
            config.tickSeconds.append(v.toInt());
    }
    config.sets = c.value(QStringLiteral("sets"), 1).toInt();
    config.restSec = c.value(QStringLiteral("restSec"), 0).toInt();
    if (out)
        *out = config;
    if (!tempo.isValid()) {
        *problem = MetronomeForm::tempoProblemCode(tempo.problem());
    } else if (config.sets < Limits::minSets || config.sets > Limits::maxSets) {
        *problem = QStringLiteral("sets");
    } else if (config.restSec < Limits::minRestSec || config.restSec > Limits::maxRestSec) {
        *problem = QStringLiteral("rest");
    } else if (config.leadInSec < Limits::minLeadInSec || config.leadInSec > Limits::maxLeadInSec) {
        *problem = QStringLiteral("lead-in");
    } else {
        problem->clear();
    }
    return problem->isEmpty() && Metronome::isValid(config);
}

QString MetronomeBridge::problem(const QVariantMap &config) const
{
    QString problem;
    build(config, nullptr, &problem);
    return problem;
}

bool MetronomeBridge::start(const QVariantMap &config)
{
    MetronomeConfig c;
    QString problem;
    if (state_ != Idle || !build(config, &c, &problem))
        return false;
    run_.reset(new Metronome(services_->clock(), c));
    phaseCount_ = c.tempo.phaseCount();
    phaseNames_ = c.tempo.phaseNameList();
    // The start of every phase as a share of a repetition; the first repetition stands for all
    // (the repetitions of the form with a total time differ by a millisecond at most).
    phaseStarts_.clear();
    const qint64 repMs = qMax<qint64>(1, c.tempo.repDurationMs(0));
    for (qint64 offset : c.tempo.beatOffsetsMs())
        phaseStarts_.append(double(offset) / double(repMs));
    restMs_ = qint64(c.restSec) * 1000;
    name_ = config.value(QStringLiteral("name")).toString();
    handle(run_->start());
    syncState();
    emit timeChanged();
    return true;
}

void MetronomeBridge::pause()
{
    if (!run_ || !isRunning())
        return;
    handle(run_->pause());
    syncState();
    emit timeChanged();
}

void MetronomeBridge::resume()
{
    if (!run_ || state_ != Paused)
        return;
    handle(run_->resume());
    syncState();
    emit timeChanged();
}

void MetronomeBridge::stop()
{
    if (!run_ || state_ == Idle)
        return;
    handle(run_->stop());
    run_.reset();
    name_.clear();
    setState(Idle);
    emit timeChanged();
}

bool MetronomeBridge::addSet()
{
    if (!run_)
        return false;
    const bool done = run_->addSet();
    emit timeChanged();
    return done;
}

bool MetronomeBridge::removeSet()
{
    if (!run_)
        return false;
    const bool done = run_->removeSet();
    // Removing the last set during a rest ends the run at once: the core reports it at the next call.
    handle(run_->update());
    syncState();
    emit timeChanged();
    return done;
}

void MetronomeBridge::poll()
{
    if (!run_ || !isRunning())
        return;
    handle(run_->update());
    syncState();
    emit timeChanged();
}

void MetronomeBridge::setState(State state)
{
    if (state_ == state)
        return;
    state_ = state;
    updatePolling();
    emit stateChanged();
}

// The bridge state follows the core; a finished or stopped run leaves the bridge idle.
void MetronomeBridge::syncState()
{
    if (!run_) {
        setState(Idle);
        return;
    }
    switch (run_->state()) {
    case MetronomeState::LeadIn: setState(LeadIn); break;
    case MetronomeState::Running: setState(Running); break;
    case MetronomeState::Resting: setState(Resting); break;
    case MetronomeState::Paused: setState(Paused); break;
    case MetronomeState::Ready: break;
    case MetronomeState::Finished:
    case MetronomeState::Stopped:
            run_.reset();
        name_.clear();
        setState(Idle);
        break;
    }
}

void MetronomeBridge::handle(const QList<MetronomeEvent> &events)
{
    for (const MetronomeEvent &e : events) {
        switch (e.type) {
        case MetronomeEventType::LeadInTick: emit leadInTick(e.remainingSec); break;
        case MetronomeEventType::SetStarted: emit setStarted(e.set); break;
        case MetronomeEventType::RepetitionStarted: emit repetitionStarted(e.set, e.repetition); break;
        case MetronomeEventType::Beat: emit beat(kindName(e.beatKind), e.phase, e.repetition, e.atMs); break;
        case MetronomeEventType::SetEnded: emit setEnded(e.set); break;
        case MetronomeEventType::RestStarted: emit restStarted(e.set); break;
        case MetronomeEventType::RestTick: emit restTick(e.remainingSec); break;
        case MetronomeEventType::Finished: emit finished(); break;
        case MetronomeEventType::Stopped: emit stopped(); break;
        }
    }
}

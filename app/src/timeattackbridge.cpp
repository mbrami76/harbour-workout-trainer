// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "timeattackbridge.h"

#include <QDateTime>

#include "services.h"
#include "workoutlimits.h"

namespace {

QVariantMap toMap(const ChallengeSummary &s)
{
    QVariantMap map;
    map.insert(QStringLiteral("id"), s.id);
    map.insert(QStringLiteral("name"), s.name);
    map.insert(QStringLiteral("count"), s.count);
    map.insert(QStringLiteral("bestMs"), s.bestMs);
    map.insert(QStringLiteral("lastMs"), s.lastMs);
    map.insert(QStringLiteral("averageMs"), s.averageMs);
    return map;
}

}

TimeAttackBridge::TimeAttackBridge(Services *services, QObject *parent)
    : PolledBridge(parent), services_(services), watch_(services->clock())
{
}

void TimeAttackBridge::refresh()
{
    challenges_.clear();
    if (services_->isReady()) {
        for (const ChallengeSummary &s : services_->timeAttack()->list())
            challenges_.append(toMap(s));
    }
    emit challengesChanged();
}

QString TimeAttackBridge::createChallenge(const QString &name)
{
    QList<ChallengeProblem> problems;
    const StoreStatus status = services_->timeAttack()->createChallenge(
        name, QDateTime::currentMSecsSinceEpoch() / 1000, nullptr, &problems);
    if (status == StoreStatus::Ok) {
        refresh();
        return QString();
    }
    if (problems.contains(ChallengeProblem::NameEmpty))
        return QStringLiteral("name-empty");
    if (problems.contains(ChallengeProblem::NameTooLong))
        return QStringLiteral("name-too-long");
    return QStringLiteral("failed");
}

bool TimeAttackBridge::loadChallenge(int id)
{
    ChallengeSummary summary;
    if (!services_->timeAttack()->summary(id, &summary)) {
        challenge_.clear();
        results_.clear();
        emit challengeChanged();
        return false;
    }
    challenge_ = toMap(summary);
    results_.clear();
    for (const ChallengeResult &r : services_->timeAttack()->results(id)) {
        QVariantMap map;
        map.insert(QStringLiteral("id"), r.id);
        map.insert(QStringLiteral("durationMs"), r.durationMs);
        map.insert(QStringLiteral("finishedAt"), r.finishedAt);
        results_.append(map);
    }
    emit challengeChanged();
    return true;
}

bool TimeAttackBridge::deleteChallenge(int id)
{
    const bool ok = services_->timeAttack()->removeChallenge(id) == StoreStatus::Ok;
    if (ok) {
        challenge_.clear();
        results_.clear();
        emit challengeChanged();
        refresh();
    }
    return ok;
}

bool TimeAttackBridge::deleteResult(int resultId)
{
    const bool ok = services_->timeAttack()->removeResult(resultId) == StoreStatus::Ok;
    if (ok && !challenge_.isEmpty())
        loadChallenge(challenge_.value(QStringLiteral("id")).toInt());
    return ok;
}

void TimeAttackBridge::start()
{
    if (watch_.start()) {
        setState(Running);
        emit started();
    }
    emit timeChanged();
}

void TimeAttackBridge::pause()
{
    if (watch_.pause())
        setState(Paused);
    emit timeChanged();
}

void TimeAttackBridge::resume()
{
    if (watch_.resume())
        setState(Running);
    emit timeChanged();
}

QVariantMap TimeAttackBridge::stop()
{
    QVariantMap result;
    result.insert(QStringLiteral("ok"), false);
    result.insert(QStringLiteral("durationMs"), qint64(0));
    result.insert(QStringLiteral("newBest"), false);
    if (state_ == Ready || challenge_.isEmpty())
        return result;

    const qint64 elapsed = watch_.stop();
    setState(Ready);
    emit timeChanged();
    emit finished();
    const qint64 ms = qBound<qint64>(Limits::minResultMs, elapsed, Limits::maxResultMs);
    bool newBest = false;
    const int id = challenge_.value(QStringLiteral("id")).toInt();
    const StoreStatus status = services_->timeAttack()->addResult(id, ms, QDateTime::currentMSecsSinceEpoch() / 1000, &newBest);
    result.insert(QStringLiteral("durationMs"), ms);
    result.insert(QStringLiteral("ok"), status == StoreStatus::Ok);
    result.insert(QStringLiteral("newBest"), status == StoreStatus::Ok && newBest);
    loadChallenge(id);
    return result;
}

void TimeAttackBridge::cancel()
{
    watch_.reset();
    setState(Ready);
    emit timeChanged();
}

void TimeAttackBridge::setState(State state)
{
    if (state_ == state)
        return;
    state_ = state;
    updatePolling();
    emit stateChanged();
}

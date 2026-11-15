// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef TIMEATTACKBRIDGE_H
#define TIMEATTACKBRIDGE_H

#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "polledbridge.h"
#include "stopwatch.h"

class Services;

// Time attack (R-MOD-4) for QML: the challenges and their results in the database, and the
// stopwatch of a run. Exposed as the context property "timeAttackBridge". All store calls run on the
// main thread. The stopwatch is polled only while it runs and the app is active (PolledBridge).
// A run that the app pauses stays paused; stopping a paused run stores the time reached when it was
// paused.
class TimeAttackBridge : public PolledBridge
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool running READ isRunning NOTIFY stateChanged)
    // Running or paused.
    Q_PROPERTY(bool inProgress READ inProgress NOTIFY stateChanged)
    Q_PROPERTY(qint64 elapsedMs READ elapsedMs NOTIFY timeChanged)
    // [{id, name, count, bestMs}], by name.
    Q_PROPERTY(QVariantList challenges READ challenges NOTIFY challengesChanged)
    // The loaded challenge: {id, name, count, bestMs, lastMs, averageMs}; empty when none.
    Q_PROPERTY(QVariantMap challenge READ challenge NOTIFY challengeChanged)
    // Its results, newest first: [{id, durationMs, finishedAt}], finishedAt in UTC seconds.
    Q_PROPERTY(QVariantList results READ results NOTIFY challengeChanged)

public:
    enum State { Ready, Running, Paused };
    Q_ENUM(State)

    explicit TimeAttackBridge(Services *services, QObject *parent = nullptr);

    State state() const { return state_; }
    bool isRunning() const { return state_ == Running; }
    bool inProgress() const { return state_ != Ready; }
    qint64 elapsedMs() const { return watch_.elapsedMs(); }
    QVariantList challenges() const { return challenges_; }
    QVariantMap challenge() const { return challenge_; }
    QVariantList results() const { return results_; }

    // Reads the list of challenges again.
    Q_INVOKABLE void refresh();
    // Creates a challenge. Returns "" when it was created, otherwise a code of the refusal:
    // "name-empty", "name-too-long" or "failed".
    Q_INVOKABLE QString createChallenge(const QString &name);
    // Loads one challenge and its results into the properties; false when it does not exist.
    Q_INVOKABLE bool loadChallenge(int id);
    Q_INVOKABLE bool deleteChallenge(int id);
    Q_INVOKABLE bool deleteResult(int resultId);

    // The run of the loaded challenge.
    Q_INVOKABLE void start();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    // Ends the run, stores the result under the loaded challenge with the date of now and returns
    // {ok, durationMs, newBest}. A run stopped at once is stored as 1 ms, the shortest result.
    Q_INVOKABLE QVariantMap stop();
    // Drops the run without storing anything.
    Q_INVOKABLE void cancel();

signals:
    void stateChanged();
    void timeChanged();
    void challengesChanged();
    void challengeChanged();
    // A run started, and a run ended with its result stored (R-ALR-1).
    void started();
    void finished();

protected:
    bool wantsPolling() const override { return state_ == Running; }
    void poll() override { emit timeChanged(); }

private:
    void setState(State state);

    Services *services_;
    Stopwatch watch_;
    State state_ = Ready;
    QVariantList challenges_;
    QVariantMap challenge_;
    QVariantList results_;
};

#endif

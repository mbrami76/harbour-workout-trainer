// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "timeattackstore.h"

#include "colours.h"
#include "identity.h"
#include "workoutlimits.h"

namespace {

ChallengeSummary readSummary(const QSqlQuery &q)
{
    ChallengeSummary s;
    s.id = q.value(0).toInt();
    s.name = q.value(1).toString();
    s.createdAt = q.value(2).toLongLong();
    s.count = q.value(3).toInt();
    s.icon = q.value(7).toString();
    s.colour = q.value(8).toString();
    if (s.count > 0) {
        s.bestMs = q.value(4).toLongLong();
        s.lastMs = q.value(5).toLongLong();
        const qint64 sum = q.value(6).toLongLong();
        s.averageMs = (2 * sum + s.count) / (2 * qint64(s.count));
    }
    return s;
}

// Newest result of a challenge: latest finish time, then latest id.
const char summarySql[] =
    "SELECT c.id, c.name, c.created_at,"
    " (SELECT COUNT(*) FROM result r WHERE r.challenge_id = c.id),"
    " (SELECT MIN(duration_ms) FROM result r WHERE r.challenge_id = c.id),"
    " (SELECT duration_ms FROM result r WHERE r.challenge_id = c.id ORDER BY finished_at DESC, id DESC LIMIT 1),"
    " (SELECT SUM(duration_ms) FROM result r WHERE r.challenge_id = c.id),"
    " c.icon, c.colour"
    " FROM challenge c";

}

QList<ChallengeProblem> TimeAttackStore::validateName(const QString &name)
{
    QList<ChallengeProblem> found;
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty())
        found.append(ChallengeProblem::NameEmpty);
    if (trimmed.length() > Limits::maxChallengeNameChars)
        found.append(ChallengeProblem::NameTooLong);
    return found;
}

bool TimeAttackStore::exists(int id)
{
    QSqlQuery q;
    return db_->run(QStringLiteral("SELECT 1 FROM challenge WHERE id = ?"), QVariantList() << id, &q) && q.next();
}

QList<ChallengeProblem> TimeAttackStore::validate(const Challenge &challenge)
{
    QList<ChallengeProblem> found = validateName(challenge.name);
    if (!Identity::isValidIconKey(challenge.icon))
        found.append(ChallengeProblem::IconInvalid);
    if (!challenge.colour.isEmpty() && !Colours::isKnown(challenge.colour))
        found.append(ChallengeProblem::ColourUnknown);
    return found;
}

QList<ChallengeProblem> TimeAttackStore::check(const Challenge &challenge, int excludeId, bool *failed)
{
    *failed = false;
    QList<ChallengeProblem> found = validate(challenge);
    if (!found.isEmpty())
        return found;
    const QString name = challenge.name.trimmed();
    if (excludeId > 0) {
        QSqlQuery q;
        if (!db_->run(QStringLiteral("SELECT name, icon, colour FROM challenge WHERE id = ?"),
                      QVariantList() << excludeId, &q)) {
            *failed = true;
            return found;
        }
        if (q.next() && Identity::isSame(name, challenge.icon, challenge.colour, q.value(0).toString(),
                                         q.value(1).toString(), q.value(2).toString()))
            return found;
    }
    bool taken = false;
    if (!Identity::isTaken(db_, Identity::Table::Challenge, name, challenge.icon, challenge.colour, excludeId,
                           &taken)) {
        *failed = true;
        return found;
    }
    if (taken)
        found.append(ChallengeProblem::IdentityTaken);
    return found;
}

StoreStatus TimeAttackStore::createChallenge(const QString &name, qint64 createdAt, int *id,
                                             QList<ChallengeProblem> *problems)
{
    Challenge challenge;
    challenge.name = name;
    challenge.createdAt = createdAt;
    return createChallenge(challenge, id, problems);
}

StoreStatus TimeAttackStore::createChallenge(const Challenge &challenge, int *id,
                                             QList<ChallengeProblem> *problems)
{
    bool failed = false;
    const QList<ChallengeProblem> found = check(challenge, 0, &failed);
    if (problems)
        *problems = found;
    if (failed)
        return StoreStatus::Failed;
    if (!found.isEmpty())
        return StoreStatus::Invalid;
    Transaction transaction(db_);
    if (!transaction.isActive())
        return StoreStatus::Failed;
    if (!db_->run(QStringLiteral("INSERT INTO challenge (name, created_at, icon, colour) VALUES (?, ?, ?, ?)"),
                  QVariantList() << textValue(challenge.name.trimmed()) << challenge.createdAt
                                 << textValue(challenge.icon) << textValue(challenge.colour)))
        return StoreStatus::Failed;
    const int newId = int(db_->lastInsertId());
    if (!transaction.commit())
        return StoreStatus::Failed;
    if (id)
        *id = newId;
    return StoreStatus::Ok;
}

StoreStatus TimeAttackStore::renameChallenge(int id, const QString &name, QList<ChallengeProblem> *problems)
{
    Challenge challenge;
    if (!get(id, &challenge)) {
        const QList<ChallengeProblem> found = validateName(name);
        if (problems)
            *problems = found;
        return found.isEmpty() ? StoreStatus::NotFound : StoreStatus::Invalid;
    }
    challenge.name = name;
    return updateChallenge(id, challenge, problems);
}

StoreStatus TimeAttackStore::updateChallenge(int id, const Challenge &challenge, QList<ChallengeProblem> *problems)
{
    bool failed = false;
    const QList<ChallengeProblem> found = check(challenge, id, &failed);
    if (problems)
        *problems = found;
    if (failed)
        return StoreStatus::Failed;
    if (!found.isEmpty())
        return StoreStatus::Invalid;
    if (!exists(id))
        return StoreStatus::NotFound;
    Transaction transaction(db_);
    if (!transaction.isActive())
        return StoreStatus::Failed;
    if (!db_->run(QStringLiteral("UPDATE challenge SET name = ?, icon = ?, colour = ? WHERE id = ?"),
                  QVariantList() << textValue(challenge.name.trimmed()) << textValue(challenge.icon)
                                 << textValue(challenge.colour) << id)
        || !transaction.commit())
        return StoreStatus::Failed;
    return StoreStatus::Ok;
}

StoreStatus TimeAttackStore::removeChallenge(int id)
{
    if (!exists(id))
        return StoreStatus::NotFound;
    Transaction transaction(db_);
    if (!transaction.isActive())
        return StoreStatus::Failed;
    if (!db_->run(QStringLiteral("DELETE FROM challenge WHERE id = ?"), QVariantList() << id)
        || !transaction.commit())
        return StoreStatus::Failed;
    return StoreStatus::Ok;
}

bool TimeAttackStore::get(int id, Challenge *challenge)
{
    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT id, name, created_at, icon, colour FROM challenge WHERE id = ?"), QVariantList() << id, &q)
        || !q.next())
        return false;
    challenge->id = q.value(0).toInt();
    challenge->name = q.value(1).toString();
    challenge->createdAt = q.value(2).toLongLong();
    challenge->icon = q.value(3).toString();
    challenge->colour = q.value(4).toString();
    return true;
}

StoreStatus TimeAttackStore::addResult(int challengeId, qint64 durationMs, qint64 finishedAt, bool *newBest,
                                       QList<ChallengeProblem> *problems)
{
    QList<ChallengeProblem> found;
    if (durationMs < Limits::minResultMs || durationMs > Limits::maxResultMs)
        found.append(ChallengeProblem::DurationOutOfRange);
    if (problems)
        *problems = found;
    if (!found.isEmpty())
        return StoreStatus::Invalid;
    if (!exists(challengeId))
        return StoreStatus::NotFound;

    Transaction transaction(db_);
    if (!transaction.isActive())
        return StoreStatus::Failed;
    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT MIN(duration_ms) FROM result WHERE challenge_id = ?"),
                  QVariantList() << challengeId, &q)
        || !q.next())
        return StoreStatus::Failed;
    const bool best = q.value(0).isNull() || durationMs < q.value(0).toLongLong();
    if (!db_->run(QStringLiteral("INSERT INTO result (challenge_id, duration_ms, finished_at) VALUES (?, ?, ?)"),
                  QVariantList() << challengeId << durationMs << finishedAt)
        || !transaction.commit())
        return StoreStatus::Failed;
    if (newBest)
        *newBest = best;
    return StoreStatus::Ok;
}

StoreStatus TimeAttackStore::removeResult(int resultId)
{
    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT 1 FROM result WHERE id = ?"), QVariantList() << resultId, &q))
        return StoreStatus::Failed;
    if (!q.next())
        return StoreStatus::NotFound;
    Transaction transaction(db_);
    if (!transaction.isActive())
        return StoreStatus::Failed;
    if (!db_->run(QStringLiteral("DELETE FROM result WHERE id = ?"), QVariantList() << resultId)
        || !transaction.commit())
        return StoreStatus::Failed;
    return StoreStatus::Ok;
}

QList<ChallengeSummary> TimeAttackStore::list()
{
    QList<ChallengeSummary> all;
    QSqlQuery q;
    if (!db_->run(QString::fromLatin1(summarySql) + QStringLiteral(" ORDER BY c.name COLLATE NOCASE, c.id"),
                  QVariantList(), &q))
        return all;
    while (q.next())
        all.append(readSummary(q));
    return all;
}

bool TimeAttackStore::summary(int id, ChallengeSummary *out)
{
    QSqlQuery q;
    if (!db_->run(QString::fromLatin1(summarySql) + QStringLiteral(" WHERE c.id = ?"), QVariantList() << id, &q)
        || !q.next())
        return false;
    *out = readSummary(q);
    return true;
}

QList<ChallengeResult> TimeAttackStore::results(int challengeId)
{
    QList<ChallengeResult> all;
    QSqlQuery q;
    if (!db_->run(QStringLiteral("SELECT id, duration_ms, finished_at FROM result WHERE challenge_id = ? "
                                 "ORDER BY finished_at DESC, id DESC"),
                  QVariantList() << challengeId, &q))
        return all;
    while (q.next()) {
        ChallengeResult r;
        r.id = q.value(0).toInt();
        r.durationMs = q.value(1).toLongLong();
        r.finishedAt = q.value(2).toLongLong();
        all.append(r);
    }
    return all;
}

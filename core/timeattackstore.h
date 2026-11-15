// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef TIMEATTACKSTORE_H
#define TIMEATTACKSTORE_H

#include "database.h"

enum class ChallengeProblem {
    NameEmpty,
    NameTooLong,
    DurationOutOfRange,
    IconInvalid,    // the icon key has other characters than a-z, 0-9 and hyphens, or is too long
    ColourUnknown,  // the colour key is not in the palette
    IdentityTaken   // another challenge has the same name, icon and colour (R-MOD-4)
};

struct Challenge {
    int id = 0;
    QString name;
    // UTC seconds.
    qint64 createdAt = 0;
    // Optional keys; empty means the default. Unknown keys from a newer version are kept.
    QString icon;
    QString colour;
};

// A challenge with its figures. The figures are 0 when the challenge has no result.
struct ChallengeSummary {
    int id = 0;
    QString name;
    QString icon;
    QString colour;
    qint64 createdAt = 0;
    int count = 0;
    qint64 bestMs = 0;
    // Duration of the newest result.
    qint64 lastMs = 0;
    // Mean duration rounded to the nearest millisecond.
    qint64 averageMs = 0;
};

struct ChallengeResult {
    int id = 0;
    qint64 durationMs = 0;
    // UTC seconds.
    qint64 finishedAt = 0;
};

// Named time-attack challenges and their results (R-MOD-4). Two challenges may share a name only if
// their icon key or colour key differs; a create or an edit that would make them equal is Invalid
// with ChallengeProblem::IdentityTaken and writes nothing. An edit that leaves name, icon and
// colour as stored is not compared. Deleting a challenge deletes its results.
class TimeAttackStore
{
public:
    explicit TimeAttackStore(Database *database) : db_(database) {}

    static QList<ChallengeProblem> validateName(const QString &name);
    // Name, icon and colour rules; the identity is checked on write.
    static QList<ChallengeProblem> validate(const Challenge &challenge);

    // The name is trimmed. Sets the id of the new challenge. The first form has the default icon
    // and colour; of the second only name, icon, colour and createdAt are used.
    StoreStatus createChallenge(const QString &name, qint64 createdAt, int *id = nullptr,
                                QList<ChallengeProblem> *problems = nullptr);
    StoreStatus createChallenge(const Challenge &challenge, int *id = nullptr,
                                QList<ChallengeProblem> *problems = nullptr);
    // Changes the name; icon and colour stay.
    StoreStatus renameChallenge(int id, const QString &name, QList<ChallengeProblem> *problems = nullptr);
    // Replaces name, icon and colour.
    StoreStatus updateChallenge(int id, const Challenge &challenge, QList<ChallengeProblem> *problems = nullptr);
    StoreStatus removeChallenge(int id);
    bool get(int id, Challenge *challenge);

    // Stores a result. *newBest is true when no result of the challenge was as fast: the first
    // result is a new best, a time equal to the best is not.
    StoreStatus addResult(int challengeId, qint64 durationMs, qint64 finishedAt, bool *newBest = nullptr,
                          QList<ChallengeProblem> *problems = nullptr);
    StoreStatus removeResult(int resultId);

    // Sorted by name without regard to case, then by id.
    QList<ChallengeSummary> list();
    bool summary(int id, ChallengeSummary *summary);
    // Newest first.
    QList<ChallengeResult> results(int challengeId);

private:
    bool exists(int id);
    // excludeId: the challenge being edited, or 0. Sets *failed when the database reports an error.
    QList<ChallengeProblem> check(const Challenge &challenge, int excludeId, bool *failed);
    Database *db_;
};

#endif

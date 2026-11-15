// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "testdb.h"
#include "timeattackstore.h"
#include "workoutlimits.h"

class TstTimeAttackStore : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void emptyDatabase();
    void createGetAndTrim();
    void nameLimits();
    void namesMayRepeatWithAnotherColour();
    void rename();
    void renameInvalidChangesNothing();
    void removeChallengeDeletesItsResults();
    void unknownIdsAreNotFound();
    void firstResultIsANewBest();
    void newBestLogic();
    void equalTimeIsNotANewBest();
    void bestIsPerChallenge();
    void deletingTheBestMakesTheNextOneBest();
    void durationLimits();
    void summaryFigures();
    void averageIsRoundedToMilliseconds();
    void lastIsTheNewestByFinishTime();
    void resultsAreNewestFirst();
    void removeResult();
    void listIsSortedByName();
    void foreignKeysAreEnforced();
    void crashLeavesNoPartialWrite();
    void crashAfterCommitKeepsData();
    void nestedRollbackUndoesAResult();


    void iconAndColourRoundTrip();
    void defaultsAreEmptyKeys();
    void unknownStoredKeysAreKept();
    void invalidIconAndColourAreRefusedOnWrite();
    void equalIdentityIsRefused();
    void sameNameWithAnotherColourOrIconIsAccepted();
    void caseAndBlankSpaceVariantsAreRefused();
    void emptyKeysEqualEachOther();
    void editingWithoutChangingTheIdentityIsAccepted();
    void changingIntoAClashIsRefused();
    void renameKeepsIconAndColour();
    void refusedSaveWritesNothing();
private:
    int make(const QString &name, qint64 at = 100);
    TestDb t_;
};

void TstTimeAttackStore::init()
{
    QVERIFY(t_.dir.isValid());
    t_.db.close();
    QDir dir(t_.dir.path());
    dir.removeRecursively();
    QVERIFY(t_.open());
}

int TstTimeAttackStore::make(const QString &name, qint64 at)
{
    TimeAttackStore store(&t_.db);
    int id = 0;
    if (store.createChallenge(name, at, &id) != StoreStatus::Ok)
        return 0;
    return id;
}

void TstTimeAttackStore::emptyDatabase()
{
    TimeAttackStore store(&t_.db);
    QVERIFY(store.list().isEmpty());
    QVERIFY(store.results(1).isEmpty());
    ChallengeSummary s;
    QVERIFY(!store.summary(1, &s));
    Challenge c;
    QVERIFY(!store.get(1, &c));
}

void TstTimeAttackStore::createGetAndTrim()
{
    TimeAttackStore store(&t_.db);
    int id = 0;
    QVERIFY(store.createChallenge("  Stairs  ", 1700000000, &id) == StoreStatus::Ok);
    QVERIFY(id > 0);
    Challenge c;
    QVERIFY(store.get(id, &c));
    QCOMPARE(c.name, QStringLiteral("Stairs"));
    QCOMPARE(c.createdAt, qint64(1700000000));
    ChallengeSummary s;
    QVERIFY(store.summary(id, &s));
    QCOMPARE(s.count, 0);
    QCOMPARE(s.bestMs, qint64(0));
    QCOMPARE(s.lastMs, qint64(0));
    QCOMPARE(s.averageMs, qint64(0));
}

void TstTimeAttackStore::nameLimits()
{
    TimeAttackStore store(&t_.db);
    QList<ChallengeProblem> problems;
    QVERIFY(store.createChallenge("", 1, nullptr, &problems) == StoreStatus::Invalid);
    QVERIFY(problems.contains(ChallengeProblem::NameEmpty));
    QVERIFY(store.createChallenge(" \t ", 1, nullptr, &problems) == StoreStatus::Invalid);
    QVERIFY(store.createChallenge(QString(), 1, nullptr, &problems) == StoreStatus::Invalid);
    QVERIFY(store.createChallenge(QString(61, 'a'), 1, nullptr, &problems) == StoreStatus::Invalid);
    QVERIFY(problems.contains(ChallengeProblem::NameTooLong));
    QCOMPARE(countRows(t_.db, "challenge"), 0);
    QVERIFY(store.createChallenge(QString(60, 'a'), 1) == StoreStatus::Ok);
    // The limit counts UTF-16 units, after trimming.
    QVERIFY(store.createChallenge(" " + QString(60, 'b') + " ", 1) == StoreStatus::Ok);
    QVERIFY(store.createChallenge(QString(30, QChar(0x00fc)) + QString(31, QChar(0x00fc)), 1) == StoreStatus::Invalid);
    QCOMPARE(countRows(t_.db, "challenge"), 2);
}

void TstTimeAttackStore::namesMayRepeatWithAnotherColour()
{
    TimeAttackStore store(&t_.db);
    int a = 0, b = 0;
    QVERIFY(store.createChallenge("Same", 1, &a) == StoreStatus::Ok);
    Challenge other;
    other.name = QStringLiteral("Same");
    other.createdAt = 2;
    other.colour = QStringLiteral("blue");
    QVERIFY(store.createChallenge(other, &b) == StoreStatus::Ok);
    QVERIFY(a != b);
    QCOMPARE(store.list().size(), 2);
    QCOMPARE(store.list().at(0).name, QStringLiteral("Same"));
    QCOMPARE(store.list().at(1).name, QStringLiteral("Same"));
}

void TstTimeAttackStore::rename()
{
    TimeAttackStore store(&t_.db);
    const int id = make("Old");
    QVERIFY(store.renameChallenge(id, "  New name ") == StoreStatus::Ok);
    Challenge c;
    QVERIFY(store.get(id, &c));
    QCOMPARE(c.name, QStringLiteral("New name"));
    QCOMPARE(c.createdAt, qint64(100));
    // The results stay.
    QVERIFY(store.addResult(id, 5000, 1) == StoreStatus::Ok);
    QVERIFY(store.renameChallenge(id, "Newer") == StoreStatus::Ok);
    QCOMPARE(store.results(id).size(), 1);
}

void TstTimeAttackStore::renameInvalidChangesNothing()
{
    TimeAttackStore store(&t_.db);
    const int id = make("Keep");
    QList<ChallengeProblem> problems;
    QVERIFY(store.renameChallenge(id, "  ", &problems) == StoreStatus::Invalid);
    QVERIFY(store.renameChallenge(id, QString(61, 'x'), &problems) == StoreStatus::Invalid);
    QVERIFY(problems.contains(ChallengeProblem::NameTooLong));
    Challenge c;
    QVERIFY(store.get(id, &c));
    QCOMPARE(c.name, QStringLiteral("Keep"));
}

void TstTimeAttackStore::removeChallengeDeletesItsResults()
{
    TimeAttackStore store(&t_.db);
    const int a = make("A");
    const int b = make("B");
    QVERIFY(store.addResult(a, 1000, 1) == StoreStatus::Ok);
    QVERIFY(store.addResult(a, 2000, 2) == StoreStatus::Ok);
    QVERIFY(store.addResult(b, 3000, 3) == StoreStatus::Ok);
    QVERIFY(store.removeChallenge(a) == StoreStatus::Ok);
    QCOMPARE(countRows(t_.db, "challenge"), 1);
    QCOMPARE(countRows(t_.db, "result"), 1);
    QVERIFY(store.results(a).isEmpty());
    QCOMPARE(store.results(b).size(), 1);
    QVERIFY(integrityOk(t_.db));
}

void TstTimeAttackStore::unknownIdsAreNotFound()
{
    TimeAttackStore store(&t_.db);
    QVERIFY(store.renameChallenge(7, "X") == StoreStatus::NotFound);
    QVERIFY(store.removeChallenge(7) == StoreStatus::NotFound);
    QVERIFY(store.addResult(7, 1000, 1) == StoreStatus::NotFound);
    QVERIFY(store.removeResult(7) == StoreStatus::NotFound);
    QCOMPARE(countRows(t_.db, "result"), 0);
}

void TstTimeAttackStore::firstResultIsANewBest()
{
    TimeAttackStore store(&t_.db);
    const int id = make("C");
    bool best = false;
    QVERIFY(store.addResult(id, 86400000, 1, &best) == StoreStatus::Ok);
    QVERIFY(best);
}

void TstTimeAttackStore::newBestLogic()
{
    TimeAttackStore store(&t_.db);
    const int id = make("C");
    bool best = false;
    QVERIFY(store.addResult(id, 9000, 1, &best) == StoreStatus::Ok);
    QVERIFY(best);
    QVERIFY(store.addResult(id, 9500, 2, &best) == StoreStatus::Ok);
    QVERIFY(!best);
    QVERIFY(store.addResult(id, 8999, 3, &best) == StoreStatus::Ok);
    QVERIFY(best);
    QVERIFY(store.addResult(id, 12000, 4, &best) == StoreStatus::Ok);
    QVERIFY(!best);
    QVERIFY(store.addResult(id, 1, 5, &best) == StoreStatus::Ok);
    QVERIFY(best);
    ChallengeSummary s;
    QVERIFY(store.summary(id, &s));
    QCOMPARE(s.bestMs, qint64(1));
    QCOMPARE(s.count, 5);
}

void TstTimeAttackStore::equalTimeIsNotANewBest()
{
    TimeAttackStore store(&t_.db);
    const int id = make("C");
    bool best = false;
    QVERIFY(store.addResult(id, 7000, 1, &best) == StoreStatus::Ok);
    QVERIFY(best);
    QVERIFY(store.addResult(id, 7000, 2, &best) == StoreStatus::Ok);
    QVERIFY(!best);
    QCOMPARE(store.results(id).size(), 2);
}

void TstTimeAttackStore::bestIsPerChallenge()
{
    TimeAttackStore store(&t_.db);
    const int a = make("A");
    const int b = make("B");
    bool best = false;
    QVERIFY(store.addResult(a, 1000, 1, &best) == StoreStatus::Ok);
    QVERIFY(best);
    QVERIFY(store.addResult(b, 9000, 2, &best) == StoreStatus::Ok);
    QVERIFY(best);
}

void TstTimeAttackStore::deletingTheBestMakesTheNextOneBest()
{
    TimeAttackStore store(&t_.db);
    const int id = make("C");
    QVERIFY(store.addResult(id, 5000, 1) == StoreStatus::Ok);
    QVERIFY(store.addResult(id, 4000, 2) == StoreStatus::Ok);
    const QList<ChallengeResult> list = store.results(id);
    QCOMPARE(list.first().durationMs, qint64(4000));
    QVERIFY(store.removeResult(list.first().id) == StoreStatus::Ok);
    ChallengeSummary s;
    QVERIFY(store.summary(id, &s));
    QCOMPARE(s.bestMs, qint64(5000));
    bool best = false;
    QVERIFY(store.addResult(id, 4500, 3, &best) == StoreStatus::Ok);
    QVERIFY(best);
}

void TstTimeAttackStore::durationLimits()
{
    TimeAttackStore store(&t_.db);
    const int id = make("C");
    QList<ChallengeProblem> problems;
    bool best = true;
    const qint64 bad[] = { 0, -1, 86400001, qint64(1) << 40 };
    for (qint64 ms : bad) {
        QVERIFY2(store.addResult(id, ms, 1, &best, &problems) == StoreStatus::Invalid, qPrintable(QString::number(ms)));
        QVERIFY(problems.contains(ChallengeProblem::DurationOutOfRange));
    }
    QCOMPARE(countRows(t_.db, "result"), 0);
    QVERIFY(store.addResult(id, 1, 1) == StoreStatus::Ok);
    QVERIFY(store.addResult(id, 86400000, 2) == StoreStatus::Ok);
    QCOMPARE(countRows(t_.db, "result"), 2);
}

void TstTimeAttackStore::summaryFigures()
{
    TimeAttackStore store(&t_.db);
    const int id = make("C");
    QVERIFY(store.addResult(id, 12000, 10) == StoreStatus::Ok);
    QVERIFY(store.addResult(id, 9000, 20) == StoreStatus::Ok);
    QVERIFY(store.addResult(id, 15000, 30) == StoreStatus::Ok);
    ChallengeSummary s;
    QVERIFY(store.summary(id, &s));
    QCOMPARE(s.count, 3);
    QCOMPARE(s.bestMs, qint64(9000));
    QCOMPARE(s.lastMs, qint64(15000));
    QCOMPARE(s.averageMs, qint64(12000));
    const QList<ChallengeSummary> all = store.list();
    QCOMPARE(all.size(), 1);
    QCOMPARE(all.first().count, 3);
    QCOMPARE(all.first().bestMs, qint64(9000));
    QCOMPARE(all.first().lastMs, qint64(15000));
    QCOMPARE(all.first().averageMs, qint64(12000));
}

void TstTimeAttackStore::averageIsRoundedToMilliseconds()
{
    TimeAttackStore store(&t_.db);
    struct Case { QList<qint64> times; qint64 average; };
    const Case cases[] = {
        { QList<qint64>() << 1000 << 1001, 1001 },          // 1000.5 rounds up
        { QList<qint64>() << 1000 << 1000 << 1001, 1000 },  // 1000.33 rounds down
        { QList<qint64>() << 1000 << 1001 << 1001, 1001 },  // 1000.67 rounds up
        { QList<qint64>() << 5, 5 },
        { QList<qint64>() << 1 << 2 << 2, 2 }
    };
    for (const Case &c : cases) {
        const int id = make(QStringLiteral("Avg %1").arg(&c - cases));
        qint64 at = 1;
        for (qint64 ms : c.times)
            QVERIFY(store.addResult(id, ms, at++) == StoreStatus::Ok);
        ChallengeSummary s;
        QVERIFY(store.summary(id, &s));
        QCOMPARE(s.averageMs, c.average);
    }
    // A large sum does not overflow.
    const int id = make("Big");
    for (int i = 0; i < 200; ++i)
        QVERIFY(store.addResult(id, 86400000, i) == StoreStatus::Ok);
    ChallengeSummary s;
    QVERIFY(store.summary(id, &s));
    QCOMPARE(s.averageMs, qint64(86400000));
}

void TstTimeAttackStore::lastIsTheNewestByFinishTime()
{
    TimeAttackStore store(&t_.db);
    const int id = make("C");
    // Added out of order: the newest by finish time is "last", not the last one added.
    QVERIFY(store.addResult(id, 3000, 500) == StoreStatus::Ok);
    QVERIFY(store.addResult(id, 1000, 100) == StoreStatus::Ok);
    ChallengeSummary s;
    QVERIFY(store.summary(id, &s));
    QCOMPARE(s.lastMs, qint64(3000));
    // The same finish time: the later insert wins.
    QVERIFY(store.addResult(id, 2000, 500) == StoreStatus::Ok);
    QVERIFY(store.summary(id, &s));
    QCOMPARE(s.lastMs, qint64(2000));
}

void TstTimeAttackStore::resultsAreNewestFirst()
{
    TimeAttackStore store(&t_.db);
    const int id = make("C");
    QVERIFY(store.addResult(id, 1000, 10) == StoreStatus::Ok);
    QVERIFY(store.addResult(id, 2000, 30) == StoreStatus::Ok);
    QVERIFY(store.addResult(id, 3000, 20) == StoreStatus::Ok);
    QVERIFY(store.addResult(id, 4000, 30) == StoreStatus::Ok);
    const QList<ChallengeResult> list = store.results(id);
    QCOMPARE(list.size(), 4);
    QCOMPARE(list.at(0).durationMs, qint64(4000));
    QCOMPARE(list.at(1).durationMs, qint64(2000));
    QCOMPARE(list.at(2).durationMs, qint64(3000));
    QCOMPARE(list.at(3).durationMs, qint64(1000));
    QCOMPARE(list.at(0).finishedAt, qint64(30));
    // Only the results of the asked challenge.
    const int other = make("D");
    QVERIFY(store.addResult(other, 77, 1) == StoreStatus::Ok);
    QCOMPARE(store.results(id).size(), 4);
    QCOMPARE(store.results(other).size(), 1);
}

void TstTimeAttackStore::removeResult()
{
    TimeAttackStore store(&t_.db);
    const int id = make("C");
    QVERIFY(store.addResult(id, 1000, 1) == StoreStatus::Ok);
    QVERIFY(store.addResult(id, 2000, 2) == StoreStatus::Ok);
    const QList<ChallengeResult> list = store.results(id);
    QVERIFY(store.removeResult(list.last().id) == StoreStatus::Ok);
    QCOMPARE(store.results(id).size(), 1);
    QCOMPARE(store.results(id).first().durationMs, qint64(2000));
    QVERIFY(store.removeResult(list.last().id) == StoreStatus::NotFound);
    // The challenge stays.
    Challenge c;
    QVERIFY(store.get(id, &c));
}

void TstTimeAttackStore::listIsSortedByName()
{
    TimeAttackStore store(&t_.db);
    make("banana");
    make("Apple");
    make("cherry");
    const QList<ChallengeSummary> all = store.list();
    QCOMPARE(all.size(), 3);
    QCOMPARE(all.at(0).name, QStringLiteral("Apple"));
    QCOMPARE(all.at(1).name, QStringLiteral("banana"));
    QCOMPARE(all.at(2).name, QStringLiteral("cherry"));
}

void TstTimeAttackStore::foreignKeysAreEnforced()
{
    QSqlQuery q;
    QVERIFY(t_.db.run("PRAGMA foreign_keys", QVariantList(), &q) && q.next());
    QCOMPARE(q.value(0).toInt(), 1);
    QVERIFY(!t_.db.run("INSERT INTO result (challenge_id, duration_ms, finished_at) VALUES (5, 1000, 1)"));
}

void TstTimeAttackStore::crashLeavesNoPartialWrite()
{
    // A transaction that is never committed (the process dies) leaves nothing after reopening.
    const int id = make("C");
    QVERIFY(id > 0);
    {
        Transaction open(&t_.db);
        QVERIFY(open.isActive());
        QVERIFY(t_.db.run("INSERT INTO result (challenge_id, duration_ms, finished_at) VALUES (?, 1000, 1)",
                          QVariantList() << id));
        QVERIFY(t_.db.run("INSERT INTO challenge (name, created_at) VALUES ('Half', 1)"));
        // No commit: the database is closed with the transaction open.
        t_.db.close();
    }
    QVERIFY(t_.db.open(t_.dir.path(), &t_.error));
    QCOMPARE(countRows(t_.db, "challenge"), 1);
    QCOMPARE(countRows(t_.db, "result"), 0);
    QVERIFY(integrityOk(t_.db));
}

void TstTimeAttackStore::crashAfterCommitKeepsData()
{
    int id = 0;
    {
        TimeAttackStore store(&t_.db);
        QVERIFY(store.createChallenge("Kept", 5, &id) == StoreStatus::Ok);
        QVERIFY(store.addResult(id, 4200, 6) == StoreStatus::Ok);
    }
    t_.db.close();
    QVERIFY(t_.db.open(t_.dir.path(), &t_.error));
    TimeAttackStore again(&t_.db);
    ChallengeSummary s;
    QVERIFY(again.summary(id, &s));
    QCOMPARE(s.name, QStringLiteral("Kept"));
    QCOMPARE(s.count, 1);
    QCOMPARE(s.bestMs, qint64(4200));
    QVERIFY(integrityOk(t_.db));
}

void TstTimeAttackStore::nestedRollbackUndoesAResult()
{
    TimeAttackStore store(&t_.db);
    const int id = make("C");
    Transaction outer(&t_.db);
    QVERIFY(outer.isActive());
    QVERIFY(store.addResult(id, 1000, 1) == StoreStatus::Ok);
    {
        Transaction inner(&t_.db);
        QVERIFY(inner.isActive());
        QVERIFY(store.addResult(id, 500, 2) == StoreStatus::Ok);
        // The inner transaction is dropped without a commit: its result is undone.
    }
    QVERIFY(outer.commit());
    QCOMPARE(store.results(id).size(), 1);
    QCOMPARE(store.results(id).first().durationMs, qint64(1000));
}

void TstTimeAttackStore::iconAndColourRoundTrip()
{
    TimeAttackStore store(&t_.db);
    Challenge c;
    c.name = QStringLiteral(" Sprint ");
    c.createdAt = 5;
    c.icon = QStringLiteral("stopwatch");
    c.colour = QStringLiteral("orange");
    int id = 0;
    QVERIFY(store.createChallenge(c, &id) == StoreStatus::Ok);
    Challenge read;
    QVERIFY(store.get(id, &read));
    QCOMPARE(read.name, QStringLiteral("Sprint"));
    QCOMPARE(read.createdAt, qint64(5));
    QCOMPARE(read.icon, QStringLiteral("stopwatch"));
    QCOMPARE(read.colour, QStringLiteral("orange"));
    ChallengeSummary s;
    QVERIFY(store.summary(id, &s));
    QCOMPARE(s.icon, QStringLiteral("stopwatch"));
    QCOMPARE(s.colour, QStringLiteral("orange"));
    QCOMPARE(store.list().first().icon, QStringLiteral("stopwatch"));
    QCOMPARE(store.list().first().colour, QStringLiteral("orange"));

    c.name = QStringLiteral("Sprint 2");
    c.icon = QStringLiteral("flag");
    c.colour.clear();
    QVERIFY(store.updateChallenge(id, c) == StoreStatus::Ok);
    QVERIFY(store.get(id, &read));
    QCOMPARE(read.name, QStringLiteral("Sprint 2"));
    QCOMPARE(read.icon, QStringLiteral("flag"));
    QCOMPARE(read.colour, QString());
    QCOMPARE(read.createdAt, qint64(5));
}

void TstTimeAttackStore::defaultsAreEmptyKeys()
{
    const int id = make("Plain");
    QSqlQuery q;
    QVERIFY(t_.db.run("SELECT icon, colour FROM challenge WHERE id = ?", QVariantList() << id, &q));
    QVERIFY(q.next());
    QVERIFY(!q.value(0).isNull());
    QVERIFY(!q.value(1).isNull());
    QCOMPARE(q.value(0).toString(), QString());
    QCOMPARE(q.value(1).toString(), QString());
}

void TstTimeAttackStore::unknownStoredKeysAreKept()
{
    const int id = make("Later");
    QVERIFY(t_.db.run("UPDATE challenge SET icon = 'future-icon', colour = 'mauve' WHERE id = ?", QVariantList() << id));
    TimeAttackStore store(&t_.db);
    Challenge c;
    QVERIFY(store.get(id, &c));
    QCOMPARE(c.icon, QStringLiteral("future-icon"));
    QCOMPARE(c.colour, QStringLiteral("mauve"));
    ChallengeSummary s;
    QVERIFY(store.summary(id, &s));
    QCOMPARE(s.icon, QStringLiteral("future-icon"));
    QCOMPARE(s.colour, QStringLiteral("mauve"));
}

void TstTimeAttackStore::invalidIconAndColourAreRefusedOnWrite()
{
    TimeAttackStore store(&t_.db);
    Challenge c;
    c.name = QStringLiteral("Bad");
    c.colour = QStringLiteral("mauve");
    QList<ChallengeProblem> problems;
    QVERIFY(store.createChallenge(c, nullptr, &problems) == StoreStatus::Invalid);
    QCOMPARE(problems, QList<ChallengeProblem>() << ChallengeProblem::ColourUnknown);
    c.colour.clear();
    for (const QString &icon : { QStringLiteral("Run"), QStringLiteral("run fast"),
                                 QString(Limits::maxIconKeyChars + 1, QLatin1Char('a')) }) {
        c.icon = icon;
        QVERIFY(store.createChallenge(c, nullptr, &problems) == StoreStatus::Invalid);
        QCOMPARE(problems, QList<ChallengeProblem>() << ChallengeProblem::IconInvalid);
    }
    c.icon = QStringLiteral("anything-new");
    QVERIFY(store.createChallenge(c) == StoreStatus::Ok);
    QCOMPARE(countRows(t_.db, "challenge"), 1);
    // The name rules stay.
    c.name = QStringLiteral("  ");
    QVERIFY(store.createChallenge(c, nullptr, &problems) == StoreStatus::Invalid);
    QCOMPARE(problems, QList<ChallengeProblem>() << ChallengeProblem::NameEmpty);
}

void TstTimeAttackStore::equalIdentityIsRefused()
{
    TimeAttackStore store(&t_.db);
    Challenge c;
    c.name = QStringLiteral("Same");
    c.icon = QStringLiteral("flag");
    c.colour = QStringLiteral("red");
    QVERIFY(store.createChallenge(c) == StoreStatus::Ok);
    QList<ChallengeProblem> problems;
    QVERIFY(store.createChallenge(c, nullptr, &problems) == StoreStatus::Invalid);
    QCOMPARE(problems, QList<ChallengeProblem>() << ChallengeProblem::IdentityTaken);
    QCOMPARE(countRows(t_.db, "challenge"), 1);
}

void TstTimeAttackStore::sameNameWithAnotherColourOrIconIsAccepted()
{
    TimeAttackStore store(&t_.db);
    Challenge c;
    c.name = QStringLiteral("Same");
    c.icon = QStringLiteral("flag");
    c.colour = QStringLiteral("red");
    QVERIFY(store.createChallenge(c) == StoreStatus::Ok);
    c.colour = QStringLiteral("blue");
    QVERIFY(store.createChallenge(c) == StoreStatus::Ok);
    c.colour = QStringLiteral("red");
    c.icon = QStringLiteral("clock");
    QVERIFY(store.createChallenge(c) == StoreStatus::Ok);
    c.icon.clear();
    c.colour.clear();
    QVERIFY(store.createChallenge(c) == StoreStatus::Ok);
    QCOMPARE(countRows(t_.db, "challenge"), 4);
}

void TstTimeAttackStore::caseAndBlankSpaceVariantsAreRefused()
{
    TimeAttackStore store(&t_.db);
    QVERIFY(make("Hill Climb") > 0);
    const QString variants[] = { QStringLiteral("hill climb"), QStringLiteral("HILL CLIMB"),
                                 QStringLiteral("  Hill Climb"), QStringLiteral("Hill Climb \t") };
    for (const QString &name : variants) {
        QList<ChallengeProblem> problems;
        QVERIFY2(store.createChallenge(name, 1, nullptr, &problems) == StoreStatus::Invalid, qPrintable(name));
        QCOMPARE(problems, QList<ChallengeProblem>() << ChallengeProblem::IdentityTaken);
    }
    QCOMPARE(countRows(t_.db, "challenge"), 1);
}

void TstTimeAttackStore::emptyKeysEqualEachOther()
{
    TimeAttackStore store(&t_.db);
    QVERIFY(make("Plain") > 0);
    QCOMPARE(make("plain"), 0);
}

void TstTimeAttackStore::editingWithoutChangingTheIdentityIsAccepted()
{
    TimeAttackStore store(&t_.db);
    Challenge c;
    c.name = QStringLiteral("Mine");
    c.colour = QStringLiteral("blue");
    int id = 0;
    QVERIFY(store.createChallenge(c, &id) == StoreStatus::Ok);
    QVERIFY(store.updateChallenge(id, c) == StoreStatus::Ok);
    QVERIFY(store.renameChallenge(id, QStringLiteral("Mine")) == StoreStatus::Ok);
    QVERIFY(store.renameChallenge(id, QStringLiteral(" MINE ")) == StoreStatus::Ok);
    Challenge read;
    QVERIFY(store.get(id, &read));
    QCOMPARE(read.name, QStringLiteral("MINE"));
}

void TstTimeAttackStore::changingIntoAClashIsRefused()
{
    TimeAttackStore store(&t_.db);
    Challenge a;
    a.name = QStringLiteral("Alpha");
    a.colour = QStringLiteral("red");
    Challenge b = a;
    b.colour = QStringLiteral("blue");
    int idB = 0;
    QVERIFY(store.createChallenge(a) == StoreStatus::Ok);
    QVERIFY(store.createChallenge(b, &idB) == StoreStatus::Ok);
    QList<ChallengeProblem> problems;
    b.colour = QStringLiteral("red");
    QVERIFY(store.updateChallenge(idB, b, &problems) == StoreStatus::Invalid);
    QCOMPARE(problems, QList<ChallengeProblem>() << ChallengeProblem::IdentityTaken);
    Challenge read;
    QVERIFY(store.get(idB, &read));
    QCOMPARE(read.colour, QStringLiteral("blue"));
    // A rename into the name of another challenge with the same icon and colour.
    const int idC = make("Gamma");
    QVERIFY(store.renameChallenge(idC, QStringLiteral("alpha")) == StoreStatus::Ok);
    Challenge gamma;
    QVERIFY(store.get(idC, &gamma));
    gamma.name = QStringLiteral("Alpha");
    gamma.colour = QStringLiteral("red");
    QVERIFY(store.updateChallenge(idC, gamma, &problems) == StoreStatus::Invalid);
    QVERIFY(store.renameChallenge(idB, QStringLiteral("alpha")) == StoreStatus::Ok);
}

void TstTimeAttackStore::renameKeepsIconAndColour()
{
    TimeAttackStore store(&t_.db);
    Challenge c;
    c.name = QStringLiteral("Old");
    c.icon = QStringLiteral("flag");
    c.colour = QStringLiteral("teal");
    int id = 0;
    QVERIFY(store.createChallenge(c, &id) == StoreStatus::Ok);
    QVERIFY(store.renameChallenge(id, QStringLiteral("New")) == StoreStatus::Ok);
    Challenge read;
    QVERIFY(store.get(id, &read));
    QCOMPARE(read.name, QStringLiteral("New"));
    QCOMPARE(read.icon, QStringLiteral("flag"));
    QCOMPARE(read.colour, QStringLiteral("teal"));
    QVERIFY(store.renameChallenge(99, QStringLiteral("x")) == StoreStatus::NotFound);
}

void TstTimeAttackStore::refusedSaveWritesNothing()
{
    TimeAttackStore store(&t_.db);
    const int id = make("One");
    QVERIFY(make("Two") > 0);
    QVERIFY(store.addResult(id, 1000, 1) == StoreStatus::Ok);
    QVERIFY(store.renameChallenge(id, QStringLiteral("two")) == StoreStatus::Invalid);
    Challenge c;
    QVERIFY(store.get(id, &c));
    QCOMPARE(c.name, QStringLiteral("One"));
    QCOMPARE(store.results(id).size(), 1);
    QCOMPARE(countRows(t_.db, "challenge"), 2);
}

QTEST_APPLESS_MAIN(TstTimeAttackStore)
#include "tst_timeattackstore.moc"

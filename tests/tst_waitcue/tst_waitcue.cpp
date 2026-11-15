// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "waitcue.h"

// The cue of a step that waits for a tap (R-WRK-11, R-ALR-1): at the start of the wait and every 3 s.
class TstWaitCue : public QObject
{
    Q_OBJECT

private slots:
    void theRepeatTimeIsThreeSeconds();
    void nothingIsDueBeforeTheWaitBegins();
    void aCueIsDueWhenTheWaitBeginsAndThenEveryThreeSeconds();
    void eachCueIsReportedOnce();
    void aLateCallGetsOneCue();
    void endingTheWaitStopsTheCue();
    void aNewWaitStartsAgainAtOnce();
};

void TstWaitCue::theRepeatTimeIsThreeSeconds()
{
    QCOMPARE(qint64(WaitCue::repeatMs), qint64(3000));
}

void TstWaitCue::nothingIsDueBeforeTheWaitBegins()
{
    WaitCue cue;
    QVERIFY(!cue.active());
    QVERIFY(!cue.due(0));
    QVERIFY(!cue.due(100000));
}

void TstWaitCue::aCueIsDueWhenTheWaitBeginsAndThenEveryThreeSeconds()
{
    WaitCue cue;
    cue.begin(10000);
    QVERIFY(cue.active());
    QVERIFY(cue.due(10000));
    QVERIFY(!cue.due(12999));
    QVERIFY(cue.due(13000));
    QVERIFY(!cue.due(15999));
    QVERIFY(cue.due(16000));
}

void TstWaitCue::eachCueIsReportedOnce()
{
    WaitCue cue;
    cue.begin(0);
    QVERIFY(cue.due(0));
    QVERIFY(!cue.due(0));
    QVERIFY(!cue.due(1));
    QVERIFY(cue.due(3000));
    QVERIFY(!cue.due(3000));
}

void TstWaitCue::aLateCallGetsOneCue()
{
    WaitCue cue;
    cue.begin(0);
    QVERIFY(cue.due(0));
    // The app was suspended for 20 s: one cue, then the beat goes on from the periods of the begin.
    QVERIFY(cue.due(20000));
    QVERIFY(!cue.due(20500));
    QVERIFY(cue.due(21000));
}

void TstWaitCue::endingTheWaitStopsTheCue()
{
    WaitCue cue;
    cue.begin(0);
    QVERIFY(cue.due(0));
    cue.end();
    QVERIFY(!cue.active());
    QVERIFY(!cue.due(3000));
    QVERIFY(!cue.due(60000));
}

void TstWaitCue::aNewWaitStartsAgainAtOnce()
{
    WaitCue cue;
    cue.begin(0);
    cue.due(0);
    cue.end();
    cue.begin(50000);
    QVERIFY(cue.due(50000));
    QVERIFY(!cue.due(52000));
    QVERIFY(cue.due(53000));
}

QTEST_APPLESS_MAIN(TstWaitCue)
#include "tst_waitcue.moc"

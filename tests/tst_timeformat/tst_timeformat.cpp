// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "timeformat.h"

class TstTimeFormat : public QObject
{
    Q_OBJECT

private slots:
    void countdown_data();
    void countdown();
    void partsFollowTheMagnitude_data();
    void partsFollowTheMagnitude();
    void partsJoinedAreTheCountdownTextWithoutAMinuteZero();
    void preciseKeepsHundredths_data();
    void preciseKeepsHundredths();
    void agoUsesTheLargestUnit_data();
    void agoUsesTheLargestUnit();
};

void TstTimeFormat::countdown_data()
{
    QTest::addColumn<int>("seconds");
    QTest::addColumn<QString>("expected");

    QTest::newRow("zero") << 0 << "00:00";
    QTest::newRow("one second") << 1 << "00:01";
    QTest::newRow("59 seconds") << 59 << "00:59";
    QTest::newRow("one minute") << 60 << "01:00";
    QTest::newRow("just below one hour") << 3599 << "59:59";
    QTest::newRow("one hour") << 3600 << "1:00:00";
    QTest::newRow("one hour, one minute, one second") << 3661 << "1:01:01";
    QTest::newRow("negative clamps to zero") << -5 << "00:00";
}

void TstTimeFormat::countdown()
{
    QFETCH(int, seconds);
    QFETCH(QString, expected);

    QCOMPARE(TimeFormat::countdown(seconds), expected);
}

void TstTimeFormat::partsFollowTheMagnitude_data()
{
    QTest::addColumn<int>("seconds");
    QTest::addColumn<QString>("first");
    QTest::addColumn<bool>("firstLarge");
    QTest::addColumn<QString>("second");

    QTest::newRow("zero") << 0 << "0:" << false << "00";
    QTest::newRow("five seconds") << 5 << "0:" << false << "05";
    QTest::newRow("59 seconds") << 59 << "0:" << false << "59";
    QTest::newRow("one minute") << 60 << "1" << true << ":00";
    QTest::newRow("9 min 59 s") << 599 << "9" << true << ":59";
    QTest::newRow("10 minutes") << 600 << "10" << true << ":00";
    QTest::newRow("59 min 59 s") << 3599 << "59" << true << ":59";
    QTest::newRow("one hour") << 3600 << "1" << true << ":00:00";
    QTest::newRow("3 h 5 min 9 s") << (3 * 3600 + 5 * 60 + 9) << "3" << true << ":05:09";
    QTest::newRow("24 hours") << 86400 << "24" << true << ":00:00";
    QTest::newRow("negative") << -4 << "0:" << false << "00";
    QTest::newRow("99 h 59 min 59 s") << 359999 << "99" << true << ":59:59";
}

void TstTimeFormat::partsFollowTheMagnitude()
{
    QFETCH(int, seconds);
    QFETCH(QString, first);
    QFETCH(bool, firstLarge);
    QFETCH(QString, second);

    const QList<TimeFormat::Part> list = TimeFormat::parts(seconds);
    QCOMPARE(list.size(), 2);
    QCOMPARE(list.at(0).text, first);
    QCOMPARE(list.at(0).large, firstLarge);
    QCOMPARE(list.at(1).text, second);
    // Exactly one of the two parts is large.
    QCOMPARE(list.at(1).large, !firstLarge);
}

void TstTimeFormat::partsJoinedAreTheCountdownTextWithoutAMinuteZero()
{
    const int values[] = { 0, 1, 9, 10, 59, 60, 61, 599, 600, 3599, 3600, 3601, 3661, 36000, 86399, 86400 };
    for (int seconds : values) {
        const QList<TimeFormat::Part> list = TimeFormat::parts(seconds);
        // Below one hour the minutes have no leading zero.
        QString expected = TimeFormat::countdown(seconds);
        if (seconds < 3600 && expected.startsWith(QLatin1Char('0')))
            expected.remove(0, 1);
        QCOMPARE(list.at(0).text + list.at(1).text, expected);
    }
}

void TstTimeFormat::preciseKeepsHundredths_data()
{
    QTest::addColumn<qint64>("ms");
    QTest::addColumn<QString>("expected");

    QTest::newRow("one ms") << qint64(1) << "0:00.00";
    QTest::newRow("ten ms") << qint64(10) << "0:00.01";
    QTest::newRow("cut not rounded") << qint64(1999) << "0:01.99";
    QTest::newRow("a minute") << qint64(60000) << "1:00.00";
    QTest::newRow("12.345 s") << qint64(12345) << "0:12.34";
    QTest::newRow("59 min") << qint64(3599990) << "59:59.99";
    QTest::newRow("one hour") << qint64(3600000) << "1:00:00.00";
    QTest::newRow("24 hours") << qint64(86400000) << "24:00:00.00";
    QTest::newRow("negative") << qint64(-7) << "0:00.00";
}

void TstTimeFormat::preciseKeepsHundredths()
{
    QFETCH(qint64, ms);
    QFETCH(QString, expected);
    QCOMPARE(TimeFormat::precise(ms), expected);
}


void TstTimeFormat::agoUsesTheLargestUnit_data()
{
    QTest::addColumn<qint64>("ms");
    QTest::addColumn<int>("kind");
    QTest::addColumn<int>("major");
    QTest::addColumn<int>("minor");

    const qint64 min = 60000;
    const qint64 hour = 60 * min;
    const qint64 day = 24 * hour;
    typedef TimeFormat::Ago A;
    QTest::newRow("zero") << qint64(0) << int(A::JustNow) << 0 << 0;
    QTest::newRow("negative") << qint64(-5000) << int(A::JustNow) << 0 << 0;
    QTest::newRow("59 s 999") << (min - 1) << int(A::JustNow) << 0 << 0;
    QTest::newRow("one minute") << min << int(A::Minutes) << 1 << 0;
    QTest::newRow("five minutes") << (5 * min + 30000) << int(A::Minutes) << 5 << 0;
    QTest::newRow("59 minutes") << (59 * min) << int(A::Minutes) << 59 << 0;
    QTest::newRow("one hour") << hour << int(A::HoursMinutes) << 1 << 0;
    QTest::newRow("3 h 20 min") << (3 * hour + 20 * min) << int(A::HoursMinutes) << 3 << 20;
    QTest::newRow("23 h 59 min") << (day - min) << int(A::HoursMinutes) << 23 << 59;
    QTest::newRow("one day") << day << int(A::DaysHours) << 1 << 0;
    QTest::newRow("2 d 4 h") << (2 * day + 4 * hour + 59 * min) << int(A::DaysHours) << 2 << 4;
    QTest::newRow("400 days") << (400 * day) << int(A::DaysHours) << 400 << 0;
}

void TstTimeFormat::agoUsesTheLargestUnit()
{
    QFETCH(qint64, ms);
    QFETCH(int, kind);
    QFETCH(int, major);
    QFETCH(int, minor);
    const TimeFormat::Ago a = TimeFormat::ago(ms);
    QCOMPARE(int(a.kind), kind);
    QCOMPARE(a.major, major);
    QCOMPARE(a.minor, minor);
}

QTEST_APPLESS_MAIN(TstTimeFormat)

#include "tst_timeformat.moc"

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include <QTemporaryFile>

#include "statusline.h"

// The values of the status line (R-UI-15): the time and the date by locale, the minute change, the battery.
class TstStatusLine : public QObject
{
    Q_OBJECT

private slots:
    void theTimeFollowsTheLocale();
    void theDateIsShortAndInTheLanguageOfTheLocale();
    void theNextChangeIsAtTheNextMinute();
    void theValueChangesOnlyWhenTheMinuteDoes();
    void theBatteryIsReadFromTheText();
    void aFileThatCannotBeReadHasNoBattery();
    void theBatteryIsReadFromAFile();
};

static QDateTime at(int h, int m, int s, int ms = 0)
{
    return QDateTime(QDate(2026, 11, 14), QTime(h, m, s, ms), Qt::UTC);
}

void TstStatusLine::theTimeFollowsTheLocale()
{
    const QDateTime t = at(14, 32, 10);
    QCOMPARE(StatusLine::timeText(t, QLocale(QLocale::Italian, QLocale::Italy)), QStringLiteral("14:32"));
    QVERIFY(StatusLine::timeText(t, QLocale(QLocale::English, QLocale::UnitedStates)).contains(QStringLiteral("2:32")));
}

void TstStatusLine::theDateIsShortAndInTheLanguageOfTheLocale()
{
    const QDateTime t = at(14, 32, 10);
    QCOMPARE(StatusLine::dateText(t, QLocale(QLocale::English, QLocale::UnitedKingdom)), QStringLiteral("Sat 14 Nov"));
    const QString it = StatusLine::dateText(t, QLocale(QLocale::Italian, QLocale::Italy));
    QVERIFY(it.contains(QStringLiteral("14")));
    QVERIFY(it.contains(QStringLiteral("nov"), Qt::CaseInsensitive));
    QVERIFY(it.contains(QStringLiteral("sab"), Qt::CaseInsensitive));
}

void TstStatusLine::theNextChangeIsAtTheNextMinute()
{
    QCOMPARE(StatusLine::msToNextMinute(at(14, 32, 0)), qint64(60000));
    QCOMPARE(StatusLine::msToNextMinute(at(14, 32, 10)), qint64(50000));
    QCOMPARE(StatusLine::msToNextMinute(at(14, 32, 59, 900)), qint64(100));
    QVERIFY(StatusLine::msToNextMinute(at(14, 32, 59, 999)) >= 1);
}

void TstStatusLine::theValueChangesOnlyWhenTheMinuteDoes()
{
    const QLocale l(QLocale::Italian, QLocale::Italy);
    QCOMPARE(StatusLine::timeText(at(14, 32, 0), l), StatusLine::timeText(at(14, 32, 59, 999), l));
    const qint64 wait = StatusLine::msToNextMinute(at(14, 32, 20, 250));
    QCOMPARE(wait, qint64(39750));
    const QDateTime later = at(14, 32, 20, 250).addMSecs(wait);
    QCOMPARE(StatusLine::timeText(later, l), QStringLiteral("14:33"));
    // The date changes at midnight.
    const QDateTime midnight = at(23, 59, 30).addMSecs(StatusLine::msToNextMinute(at(23, 59, 30)));
    QCOMPARE(StatusLine::dateText(midnight, QLocale(QLocale::English, QLocale::UnitedKingdom)), QStringLiteral("Sun 15 Nov"));
}

void TstStatusLine::theBatteryIsReadFromTheText()
{
    QCOMPARE(StatusLine::parseBattery("82\n"), 82);
    QCOMPARE(StatusLine::parseBattery("0"), 0);
    QCOMPARE(StatusLine::parseBattery("100\n"), 100);
    QCOMPARE(StatusLine::parseBattery(""), -1);
    QCOMPARE(StatusLine::parseBattery("abc"), -1);
    QCOMPARE(StatusLine::parseBattery("101"), -1);
    QCOMPARE(StatusLine::parseBattery("-5"), -1);
}

void TstStatusLine::aFileThatCannotBeReadHasNoBattery()
{
    QCOMPARE(StatusLine::readBattery(QStringLiteral("/nonexistent/power/capacity")), -1);
}

void TstStatusLine::theBatteryIsReadFromAFile()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("57\n");
    file.flush();
    QCOMPARE(StatusLine::readBattery(file.fileName()), 57);
    file.resize(0);
    file.seek(0);
    file.write("junk\n");
    file.flush();
    QCOMPARE(StatusLine::readBattery(file.fileName()), -1);
}

QTEST_APPLESS_MAIN(TstStatusLine)
#include "tst_statusline.moc"

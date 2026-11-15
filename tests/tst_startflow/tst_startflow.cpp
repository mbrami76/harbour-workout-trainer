// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "startflow.h"

class TstStartFlow : public QObject
{
    Q_OBJECT

private slots:
    void missingConfirmationIsNotConfirmed();
    void onlyABooleanTrueIsConfirmed();
    void languageComesBeforeTheNotice();
    void noticeIsShownWhenNotConfirmed();
    void noticeIsNotShownWhenConfirmed();
    void aLanguageChangeDoesNotResetTheConfirmation();
    void dataErrorComesFirst();
    void pagesHaveNames();
};

void TstStartFlow::missingConfirmationIsNotConfirmed()
{
    QVERIFY(!StartFlow::noticeConfirmed(QVariant()));
    QVERIFY(!StartFlow::noticeConfirmed(QVariant(false)));
}

void TstStartFlow::onlyABooleanTrueIsConfirmed()
{
    QVERIFY(StartFlow::noticeConfirmed(QVariant(true)));
    QVERIFY(!StartFlow::noticeConfirmed(QVariant(QStringLiteral("true"))));
    QVERIFY(!StartFlow::noticeConfirmed(QVariant(1)));
    QVERIFY(!StartFlow::noticeConfirmed(QVariant(QStringLiteral(""))));
}

void TstStartFlow::languageComesBeforeTheNotice()
{
    QCOMPARE(StartFlow::firstPage(true, QString(), false), StartFlow::LanguageChoice);
    QCOMPARE(StartFlow::firstPage(true, QStringLiteral("de"), false), StartFlow::LanguageChoice);
    // A confirmation left over without a language still asks for the language first.
    QCOMPARE(StartFlow::firstPage(true, QString(), true), StartFlow::LanguageChoice);
}

void TstStartFlow::noticeIsShownWhenNotConfirmed()
{
    QCOMPARE(StartFlow::firstPage(true, QStringLiteral("en"), false), StartFlow::SafetyNotice);
    QCOMPARE(StartFlow::firstPage(true, QStringLiteral("it"), false), StartFlow::SafetyNotice);
}

void TstStartFlow::noticeIsNotShownWhenConfirmed()
{
    QCOMPARE(StartFlow::firstPage(true, QStringLiteral("en"), true), StartFlow::Main);
    QCOMPARE(StartFlow::firstPage(true, QStringLiteral("it"), true), StartFlow::Main);
}

void TstStartFlow::aLanguageChangeDoesNotResetTheConfirmation()
{
    const bool confirmed = StartFlow::noticeConfirmed(QVariant(true));
    QCOMPARE(StartFlow::firstPage(true, QStringLiteral("en"), confirmed), StartFlow::Main);
    // The language is changed in the settings; the confirmation is read again and is still there.
    QCOMPARE(StartFlow::firstPage(true, QStringLiteral("it"), confirmed), StartFlow::Main);
    QCOMPARE(StartFlow::firstPage(true, QStringLiteral("en"), confirmed), StartFlow::Main);
}

void TstStartFlow::dataErrorComesFirst()
{
    QCOMPARE(StartFlow::firstPage(false, QString(), false), StartFlow::Error);
    QCOMPARE(StartFlow::firstPage(false, QStringLiteral("en"), true), StartFlow::Error);
}

void TstStartFlow::pagesHaveNames()
{
    QCOMPARE(StartFlow::pageName(StartFlow::Error), QStringLiteral("error"));
    QCOMPARE(StartFlow::pageName(StartFlow::LanguageChoice), QStringLiteral("language"));
    QCOMPARE(StartFlow::pageName(StartFlow::SafetyNotice), QStringLiteral("notice"));
    QCOMPARE(StartFlow::pageName(StartFlow::Main), QStringLiteral("main"));
}

QTEST_APPLESS_MAIN(TstStartFlow)

#include "tst_startflow.moc"

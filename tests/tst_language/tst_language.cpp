// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "language.h"

class TstLanguage : public QObject
{
    Q_OBJECT

private slots:
    void shippedLanguagesAreEnglishAndItalian();
    void storedValueReadsAsAShippedTagOrNothing();
    void systemLanguageIsPreselectedWhenShipped();
    void newExerciseTakesTheChosenLanguage();
    void editorOffersShippedTagsThenTheOthers();
    void fileNamesFollowTheLanguage();
};

void TstLanguage::shippedLanguagesAreEnglishAndItalian()
{
    QCOMPARE(Language::shipped(), QStringList() << "en" << "it");
    QVERIFY(Language::isShipped("en"));
    QVERIFY(Language::isShipped("it"));
    QVERIFY(!Language::isShipped("de"));
    QVERIFY(!Language::isShipped(""));
    QVERIFY(!Language::isShipped("IT"));
}

void TstLanguage::storedValueReadsAsAShippedTagOrNothing()
{
    QCOMPARE(Language::shippedFrom("en"), QStringLiteral("en"));
    QCOMPARE(Language::shippedFrom("it"), QStringLiteral("it"));
    QCOMPARE(Language::shippedFrom("it_IT"), QStringLiteral("it"));
    QCOMPARE(Language::shippedFrom("IT-ch"), QStringLiteral("it"));
    QCOMPARE(Language::shippedFrom(" en_GB "), QStringLiteral("en"));
    QCOMPARE(Language::shippedFrom(""), QString());
    QCOMPARE(Language::shippedFrom("fr"), QString());
    QCOMPARE(Language::shippedFrom("ita"), QString());
    QCOMPARE(Language::shippedFrom("../it"), QString());
}

void TstLanguage::systemLanguageIsPreselectedWhenShipped()
{
    QCOMPARE(Language::preselected("it_IT"), QStringLiteral("it"));
    QCOMPARE(Language::preselected("en_US"), QStringLiteral("en"));
    QCOMPARE(Language::preselected("de_DE"), QStringLiteral("en"));
    QCOMPARE(Language::preselected("C"), QStringLiteral("en"));
    QCOMPARE(Language::preselected(""), QStringLiteral("en"));
}

void TstLanguage::newExerciseTakesTheChosenLanguage()
{
    QCOMPARE(Language::newExerciseTag("it"), QStringLiteral("it"));
    QCOMPARE(Language::newExerciseTag("en"), QStringLiteral("en"));
    // No choice yet.
    QCOMPARE(Language::newExerciseTag(""), QStringLiteral("en"));
}

void TstLanguage::editorOffersShippedTagsThenTheOthers()
{
    QCOMPARE(Language::exerciseChoices(QStringList()), QStringList() << "en" << "it");
    QCOMPARE(Language::exerciseChoices(QStringList() << "it" << "xx" << "de" << "xx" << "en"),
             QStringList() << "en" << "it" << "de" << "xx");
}

void TstLanguage::fileNamesFollowTheLanguage()
{
    QCOMPARE(Language::structureFileName(), QStringLiteral("exercises.json"));
    QCOMPARE(Language::textsFileName("it"), QStringLiteral("texts.it.json"));
    QCOMPARE(Language::workoutsFileName(), QStringLiteral("workouts.json"));
}

QTEST_APPLESS_MAIN(TstLanguage)

#include "tst_language.moc"

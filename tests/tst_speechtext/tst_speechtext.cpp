// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "speech.h"
#include "speechtext.h"

// Where the voice pauses (R-ALR-14): after the end of a sentence and after a comma, a semicolon or
// a colon; nothing after the last part; numbers and abbreviations are not cut.
namespace {

const int sentence = 600;
const int clause = 300;

QStringList parts(const QString &text)
{
    QStringList list;
    for (const SpeechText::Segment &s : SpeechText::segments(text, sentence, clause))
        list.append(QStringLiteral("%1|%2").arg(s.text).arg(s.pauseMs));
    return list;
}

}

class TstSpeechText : public QObject
{
    Q_OBJECT

private slots:
    void sentenceEndsPauseTheSentenceLength();
    void commasSemicolonsAndColonsPauseTheClauseLength();
    void noPauseEndsTheText();
    void decimalsThousandsAndTimesAreNotCut();
    void abbreviationsAreNotSentenceEnds();
    void aNumberBeforeAFullStopEndsTheSentence();
    void runsOfMarksAndClosingQuotesGoTogether();
    void marksAloneAreNotSpokenOnTheirOwn();
    void emptyAndBlankTextsGiveNothing();
    void theConstantsAreTheFirstValues();
};

void TstSpeechText::sentenceEndsPauseTheSentenceLength()
{
    QCOMPARE(parts("Stand tall. Is it hard? Yes! Go."),
             QStringList() << "Stand tall.|600" << "Is it hard?|600" << "Yes!|600" << "Go.|0");
}

void TstSpeechText::commasSemicolonsAndColonsPauseTheClauseLength()
{
    QCOMPARE(parts("Stand tall, chest up; breathe: relax"),
             QStringList() << "Stand tall,|300" << "chest up;|300" << "breathe:|300" << "relax|0");
}

void TstSpeechText::noPauseEndsTheText()
{
    // The last part has no pause, whatever it ends with: the step timer waits for the last word.
    QCOMPARE(parts("Stand tall."), QStringList() << "Stand tall.|0");
    QCOMPARE(parts("Stand tall,"), QStringList() << "Stand tall,|0");
    QCOMPARE(parts("Stand tall. "), QStringList() << "Stand tall.|0");
    QCOMPARE(parts("No marks at all"), QStringList() << "No marks at all|0");
}

void TstSpeechText::decimalsThousandsAndTimesAreNotCut()
{
    QCOMPARE(parts("Hold for 1.5 seconds."), QStringList() << "Hold for 1.5 seconds.|0");
    QCOMPARE(parts("Do 1,000 steps, then rest."), QStringList() << "Do 1,000 steps,|300" << "then rest.|0");
    QCOMPARE(parts("Start at 10:30 sharp."), QStringList() << "Start at 10:30 sharp.|0");
    QCOMPARE(parts("Tieni 1,5 secondi. Poi riposa."), QStringList() << "Tieni 1,5 secondi.|600" << "Poi riposa.|0");
}

void TstSpeechText::abbreviationsAreNotSentenceEnds()
{
    QCOMPARE(parts("Use a wall, e.g. a flat one. Then go."),
             QStringList() << "Use a wall,|300" << "e.g. a flat one.|600" << "Then go.|0");
    QCOMPARE(parts("Slow, i.e. controlled. Then go."),
             QStringList() << "Slow,|300" << "i.e. controlled.|600" << "Then go.|0");
    QCOMPARE(parts("Usa un appoggio, es. un muro. Poi vai."),
             QStringList() << "Usa un appoggio,|300" << "es. un muro.|600" << "Poi vai.|0");
    // "etc." ends the sentence only when a capital letter follows.
    QCOMPARE(parts("Arms, legs etc. are used."),
             QStringList() << "Arms,|300" << "legs etc. are used.|0");
    QCOMPARE(parts("Arms, legs etc. Then rest."),
             QStringList() << "Arms,|300" << "legs etc.|600" << "Then rest.|0");
}

void TstSpeechText::aNumberBeforeAFullStopEndsTheSentence()
{
    QCOMPARE(parts("Do 30. Then rest."), QStringList() << "Do 30.|600" << "Then rest.|0");
}

void TstSpeechText::runsOfMarksAndClosingQuotesGoTogether()
{
    QCOMPARE(parts("Really?! Yes... go on."), QStringList() << "Really?!|600" << "Yes...|600" << "go on.|0");
    QCOMPARE(parts("He said \"go.\" Then left."), QStringList() << "He said \"go.\"|600" << "Then left.|0");
}

void TstSpeechText::marksAloneAreNotSpokenOnTheirOwn()
{
    QCOMPARE(parts("Go. ..."), QStringList() << "Go. ...|0");
    QCOMPARE(parts("Go, , now"), QStringList() << "Go, ,|300" << "now|0");
}

void TstSpeechText::emptyAndBlankTextsGiveNothing()
{
    QVERIFY(parts("").isEmpty());
    QVERIFY(parts("   ").isEmpty());
    QVERIFY(parts(".").isEmpty());
}

void TstSpeechText::theConstantsAreTheFirstValues()
{
    QCOMPARE(SpeechTiming::sentencePauseMs, 600);
    QCOMPARE(SpeechTiming::clausePauseMs, 300);
    QCOMPARE(SpeechTiming::betweenTextsMs, 600);
}

QTEST_APPLESS_MAIN(TstSpeechText)
#include "tst_speechtext.moc"

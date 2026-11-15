// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "speechsentences.h"

#include <QTranslator>

#include "languagebridge.h"

QString AppSentences::english(const QString &source, int n)
{
    QString text = source;
    text.replace(QStringLiteral("(s)"), n == 1 ? QString() : QStringLiteral("s"));
    text.replace(QStringLiteral("%n"), QString::number(n));
    return text;
}

QString AppSentences::text(const QString &language, Sentence sentence, int n) const
{
    QTranslator *translator = language == QLatin1String("en") ? nullptr : languages_->translatorOf(language);
    QString translated;
    QString source;
    switch (sentence) {
    case Sentence::GetReadyToPerform:
        source = QStringLiteral("Get ready to perform %1");
        if (translator)
            translated = translator->translate("Speech", "Get ready to perform %1");
        break;
    case Sentence::GetReady:
        source = QStringLiteral("Get ready");
        if (translator)
            translated = translator->translate("Speech", "Get ready");
        break;
    case Sentence::Repetitions:
        source = QStringLiteral("%n repetition(s)");
        if (translator)
            translated = translator->translate("Speech", "%n repetition(s)", nullptr, n);
        break;
    case Sentence::Seconds:
        source = QStringLiteral("%n second(s)");
        if (translator)
            translated = translator->translate("Speech", "%n second(s)", nullptr, n);
        break;
    case Sentence::Minutes:
        source = QStringLiteral("%n minute(s)");
        if (translator)
            translated = translator->translate("Speech", "%n minute(s)", nullptr, n);
        break;
    case Sentence::RestFor:
        source = QStringLiteral("Now you can rest for %1");
        if (translator)
            translated = translator->translate("Speech", "Now you can rest for %1");
        break;
    case Sentence::Halfway:
        source = QStringLiteral("Halfway");
        if (translator)
            translated = translator->translate("Speech", "Halfway");
        break;
    case Sentence::TenSecondsLeft:
        source = QStringLiteral("Ten seconds left");
        if (translator)
            translated = translator->translate("Speech", "Ten seconds left");
        break;
    case Sentence::ToGo:
        source = QStringLiteral("%1 to go");
        if (translator)
            translated = translator->translate("Speech", "%1 to go");
        break;
    case Sentence::ThreeToGo:
        source = QStringLiteral("Three to go");
        if (translator)
            translated = translator->translate("Speech", "Three to go");
        break;
    }
    if (translated.isEmpty())
        return english(source, n);
    translated.replace(QStringLiteral("%n"), QString::number(n));
    return translated;
}

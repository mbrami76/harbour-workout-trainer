// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef SPEECHSENTENCES_H
#define SPEECHSENTENCES_H

#include "speech.h"

class LanguageBridge;

// The fixed spoken sentences as messages of the interface translation (R-ALR-6, R-I18N-2). They are
// looked up in the translation of the language of the text they introduce, not of the interface;
// English is the fallback for a language that has no translation of a sentence.
class AppSentences : public Sentences
{
public:
    explicit AppSentences(LanguageBridge *languages) : languages_(languages) {}
    QString text(const QString &language, Sentence sentence, int n = 0) const override;

    // A source message with its number form in English: "(s)" is dropped for one and becomes "s"
    // otherwise, and "%n" is replaced by the number.
    static QString english(const QString &source, int n);

private:
    LanguageBridge *languages_;
};

#endif

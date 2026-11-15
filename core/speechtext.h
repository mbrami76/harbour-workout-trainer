// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef SPEECHTEXT_H
#define SPEECHTEXT_H

#include <QList>
#include <QString>

// Cuts a text for speech where the voice pauses (R-ALR-14). Pure logic.
namespace SpeechText {

// A part of a text, with its punctuation, and the silence that follows it in the speech; 0 for the
// last part of a text, so that no pause ends an announcement.
struct Segment {
    Segment(const QString &t = QString(), int pause = 0) : text(t), pauseMs(pause) {}
    QString text;
    int pauseMs = 0;
};

// Cuts after the end of a sentence (. ? !, also a run of them and closing quotes or brackets) and
// after a comma, a semicolon or a colon. A mark counts only when it is followed by white space or
// the end of the text, so that "1.5", "1,000" and "10:30" stay whole. A full stop after an
// abbreviation does not end a sentence: "e.g.", "i.e.", "es.", "p.es.", "cfr.", "vs." and "approx."
// never do; "etc." and "ecc." do not when a lower case word follows. A number before a full stop
// ("Do 30.") does end it. Parts without letters or digits are not made.
QList<Segment> segments(const QString &text, int sentencePauseMs, int clausePauseMs);

}

#endif

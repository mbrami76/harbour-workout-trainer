// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "speechtext.h"

#include <QSet>

namespace {

bool isClosing(QChar c)
{
    return c == QLatin1Char('"') || c == QLatin1Char('\'') || c == QLatin1Char(')') || c == QLatin1Char(']')
        || c == QChar(0x201D) || c == QChar(0x2019) || c == QChar(0x00BB);
}

bool isTerminator(QChar c)
{
    return c == QLatin1Char('.') || c == QLatin1Char('?') || c == QLatin1Char('!') || c == QChar(0x2026);
}

// The word that ends just before position `end`, lower case, with its inner full stops ("p.es").
QString wordBefore(const QString &text, int end)
{
    int start = end;
    while (start > 0 && (text.at(start - 1).isLetter() || text.at(start - 1) == QLatin1Char('.')))
        --start;
    return text.mid(start, end - start).toLower();
}

const QSet<QString> &alwaysAbbreviations()
{
    static const QSet<QString> set = {QStringLiteral("e.g"), QStringLiteral("i.e"), QStringLiteral("es"),
                                      QStringLiteral("p.es"), QStringLiteral("cfr"), QStringLiteral("vs"),
                                      QStringLiteral("approx")};
    return set;
}

const QSet<QString> &conditionalAbbreviations()
{
    static const QSet<QString> set = {QStringLiteral("etc"), QStringLiteral("ecc")};
    return set;
}

bool hasContent(const QString &text)
{
    for (QChar c : text) {
        if (c.isLetterOrNumber())
            return true;
    }
    return false;
}

}

QList<SpeechText::Segment> SpeechText::segments(const QString &text, int sentencePauseMs, int clausePauseMs)
{
    QList<Segment> out;
    const QString t = text.trimmed();
    int start = 0;
    int i = 0;
    auto cut = [&](int end, int pause) {
        const QString part = t.mid(start, end - start).trimmed();
        if (hasContent(part)) {
            out.append(Segment(part, pause));
        } else if (!out.isEmpty() && !part.isEmpty()) {
            // Marks alone belong to the part before them.
            out.last().text += QLatin1Char(' ') + part;
        }
        start = end;
    };
    while (i < t.size()) {
        const QChar c = t.at(i);
        const bool clause = c == QLatin1Char(',') || c == QLatin1Char(';') || c == QLatin1Char(':');
        if (!isTerminator(c) && !clause) {
            ++i;
            continue;
        }
        // The run of marks and closing characters that goes with this one.
        int end = i + 1;
        if (!clause) {
            while (end < t.size() && (isTerminator(t.at(end)) || isClosing(t.at(end))))
                ++end;
        }
        const bool atBoundary = end >= t.size() || t.at(end).isSpace();
        if (!atBoundary) {
            i = end;
            continue;
        }
        if (c == QLatin1Char('.') && end == i + 1) {
            const QString word = wordBefore(t, i);
            int next = end;
            while (next < t.size() && t.at(next).isSpace())
                ++next;
            const bool lowerNext = next < t.size() && t.at(next).isLower();
            if (alwaysAbbreviations().contains(word) || (lowerNext && conditionalAbbreviations().contains(word))) {
                i = end;
                continue;
            }
        }
        cut(end, clause ? clausePauseMs : sentencePauseMs);
        i = end;
    }
    cut(t.size(), 0);
    if (!out.isEmpty())
        out.last().pauseMs = 0;
    return out;
}

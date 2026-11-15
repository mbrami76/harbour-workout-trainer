// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "speechengine.h"

QString VoiceProvider::voiceFor(const QString &language) const
{
    const QString tag = language.toLower();
    const QString primary = tag.section(QLatin1Char('-'), 0, 0);
    QString fallback;
    for (const VoiceInfo &v : voices()) {
        const QString voiceTag = v.language.toLower();
        if (voiceTag == tag)
            return v.id;
        if (fallback.isEmpty() && voiceTag == primary)
            fallback = v.id;
    }
    return fallback;
}

QSet<QString> VoiceProvider::languages() const
{
    QSet<QString> tags;
    for (const VoiceInfo &v : voices())
        tags.insert(v.language.toLower());
    return tags;
}

VoiceProvider::Reason VoiceProvider::reason() const
{
    if (!hasEngine())
        return Reason::NoEngine;
    return voices().isEmpty() ? Reason::NoVoices : Reason::None;
}

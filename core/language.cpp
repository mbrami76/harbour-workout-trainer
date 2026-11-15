// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "language.h"

QStringList Language::shipped()
{
    return QStringList() << QStringLiteral("en") << QStringLiteral("it");
}

bool Language::isShipped(const QString &tag)
{
    return shipped().contains(tag);
}

QString Language::shippedFrom(const QString &value)
{
    // Only the language part counts: the region and script of a system locale are dropped.
    const QString base = value.trimmed().toLower().split(QLatin1Char('_')).first().split(QLatin1Char('-')).first();
    return isShipped(base) ? base : QString();
}

QString Language::preselected(const QString &systemTag)
{
    const QString tag = shippedFrom(systemTag);
    return tag.isEmpty() ? QStringLiteral("en") : tag;
}

QStringList Language::exerciseChoices(const QStringList &tagsInUse)
{
    QStringList others;
    for (const QString &tag : tagsInUse) {
        if (!isShipped(tag) && !others.contains(tag))
            others.append(tag);
    }
    others.sort();
    return shipped() + others;
}

QString Language::newExerciseTag(const QString &chosen)
{
    return isShipped(chosen) ? chosen : QStringLiteral("en");
}

QString Language::structureFileName()
{
    return QStringLiteral("exercises.json");
}

QString Language::textsFileName(const QString &language)
{
    return QStringLiteral("texts.%1.json").arg(language);
}

QString Language::workoutsFileName()
{
    return QStringLiteral("workouts.json");
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "colours.h"

QStringList Colours::keys()
{
    return QStringList() << QStringLiteral("red") << QStringLiteral("orange") << QStringLiteral("yellow")
                         << QStringLiteral("green") << QStringLiteral("teal") << QStringLiteral("blue")
                         << QStringLiteral("purple") << QStringLiteral("pink") << QStringLiteral("brown")
                         << QStringLiteral("grey");
}

bool Colours::isKnown(const QString &key)
{
    return keys().contains(key);
}

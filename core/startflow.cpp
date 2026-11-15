// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "startflow.h"

#include "language.h"

bool StartFlow::noticeConfirmed(const QVariant &stored)
{
    return stored.type() == QVariant::Bool && stored.toBool();
}

StartFlow::Page StartFlow::firstPage(bool dataReady, const QString &storedLanguage, bool noticeConfirmed)
{
    if (!dataReady)
        return Error;
    if (!Language::isShipped(storedLanguage))
        return LanguageChoice;
    return noticeConfirmed ? Main : SafetyNotice;
}

QString StartFlow::pageName(Page page)
{
    switch (page) {
    case Error: return QStringLiteral("error");
    case LanguageChoice: return QStringLiteral("language");
    case SafetyNotice: return QStringLiteral("notice");
    case Main: break;
    }
    return QStringLiteral("main");
}

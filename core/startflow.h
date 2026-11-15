// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef STARTFLOW_H
#define STARTFLOW_H

#include <QString>
#include <QVariant>

// What the app shows first (R-I18N-5, R-GEN-5): the error page when the data cannot be used, then
// the language choice, then the safety notice, then the app.
namespace StartFlow {

enum Page { Error, LanguageChoice, SafetyNotice, Main };

// True only for a stored boolean true. A missing, damaged or foreign value reads as not confirmed,
// so the notice is shown again.
bool noticeConfirmed(const QVariant &stored);

// The page to show at the start. A language that is not shipped counts as not chosen. Changing the
// language later does not matter here: the confirmation is kept apart from the language.
Page firstPage(bool dataReady, const QString &storedLanguage, bool noticeConfirmed);

// The name of the page as the interface uses it: "error", "language", "notice" or "main".
QString pageName(Page page);

}

#endif

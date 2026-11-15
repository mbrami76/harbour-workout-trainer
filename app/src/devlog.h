// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef DEVLOG_H
#define DEVLOG_H

#ifdef FAKE_SPEECH

#include <QDateTime>
#include <QDebug>
#include <QString>

// The development build with the fake engine reports what is spoken and the events of the session in
// the system log, with the time of each line in milliseconds. It is not part of the package build.
inline void devLog(const QString &text)
{
    qInfo("speech %lld %s", QDateTime::currentMSecsSinceEpoch(), qPrintable(text));
}

#endif

#endif

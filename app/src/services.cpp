// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "services.h"

#include <QDir>
#include <QFileInfo>

#include "starterloader.h"

Services::Services(const QString &dataDirectory, const QString &starterDirectory, QObject *parent)
    : QObject(parent),
      starterDirectory_(starterDirectory),
      exercises_(&database_),
      workouts_(&database_),
      metronomes_(&database_),
      sessionLog_(&database_),
      timeAttack_(&database_)
{
    // The folder must exist and take writes before the database is touched: a read-only folder
    // would let an existing database open and then fail on the first save (R-NFR-3).
    QDir dir(dataDirectory);
    if (!dir.mkpath(QStringLiteral(".")) || !QFileInfo(dir.absolutePath()).isWritable()) {
        error_ = QStringLiteral("data-folder");
        return;
    }

    // Opening removes stale temporary files and migrates the schema (R-NFR-3, R-HST-4).
    if (!database_.open(dir.absolutePath())) {
        error_ = QStringLiteral("database");
        return;
    }
}

namespace {

void report(const char *what, const QList<ImportIssue> &problems)
{
    for (const ImportIssue &issue : problems)
        qWarning("Starter %s: %s [%s]", what, qPrintable(issue.text()), qPrintable(issue.location));
}

}

void Services::loadStarters(const QString &language)
{
    if (!isReady())
        return;
    const StarterLoader::Result result = StarterLoader::apply(starterDirectory_, language, &database_);
    report("library", result.problems);
    report("library", result.notices);
    if (result.changed())
        emit startersLoaded();
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QDir>
#include <QFile>
#include <QSet>
#include <QtTest>

#include "icons.h"

// The workout icons the picker offers and the files that carry them (R-WRK-12).
class TstWorkoutIcons : public QObject
{
    Q_OBJECT

private slots:
    void defaultComesFirst();
    void keysAreUnique();
    void keysEqualTheFilesOfTheSourceTree();
    void everyPictureIsInstalled();
};

static QDir iconDir() { return QDir(QStringLiteral(SOURCE_ROOT "/data/workout-icons")); }

void TstWorkoutIcons::defaultComesFirst()
{
    const QStringList keys = Icons::workoutKeys();
    QVERIFY(!keys.isEmpty());
    QVERIFY(keys.first().isEmpty());
}

void TstWorkoutIcons::keysAreUnique()
{
    const QStringList keys = Icons::workoutKeys();
    QCOMPARE(QSet<QString>::fromList(keys).size(), keys.size());
}

void TstWorkoutIcons::keysEqualTheFilesOfTheSourceTree()
{
    QStringList offered = Icons::workoutKeys();
    offered.removeFirst();
    offered.sort();
    const char *const suffixes[] = {"png", "svg"};
    for (const char *suffix : suffixes) {
        QStringList files;
        const QStringList names = iconDir().entryList(QStringList() << QStringLiteral("*.") + QLatin1String(suffix));
        for (const QString &name : names)
            files << name.left(name.length() - 4);
        files.sort();
        QCOMPARE(files, offered);
    }
}

void TstWorkoutIcons::everyPictureIsInstalled()
{
    // The project file installs every PNG of the folder by pattern, so every offered key is installed.
    QFile pro(QStringLiteral(SOURCE_ROOT "/app/app.pro"));
    QVERIFY(pro.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(pro.readAll());
    QVERIFY(text.contains(QStringLiteral("workouticons.files = $$PWD/../data/workout-icons/*.png")));
    QVERIFY(text.contains(QStringLiteral("workouticons.path = /usr/share/$${TARGET}/workout-icons")));
    QVERIFY(text.contains(QStringLiteral("INSTALLS += workouticons")));
}

QTEST_APPLESS_MAIN(TstWorkoutIcons)

#include "tst_workouticons.moc"

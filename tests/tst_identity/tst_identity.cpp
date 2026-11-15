// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>
#include <algorithm>

#include "identity.h"

class TstIdentity : public QObject
{
    Q_OBJECT

private slots:
    void copyNumbersFollowThePlainCopy();
    void otherNamesSortWithoutCase();
};

static QStringList sorted(QStringList names)
{
    std::sort(names.begin(), names.end(), [](const QString &a, const QString &b) {
        return Identity::compareNames(a, b) < 0;
    });
    return names;
}

void TstIdentity::copyNumbersFollowThePlainCopy()
{
    const QStringList expected = QStringList() << "x" << "x (copy)" << "x (copy 2)" << "x (copy 3)" << "x (copy 10)"
                                               << "x (copy 11)" << "xa";
    QCOMPARE(sorted(QStringList() << "x (copy 10)" << "xa" << "x (copy 2)" << "x (copy 11)" << "x" << "x (copy 3)"
                                  << "x (copy)"), expected);
    // The base is compared without case.
    QCOMPARE(sorted(QStringList() << "X (copy 10)" << "x (copy 2)"), QStringList() << "x (copy 2)" << "X (copy 10)");
}

void TstIdentity::otherNamesSortWithoutCase()
{
    QCOMPARE(sorted(QStringList() << "b" << "A" << "a1" << "B2"), QStringList() << "A" << "a1" << "b" << "B2");
    QVERIFY(Identity::compareNames("Abc", "abc") == 0);
}

QTEST_APPLESS_MAIN(TstIdentity)

#include "tst_identity.moc"

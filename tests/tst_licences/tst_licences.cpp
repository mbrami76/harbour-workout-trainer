// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QtTest>

#include "licences.h"

namespace {

const QString root = QStringLiteral(SOURCE_ROOT);

QString notices()
{
    QString text;
    Licences::readText(root + QStringLiteral("/third-party/") + Licences::noticesFileName(), 1000000, &text);
    return text;
}

}

class TstLicences : public QObject
{
    Q_OBJECT

private slots:
    void parsesAFakeNoticesFile();
    void aTextWithoutATableIsNotValid();
    void shippedNoticesHaveEverySection();
    void everyFileNamedByTheNoticesExists();
    void everyShippedLicenceFileIsNamedByTheNotices();
    void theFontLicenceExists();
    void textFilesAreSortedAndWithoutTheNotices();
    void textFilesOfAMissingFolderAreEmpty();
    void readTextCutsALongFile();
    void readTextOfAMissingFileFails();
    void paragraphsSplitAtBlankLines();
    void bundledTextsAreTheAppLicenceAndTheAssetNotices();
    void everyBundledTextExistsInTheSourceTree();
    void everyBundledTextIsInstalledByTheProjectFile();
    void installedBundledTextsAreExactlyTheFilesThere();
    void installedBundledTextsOfAMissingFolderAreEmpty();
    void shippedNoticesNameTheStarterData();
};

void TstLicences::parsesAFakeNoticesFile()
{
    const QString md = QStringLiteral(
        "# Notices\n\nIntro.\n\n"
        "| Component | Version | Licence | Text |\n|---|---|---|---|\n"
        "| Alpha lib | 1.0 | MIT | alpha-MIT.txt |\n"
        "| Beta (static) | as in Alpha | GPL-3.0-or-later; some BSD | beta-GPL.txt, beta-BSD.txt |\n\n"
        "## Source code\n\n- ignored: http://example.invalid\n\n"
        "## Voices\n\nFirst `voice` line\nsecond line.\n\nSecond paragraph.\n\n"
        "## Bundled assets of the project\n\n- Font **X**, OFL: `fonts/X.txt`.\n- Sounds: CC0.\n");
    const Licences::Notices n = Licences::parseNotices(md);
    QVERIFY(n.valid);
    QCOMPARE(n.components.size(), 2);
    QCOMPARE(n.components.at(0).name, QStringLiteral("Alpha lib"));
    QCOMPARE(n.components.at(0).licence, QStringLiteral("MIT"));
    QCOMPARE(n.components.at(0).files, QStringList() << "alpha-MIT.txt");
    QCOMPARE(n.components.at(1).licence, QStringLiteral("GPL-3.0-or-later; some BSD"));
    QCOMPARE(n.components.at(1).files, QStringList() << "beta-GPL.txt" << "beta-BSD.txt");
    QCOMPARE(n.voices, QStringList() << "First voice line second line." << "Second paragraph.");
    QCOMPARE(n.assets, QStringList() << "Font X, OFL: fonts/X.txt." << "Sounds: CC0.");
}

void TstLicences::aTextWithoutATableIsNotValid()
{
    QVERIFY(!Licences::parseNotices(QString()).valid);
    QVERIFY(!Licences::parseNotices(QStringLiteral("# Title\n\nNo table here.\n")).valid);
}

void TstLicences::shippedNoticesHaveEverySection()
{
    const Licences::Notices n = Licences::parseNotices(notices());
    QVERIFY(n.valid);
    QVERIFY(n.components.size() >= 10);
    for (const Licences::Component &c : n.components) {
        QVERIFY2(!c.name.isEmpty() && !c.licence.isEmpty() && !c.files.isEmpty(), qPrintable(c.name));
    }
    QVERIFY(n.voices.size() >= 2);
    QVERIFY(n.assets.size() >= 4);
}

void TstLicences::everyFileNamedByTheNoticesExists()
{
    const Licences::Notices n = Licences::parseNotices(notices());
    for (const Licences::Component &c : n.components) {
        for (const QString &file : c.files)
            QVERIFY2(QFileInfo(root + QStringLiteral("/third-party/") + file).isFile(), qPrintable(file));
    }
}

void TstLicences::everyShippedLicenceFileIsNamedByTheNotices()
{
    // The page lists the folder; the table names the licence text of each component. A file that no
    // line of the notices mentions would be listed without a reason.
    const QString text = notices();
    for (const QString &file : Licences::textFiles(root + QStringLiteral("/third-party")))
        QVERIFY2(text.contains(file), qPrintable(file));
}

void TstLicences::theFontLicenceExists()
{
    QVERIFY(QFileInfo(root + QStringLiteral("/app/fonts/DSEG-LICENSE.txt")).isFile());
}

void TstLicences::textFilesAreSortedAndWithoutTheNotices()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    for (const char *name : { "b.txt", "A.txt", "THIRD-PARTY-NOTICES.md", "c.md" }) {
        QFile f(dir.path() + QLatin1Char('/') + QString::fromLatin1(name));
        QVERIFY(f.open(QIODevice::WriteOnly));
    }
    QVERIFY(QDir(dir.path()).mkdir(QStringLiteral("sub")));
    QCOMPARE(Licences::textFiles(dir.path()), QStringList() << "A.txt" << "b.txt" << "c.md");
}

void TstLicences::textFilesOfAMissingFolderAreEmpty()
{
    QVERIFY(Licences::textFiles(QStringLiteral("/nonexistent-folder-for-test")).isEmpty());
}

void TstLicences::readTextCutsALongFile()
{
    QTemporaryDir dir;
    QFile f(dir.path() + QStringLiteral("/long.txt"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("one\r\ntwo\r\n" + QByteArray(100, 'x'));
    f.close();
    QString text;
    bool cut = false;
    QVERIFY(Licences::readText(f.fileName(), 1000, &text, &cut));
    QVERIFY(!cut);
    QVERIFY(text.startsWith(QStringLiteral("one\ntwo\n")));
    QVERIFY(Licences::readText(f.fileName(), 20, &text, &cut));
    QVERIFY(cut);
    QCOMPARE(text, QStringLiteral("one\ntwo\nxxxxxxxxxx"));
}

void TstLicences::readTextOfAMissingFileFails()
{
    QString text;
    QVERIFY(!Licences::readText(QStringLiteral("/nonexistent-file-for-test"), 100, &text));
}

void TstLicences::paragraphsSplitAtBlankLines()
{
    QCOMPARE(Licences::paragraphs(QStringLiteral("a\nb\n\n\n  \nc\n")), QStringList() << "a\nb" << "c");
    // A filled line is joined with the next unindented one; an indented line and a short line are not.
    const QString filled(60, QLatin1Char('x'));
    QCOMPARE(Licences::paragraphs(filled + "\nnext\n  indented\nshort\nlast"),
             QStringList() << filled + " next\n  indented\nshort\nlast");
    QVERIFY(Licences::paragraphs(QString()).isEmpty());
}

void TstLicences::bundledTextsAreTheAppLicenceAndTheAssetNotices()
{
    QStringList ids;
    int app = 0;
    for (const Licences::BundledText &t : Licences::bundledTexts()) {
        ids.append(t.id);
        app += t.app ? 1 : 0;
    }
    QCOMPARE(app, 1);
    QCOMPARE(ids, QStringList() << "LICENSE" << "exercise-icons/README.md" << "mode-icons/README.md"
                                << "muscle-groups/README.md" << "workout-icons/README.md"
                                << "sounds/README.md" << "starter/README.md");
}

void TstLicences::everyBundledTextExistsInTheSourceTree()
{
    for (const Licences::BundledText &t : Licences::bundledTexts())
        QVERIFY2(QFileInfo(root + QLatin1Char('/') + t.source).isFile(), qPrintable(t.source));
}

void TstLicences::everyBundledTextIsInstalledByTheProjectFile()
{
    QFile pro(root + QStringLiteral("/app/app.pro"));
    QVERIFY(pro.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(pro.readAll());
    QVERIFY(text.contains(QStringLiteral("$$PWD/../LICENSE")));
    for (const Licences::BundledText &t : Licences::bundledTexts()) {
        if (!t.app)
            QVERIFY2(text.contains(QStringLiteral("$$PWD/../") + t.source), qPrintable(t.source));
    }
}

void TstLicences::installedBundledTextsAreExactlyTheFilesThere()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // Only some of the files exist, and one more file that is not on the list.
    for (const char *name : { "LICENSE", "sounds/README.md", "starter/README.md", "other/README.md" }) {
        const QString path = dir.path() + QLatin1Char('/') + QString::fromLatin1(name);
        QVERIFY(QDir().mkpath(QFileInfo(path).path()));
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
    }
    // A folder with a listed name is not a text.
    QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("mode-icons/README.md")));
    QStringList ids;
    for (const Licences::BundledText &t : Licences::installedBundledTexts(dir.path()))
        ids.append(t.id);
    QCOMPARE(ids, QStringList() << "LICENSE" << "sounds/README.md" << "starter/README.md");
}

void TstLicences::installedBundledTextsOfAMissingFolderAreEmpty()
{
    QVERIFY(Licences::installedBundledTexts(QStringLiteral("/nonexistent-folder-for-test")).isEmpty());
}

void TstLicences::shippedNoticesNameTheStarterData()
{
    const Licences::Notices n = Licences::parseNotices(notices());
    bool found = false;
    for (const QString &line : n.assets)
        found = found || (line.startsWith(QStringLiteral("Starter data")) && line.contains(QStringLiteral("CC0 1.0")));
    QVERIFY(found);
}

QTEST_APPLESS_MAIN(TstLicences)

#include "tst_licences.moc"

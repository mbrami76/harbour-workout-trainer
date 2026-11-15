// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef LICENCES_H
#define LICENCES_H

#include <QList>
#include <QString>
#include <QStringList>

// What the Licences page shows, read from the notices file that is installed with the app. Nothing of
// the file is copied into the program: the page reads it at run time and shows what is there.
namespace Licences {

// One row of the components table: the name, the version, the licence names and the files that hold
// the licence texts (names relative to the folder of the notices).
struct Component {
    QString name;
    QString version;
    QString licence;
    QStringList files;
};

struct Notices {
    // False when the text has no components table.
    bool valid = false;
    QList<Component> components;
    // The paragraphs of the section "Voices", each on one line, markup removed.
    QStringList voices;
    // The lines of the section "Bundled assets", one per list entry, markup removed.
    QStringList assets;
};

// Reads the notices file (Markdown): the table of components, the section "Voices" and the section
// whose heading starts with "Bundled assets". Other sections are ignored.
Notices parseNotices(const QString &markdown);

// Every file that holds a licence text of the folder, sorted by name; the notices file itself is not
// one. Empty when the folder does not exist.
QStringList textFiles(const QString &directory);

// One licence text that is installed with the app, outside the folder of the notices.
struct BundledText {
    // Path inside the folder the app is installed in; it is also the name the page shows.
    QString id;
    // Where the file is in the source tree, relative to its root.
    QString source;
    // True for the licence of the app itself, false for the notice of a bundled asset.
    bool app;
};

// Every text that the package installs beside the notices, in the order the page offers them.
QList<BundledText> bundledTexts();

// The entries of bundledTexts() whose file exists in the installed folder.
QList<BundledText> installedBundledTexts(const QString &shareDirectory);

// The name of the notices file inside the folder of the licence texts.
QString noticesFileName();

// The file read as UTF-8 text with line ends as "\n", at most maxBytes of it (a longer file is cut
// and *cut is set). False when the file cannot be read.
bool readText(const QString &path, qint64 maxBytes, QString *text, bool *cut = nullptr);

// The text split into paragraphs at blank lines. Lines that were filled up to the margin and are
// followed by an unindented line are joined with a space; the other line breaks stay.
QStringList paragraphs(const QString &text);

}

#endif

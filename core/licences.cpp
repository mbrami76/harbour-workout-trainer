// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "licences.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

namespace {

const char noticesName[] = "THIRD-PARTY-NOTICES.md";

// Emphasis and code marks of the Markdown text taken out; the words stay.
QString plain(QString text)
{
    text.remove(QLatin1Char('`'));
    text.replace(QStringLiteral("**"), QString());
    return text.trimmed();
}

QStringList cellsOf(const QString &line)
{
    QString inner = line.trimmed();
    if (inner.startsWith(QLatin1Char('|')))
        inner.remove(0, 1);
    if (inner.endsWith(QLatin1Char('|')))
        inner.chop(1);
    QStringList cells;
    for (const QString &cell : inner.split(QLatin1Char('|')))
        cells.append(cell.trimmed());
    return cells;
}

bool isSeparatorRow(const QStringList &cells)
{
    static const QRegularExpression dashes(QStringLiteral("^:?-+:?$"));
    for (const QString &cell : cells) {
        if (!dashes.match(cell).hasMatch())
            return false;
    }
    return !cells.isEmpty();
}

}

QList<Licences::BundledText> Licences::bundledTexts()
{
    return QList<BundledText>()
        << BundledText { QStringLiteral("LICENSE"), QStringLiteral("LICENSE"), true }
        << BundledText { QStringLiteral("exercise-icons/README.md"), QStringLiteral("data/exercise-icons/README.md"), false }
        << BundledText { QStringLiteral("mode-icons/README.md"), QStringLiteral("data/mode-icons/README.md"), false }
        << BundledText { QStringLiteral("muscle-groups/README.md"), QStringLiteral("data/muscle-groups/README.md"), false }
        << BundledText { QStringLiteral("workout-icons/README.md"), QStringLiteral("data/workout-icons/README.md"), false }
        << BundledText { QStringLiteral("sounds/README.md"), QStringLiteral("data/sounds/README.md"), false }
        << BundledText { QStringLiteral("starter/README.md"), QStringLiteral("data/starter/README.md"), false };
}

QList<Licences::BundledText> Licences::installedBundledTexts(const QString &shareDirectory)
{
    QList<BundledText> installed;
    for (const BundledText &text : bundledTexts()) {
        if (QFileInfo(shareDirectory + QLatin1Char('/') + text.id).isFile())
            installed.append(text);
    }
    return installed;
}

QString Licences::noticesFileName()
{
    return QLatin1String(noticesName);
}

Licences::Notices Licences::parseNotices(const QString &markdown)
{
    Notices notices;
    enum Section { Other, Voices, Assets } section = Other;
    bool inTable = false;
    int headerRows = 0;
    QStringList paragraph;

    auto endParagraph = [&]() {
        if (!paragraph.isEmpty() && section == Voices)
            notices.voices.append(plain(paragraph.join(QLatin1Char(' '))));
        paragraph.clear();
    };

    const QStringList lines = QString(markdown).replace(QStringLiteral("\r\n"), QStringLiteral("\n")).split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        if (line.startsWith(QStringLiteral("## "))) {
            endParagraph();
            const QString heading = line.mid(3).trimmed();
            section = heading == QLatin1String("Voices") ? Voices
                      : (heading.startsWith(QStringLiteral("Bundled assets")) ? Assets : Other);
            inTable = false;
            continue;
        }
        if (line.startsWith(QLatin1Char('|'))) {
            // The table of components is the first table of the file, before any section.
            if (section != Other || notices.valid)
                continue;
            const QStringList cells = cellsOf(line);
            if (!inTable) {
                inTable = true;
                headerRows = 1;
                continue;
            }
            if (headerRows == 1 && isSeparatorRow(cells)) {
                headerRows = 2;
                continue;
            }
            if (headerRows < 2 || cells.size() < 4)
                continue;
            Component c;
            c.name = plain(cells.at(0));
            c.version = plain(cells.at(1));
            c.licence = plain(cells.at(2));
            for (const QString &file : cells.at(3).split(QLatin1Char(','), QString::SkipEmptyParts))
                c.files.append(plain(file));
            notices.components.append(c);
            continue;
        }
        if (inTable) {
            inTable = false;
            notices.valid = !notices.components.isEmpty();
        }
        if (section == Voices) {
            if (line.trimmed().isEmpty())
                endParagraph();
            else
                paragraph.append(line.trimmed());
        } else if (section == Assets && line.startsWith(QStringLiteral("- "))) {
            notices.assets.append(plain(line.mid(2)));
        }
    }
    endParagraph();
    if (!notices.valid)
        notices.valid = !notices.components.isEmpty();
    return notices;
}

QStringList Licences::textFiles(const QString &directory)
{
    QStringList names = QDir(directory).entryList(QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase);
    names.removeAll(noticesFileName());
    return names;
}

bool Licences::readText(const QString &path, qint64 maxBytes, QString *text, bool *cut)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QByteArray bytes = file.read(maxBytes + 1);
    const bool tooLong = bytes.size() > maxBytes;
    if (tooLong)
        bytes.truncate(int(maxBytes));
    if (cut)
        *cut = tooLong;
    *text = QString::fromUtf8(bytes).replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    return true;
}

QStringList Licences::paragraphs(const QString &text)
{
    // A text wrapped at about 80 characters is joined again where a line was filled up and the next
    // one does not start with blank space (an indent marks a list entry or a heading), so that the
    // screen wraps it to its own width. Short lines and indented lines keep their breaks.
    const int filledLine = 55;
    QStringList result;
    QString current;
    QString previous;
    auto flush = [&]() {
        if (!current.isEmpty())
            result.append(current);
        current.clear();
        previous.clear();
    };
    for (const QString &line : text.split(QLatin1Char('\n'))) {
        if (line.trimmed().isEmpty()) {
            flush();
            continue;
        }
        if (current.isEmpty()) {
            current = line;
        } else {
            const bool join = previous.size() >= filledLine && !line.at(0).isSpace();
            current += (join ? QStringLiteral(" ") : QStringLiteral("\n")) + line;
        }
        previous = line;
    }
    flush();
    return result;
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "licencesbridge.h"

#include <QDir>
#include <QFileInfo>

#include "licences.h"

namespace {

const qint64 maxTextBytes = 2 * 1024 * 1024;
const char fontId[] = "fonts/DSEG-LICENSE.txt";
const char thirdPartyPrefix[] = "third-party/";

}

LicencesBridge::LicencesBridge(const QString &shareDirectory, QObject *parent)
    : QObject(parent), share_(QDir::cleanPath(shareDirectory))
{
}

QString LicencesBridge::thirdPartyDirectory() const
{
    return share_ + QStringLiteral("/third-party");
}

QString LicencesBridge::fontLicencePath() const
{
    return share_ + QLatin1Char('/') + QLatin1String(fontId);
}

QVariantMap LicencesBridge::notices() const
{
    QVariantMap map;
    QString markdown;
    const QString path = thirdPartyDirectory() + QLatin1Char('/') + Licences::noticesFileName();
    const Licences::Notices parsed = Licences::readText(path, maxTextBytes, &markdown) ? Licences::parseNotices(markdown)
                                                                                         : Licences::Notices();
    map.insert(QStringLiteral("available"), parsed.valid);
    QVariantList components;
    for (const Licences::Component &c : parsed.components) {
        QVariantMap row;
        row.insert(QStringLiteral("name"), c.name);
        row.insert(QStringLiteral("version"), c.version);
        row.insert(QStringLiteral("licence"), c.licence);
        components.append(row);
    }
    map.insert(QStringLiteral("components"), components);
    map.insert(QStringLiteral("voices"), QVariant(parsed.voices));
    map.insert(QStringLiteral("assets"), QVariant(parsed.assets));
    return map;
}

QVariantList LicencesBridge::texts() const
{
    QVariantList list;
    for (const Licences::BundledText &text : Licences::installedBundledTexts(share_)) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), text.id);
        row.insert(QStringLiteral("label"), text.id);
        row.insert(QStringLiteral("group"), QLatin1String(text.app ? "app" : "asset"));
        list.append(row);
    }
    for (const QString &file : Licences::textFiles(thirdPartyDirectory())) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), QLatin1String(thirdPartyPrefix) + file);
        row.insert(QStringLiteral("label"), file);
        row.insert(QStringLiteral("group"), QStringLiteral("text"));
        list.append(row);
    }
    if (QFileInfo(fontLicencePath()).isFile()) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), QLatin1String(fontId));
        row.insert(QStringLiteral("label"), QStringLiteral("DSEG-LICENSE.txt"));
        row.insert(QStringLiteral("group"), QStringLiteral("text"));
        list.append(row);
    }
    return list;
}

QVariantMap LicencesBridge::text(const QString &id) const
{
    QVariantMap result;
    result.insert(QStringLiteral("ok"), false);
    bool offered = false;
    for (const QVariant &row : texts())
        offered = offered || row.toMap().value(QStringLiteral("id")).toString() == id;
    if (!offered)
        return result;
    QString text;
    bool cut = false;
    if (!Licences::readText(share_ + QLatin1Char('/') + id, maxTextBytes, &text, &cut))
        return result;
    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("paragraphs"), QVariant(Licences::paragraphs(text)));
    result.insert(QStringLiteral("cut"), cut);
    return result;
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef LICENCESBRIDGE_H
#define LICENCESBRIDGE_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// What the Licences page shows, for QML. Exposed as the context property "licences". It reads the
// notices file and the licence texts installed next to the app, at run time; a file that is not
// installed (the speech components are built for the phone only) is simply not offered.
class LicencesBridge : public QObject
{
    Q_OBJECT

public:
    // shareDirectory is the folder the app is installed in (it holds "third-party" and "fonts").
    explicit LicencesBridge(const QString &shareDirectory, QObject *parent = nullptr);

    // {available, components: [{name, version, licence}], voices: [text], assets: [text]}; `available`
    // is false when the notices file is not installed.
    Q_INVOKABLE QVariantMap notices() const;

    // The licence texts that are installed, in the order they are offered: [{id, label, group}]; the
    // group is "app" (the licence of the app), "asset" (the notice of a bundled asset) or "text" (the
    // licence text of a component or of the digit font). The id is given back to text().
    Q_INVOKABLE QVariantList texts() const;

    // One of the texts of texts(), by id: {ok, paragraphs: [text], cut}. An id that is not offered is
    // refused ({ok: false}), so that no other file can be read through this call.
    Q_INVOKABLE QVariantMap text(const QString &id) const;

private:
    QString thirdPartyDirectory() const;
    QString fontLicencePath() const;

    QString share_;
};

#endif

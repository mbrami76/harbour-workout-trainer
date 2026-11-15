// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0

// One installed licence text as plain, scrollable text. A long text is shown paragraph by paragraph,
// so that only the visible part is laid out.
Page {
    id: page
    allowedOrientations: Orientation.All

    // The id the bridge offered, and the name shown in the header.
    property string textId: ""
    property string title: ""

    readonly property var content: licences.text(textId)

    SilicaListView {
        anchors.fill: parent
        model: page.content.ok ? page.content.paragraphs : []

        header: PageHeader { title: page.title }

        delegate: Label {
            x: Theme.horizontalPageMargin
            width: ListView.view.width - 2 * Theme.horizontalPageMargin
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
            bottomPadding: Theme.paddingLarge
            text: modelData
        }

        footer: Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            wrapMode: Text.Wrap
            color: Theme.secondaryColor
            topPadding: Theme.paddingLarge
            bottomPadding: Theme.paddingLarge
            visible: !page.content.ok || page.content.cut
            text: page.content.ok ? qsTr("The rest of this text is not shown.") : qsTr("This text cannot be read.")
        }

        VerticalScrollDecorator { }
    }
}

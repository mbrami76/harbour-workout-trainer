// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// The confirmation before a saved metronome is deleted (R-MET-8). Deleting is never refused: when
// exercises use the metronome the dialog names them and says that they keep all their data and only
// lose their tempo.
Dialog {
    id: dialog

    property string metronomeName
    // The exercises that use it: [{key, name}].
    property var users: []

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            DialogHeader {
                acceptText: qsTr("Delete")
                cancelText: qsTr("Cancel")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                text: dialog.users.length === 0
                      ? qsTr("Delete the rhythm \"%1\"? This cannot be undone.").arg(dialog.metronomeName)
                      : qsTr("%n exercise(s) use the rhythm \"%1\". If you delete it they keep all their data and only lose their rhythm. This cannot be undone.", "", dialog.users.length).arg(dialog.metronomeName)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: dialog.users.length > 0
                wrapMode: Text.Wrap
                textFormat: Text.PlainText
                color: Theme.primaryColor
                text: MetronomeText.names(dialog.users)
            }
        }

        VerticalScrollDecorator { }
    }
}

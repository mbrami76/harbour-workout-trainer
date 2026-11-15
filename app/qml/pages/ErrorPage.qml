// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0

// Shown instead of the main page when the data cannot be used. It has a fixed text per error code
// and touches nothing.
Page {
    // The code of services.error: "data-folder" or "database".
    property string code

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader { title: qsTr("Cannot start") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                text: code === "data-folder"
                      ? qsTr("The folder where the app keeps its data cannot be used. Nothing was changed. Check the free space on the device, then start the app again.")
                      : qsTr("The database of the app cannot be opened. It may have been written by a newer version of the app. Nothing was changed.")
            }
        }

        VerticalScrollDecorator { }
    }
}

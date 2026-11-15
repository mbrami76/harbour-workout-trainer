// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// The result of an export (R-IMP-1): the folder and the name of the file in plain words, or the
// reason it failed. The app has no network access, so it does not send the file anywhere: the file
// is in the Documents folder, where the file manager and other apps can reach it.
Page {
    id: page

    // The result map of the exchange bridge.
    property var result: ({})
    readonly property bool done: result.status === "ok"

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader { title: page.done ? qsTr("Exported") : qsTr("Not exported") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                text: page.done ? qsTr("%n workout(s) written to a file.", "", page.result.count)
                                : ExchangeText.exportFailure(page.result.status)
            }

            DetailItem {
                visible: page.done
                label: qsTr("File name")
                value: page.result.file === undefined ? "" : page.result.file
            }

            DetailItem {
                visible: page.done
                label: qsTr("Folder")
                value: page.result.folder === undefined ? "" : page.result.folder
            }

            Label {
                visible: page.done
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("The file is in the folder WorkoutTrainer of your Documents folder. Workout Trainer does not send files anywhere: copy it from there with the file manager, or attach it in another app. To load it again, use Import workouts on the first page.")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                preferredWidth: Theme.buttonWidthLarge
                height: Math.max(implicitHeight, Metrics.mm(9))
                text: qsTr("Close")
                onClicked: pageStack.pop()
            }
        }

        VerticalScrollDecorator { }
    }
}

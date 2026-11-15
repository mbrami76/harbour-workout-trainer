// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader { title: qsTr("About") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                font.pixelSize: Theme.fontSizeExtraLarge
                color: Theme.highlightColor
                text: qsTr("Workout Trainer")
            }

            DetailItem {
                label: qsTr("Version")
                value: Qt.application.version
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                text: qsTr("Free software, licensed under the GNU General Public License, version 3 or later.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                text: qsTr("Source code: %1").arg("https://github.com/mbrami76/harbour-workout-trainer")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                text: qsTr("The app works offline and keeps all its data on this device.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                text: qsTr("The seven-segment digits use the font DSEG7 Classic by keshikan, licensed under the SIL Open Font License 1.1. The licence text is installed with the app, in the folder fonts, as DSEG-LICENSE.txt.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                text: qsTr("The licences of the app and of the components it includes are on the Licences page.")
            }

            SafetyNotice { width: parent.width }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Licences")
                onClicked: pageStack.push(Qt.resolvedUrl("LicencesPage.qml"))
            }
        }

        VerticalScrollDecorator { }
    }
}

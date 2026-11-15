// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// Shown once at the first start, after the language choice and before anything else (R-GEN-5).
// The page cannot be left without the confirmation; the text scrolls, and the button follows it,
// so it stays reachable at any font size and in either orientation.
Page {
    allowedOrientations: Orientation.All
    backNavigation: false
    forwardNavigation: false
    showNavigationIndicator: false

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            Item { width: 1; height: Theme.paddingLarge }

            SafetyNotice { width: parent.width }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("I have read this")
                onClicked: {
                    Settings.setSafetyNoticeRead()
                    pageStack.replace(Qt.resolvedUrl("MainPage.qml"))
                }
            }
        }

        VerticalScrollDecorator { }
    }
}

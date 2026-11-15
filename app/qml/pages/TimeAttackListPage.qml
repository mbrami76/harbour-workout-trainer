// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// The challenges of the time attack mode (R-MOD-4): name, number of results and best time.
Page {
    id: page

    allowedOrientations: Orientation.All

    Component.onCompleted: timeAttackBridge.refresh()
    // Back from a challenge: the figures may have changed.
    onStatusChanged: {
        if (status === PageStatus.Active)
            timeAttackBridge.refresh()
    }

    SilicaListView {
        anchors.fill: parent
        model: timeAttackBridge.challenges

        PullDownMenu {
            MenuItem {
                text: qsTr("New challenge")
                onClicked: pageStack.push(Qt.resolvedUrl("NewChallengePage.qml"))
            }
        }

        header: PageHeader { title: qsTr("Time attack") }

        delegate: BackgroundItem {
            width: ListView.view.width
            height: Math.max(Theme.itemSizeExtraLarge, column.height + 2 * Theme.paddingLarge)
            onClicked: {
                if (timeAttackBridge.loadChallenge(modelData.id))
                    pageStack.push(Qt.resolvedUrl("ChallengePage.qml"))
            }

            Column {
                id: column
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                Label {
                    width: parent.width
                    text: modelData.name
                    font.pixelSize: Theme.fontSizeLarge
                    color: highlighted ? Theme.highlightColor : Theme.primaryColor
                    truncationMode: TruncationMode.Fade
                }
                Label {
                    width: parent.width
                    font.pixelSize: Theme.fontSizeSmall
                    color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    text: modelData.count > 0
                          ? qsTr("%n result(s), best %1", "", modelData.count).arg(timeFormatter.precise(modelData.bestMs))
                          : qsTr("No results yet")
                }
            }
        }

        ViewPlaceholder {
            enabled: timeAttackBridge.challenges.length === 0
            text: qsTr("No challenges yet")
            hintText: qsTr("Pull down to create one")
        }

        VerticalScrollDecorator { }
    }
}

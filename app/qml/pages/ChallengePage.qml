// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// One challenge (R-MOD-4): best, last and average time, the results with their dates, newest first,
// the start of a run, and the deletion of a result or of the challenge (with a confirmation).
Page {
    id: page

    readonly property var challenge: timeAttackBridge.challenge
    readonly property bool gone: challenge.id === undefined

    allowedOrientations: Orientation.All

    // After the deletion of the challenge the page has nothing to show: go back to the list.
    onStatusChanged: {
        if (status === PageStatus.Active && gone)
            pageStack.pop()
    }

    SilicaListView {
        id: list
        anchors.fill: parent
        model: timeAttackBridge.results

        PullDownMenu {
            MenuItem {
                text: qsTr("Delete challenge")
                onClicked: {
                    var dialog = pageStack.push(Qt.resolvedUrl("DeleteChallengeDialog.qml"),
                                                { challengeName: page.challenge.name })
                    dialog.accepted.connect(function () { timeAttackBridge.deleteChallenge(page.challenge.id) })
                }
            }
        }

        header: Column {
            width: list.width

            PageHeader { title: page.gone ? "" : page.challenge.name }

            DetailItem {
                label: qsTr("Results")
                value: page.gone ? "" : String(page.challenge.count)
            }
            DetailItem {
                label: qsTr("Best")
                value: page.gone || page.challenge.count === 0 ? "-" : timeFormatter.precise(page.challenge.bestMs)
            }
            DetailItem {
                label: qsTr("Last")
                value: page.gone || page.challenge.count === 0 ? "-" : timeFormatter.precise(page.challenge.lastMs)
            }
            DetailItem {
                label: qsTr("Average")
                value: page.gone || page.challenge.count === 0 ? "-" : timeFormatter.precise(page.challenge.averageMs)
            }

            BigButton {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Start a run")
                onClicked: pageStack.push(Qt.resolvedUrl("TimeAttackRunPage.qml"))
            }

            SectionHeader { text: qsTr("Results") }
        }

        delegate: ListItem {
            id: item
            contentHeight: Theme.itemSizeMedium
            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Delete")
                    onClicked: item.remorseAction(qsTr("Deleting"), function () {
                        timeAttackBridge.deleteResult(modelData.id)
                    })
                }
            }

            Label {
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                font.pixelSize: Theme.fontSizeLarge
                text: timeFormatter.precise(modelData.durationMs)
            }
            Label {
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                color: Theme.secondaryColor
                text: Qt.formatDateTime(new Date(modelData.finishedAt * 1000))
            }
        }

        ViewPlaceholder {
            enabled: list.count === 0
            text: qsTr("No results yet")
        }

        VerticalScrollDecorator { }
    }
}

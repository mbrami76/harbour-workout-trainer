// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// The Workouts mode opens here (R-MOD-1, R-WRK-1, R-TPL-1): "New workout" first, then the saved
// workouts, favourites first and then by name, each with its icon on a disc of its colour, a line
// about its structure and planned time and a star when it is a favourite. A tap opens the workout;
// a press and hold opens a menu to edit, duplicate, mark as favourite or delete it. The primary
// action rests at the bottom (R-UI-13).
Page {
    id: page

    allowedOrientations: Orientation.All

    // Said under the list after a duplicate, a delete or a change of favourite.
    property string notice

    function open(w) {
        notice = ""
        pageStack.push(Qt.resolvedUrl("WorkoutPage.qml"), { workoutId: w.id })
    }

    function create() {
        notice = ""
        pageStack.push(Qt.resolvedUrl("WorkoutBuilderPage.qml"), { workoutId: 0 })
    }

    function edit(w) {
        notice = ""
        pageStack.push(Qt.resolvedUrl("WorkoutBuilderPage.qml"), { workoutId: w.id })
    }

    function duplicate(w) {
        var r = workoutLibrary.duplicate(w.id)
        notice = r.status === "ok" ? qsTr("Created \"%1\".").arg(r.name) : qsTr("The workout could not be copied.")
    }

    function toggleFavorite(w) {
        if (workoutLibrary.setFavorite(w.id, !w.favorite).status !== "ok")
            notice = qsTr("The workout could not be changed.")
        else
            notice = ""
    }

    function confirmDelete(w) {
        var dialog = pageStack.push(Qt.resolvedUrl("DeleteWorkoutDialog.qml"), { workoutName: w.name })
        dialog.accepted.connect(function () {
            notice = workoutLibrary.remove(w.id).status === "ok"
                ? qsTr("Deleted \"%1\".").arg(w.name)
                : qsTr("The workout could not be deleted.")
        })
    }

    function exportWorkout(w) {
        pageStack.push(Qt.resolvedUrl("ExportResultPage.qml"), { result: exchangeBridge.exportWorkout(w.id) })
    }

    Component.onCompleted: workoutLibrary.refresh()
    onStatusChanged: {
        if (status === PageStatus.Active)
            workoutLibrary.refresh()
    }

    SilicaListView {
        id: list
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: bar.top
        }
        clip: true
        model: workoutLibrary.workouts
        cacheBuffer: height

        header: Column {
            width: list.width

            PageHeader { title: qsTr("Workouts") }

            BackgroundItem {
                width: parent.width
                height: Math.max(Theme.itemSizeLarge, Metrics.mm(9))
                onClicked: page.create()
                Accessible.role: Accessible.Button
                Accessible.name: qsTr("New workout")

                Column {
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    Label {
                        width: parent.width
                        text: qsTr("New workout")
                        font.pixelSize: Theme.fontSizeLarge
                        color: highlighted ? Theme.highlightColor : Theme.primaryColor
                    }
                    Label {
                        width: parent.width
                        text: qsTr("Build a workout from blocks and steps")
                        font.pixelSize: Theme.fontSizeSmall
                        color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                        truncationMode: TruncationMode.Fade
                    }
                }
            }

            SectionHeader { text: qsTr("Saved workouts") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: visible ? implicitHeight + Theme.paddingLarge : 0
                visible: workoutLibrary.workouts.length === 0
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
                text: qsTr("No workouts yet. Use New workout to build one.")
            }
        }

        footer: Label {
            x: Theme.horizontalPageMargin
            width: list.width - 2 * Theme.horizontalPageMargin
            height: visible ? implicitHeight + 2 * Theme.paddingLarge : 0
            visible: page.notice !== ""
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            textFormat: Text.PlainText
            color: Theme.highlightColor
            text: page.notice
        }

        delegate: ListItem {
            id: item
            width: ListView.view.width
            contentHeight: Math.max(Theme.itemSizeExtraLarge, Metrics.mm(11) + 2 * Theme.paddingMedium)
            onClicked: page.open(modelData)

            Accessible.role: Accessible.Button
            Accessible.name: modelData.favorite ? qsTr("%1, favourite").arg(modelData.name) : modelData.name

            IconBadge {
                id: badge
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                iconKey: modelData.icon
                iconSet: "workout"
                colourKey: modelData.colour
            }

            Glyph {
                id: star
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                width: visible ? Metrics.mm(5) : 0
                height: Metrics.mm(5)
                visible: modelData.favorite
                kind: "star"
                color: Theme.highlightColor
            }

            Column {
                anchors {
                    left: badge.right
                    leftMargin: Theme.paddingLarge
                    right: star.left
                    rightMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    width: parent.width
                    text: modelData.name
                    textFormat: Text.PlainText
                    font.pixelSize: Theme.fontSizeMedium
                    color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                    truncationMode: TruncationMode.Fade
                }
                Label {
                    width: parent.width
                    text: WorkoutText.listSummary(modelData)
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                }
            }

            menu: Component {
                ContextMenu {
                    MenuItem {
                        text: qsTr("Edit")
                        onClicked: page.edit(modelData)
                    }
                    MenuItem {
                        text: qsTr("Duplicate")
                        onClicked: page.duplicate(modelData)
                    }
                    MenuItem {
                        text: modelData.favorite ? qsTr("Remove from favourites") : qsTr("Mark as favourite")
                        onClicked: page.toggleFavorite(modelData)
                    }
                    MenuItem {
                        text: qsTr("Export workout")
                        onClicked: page.exportWorkout(modelData)
                    }
                    MenuItem {
                        text: qsTr("Delete")
                        onClicked: page.confirmDelete(modelData)
                    }
                }
            }
        }

        VerticalScrollDecorator { }
    }

    // The primary action rests at the bottom, where the thumb reaches first (R-UI-13).
    Item {
        id: bar
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: newButton.height + 2 * Theme.paddingMedium

        BigButton {
            id: newButton
            anchors {
                left: parent.left
                right: parent.right
                margins: Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }
            text: qsTr("New workout")
            onActivated: page.create()
        }
    }
}

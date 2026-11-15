// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// Asked before a change to an exercise that workouts use is saved (R-EXR-10): which workouts the
// change reaches, and three ways out. `choice` carries "copy" (save the change as a new exercise,
// the workouts stay as they are), "anyway" (save the change; it applies to all of them) or
// "cancel". Leaving with the back gesture is a cancel.
Page {
    id: page

    property string exerciseName
    // The names of the workouts that use it.
    property var workouts: []

    signal choice(string what)

    allowedOrientations: Orientation.All

    function choose(what) {
        choice(what)
        pageStack.pop()
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader { title: qsTr("Workouts use this exercise") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                textFormat: Text.PlainText
                color: Theme.highlightColor
                text: qsTr("%n workout(s) use the exercise \"%1\" and take the change if you save it:", "", page.workouts.length).arg(page.exerciseName)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                textFormat: Text.PlainText
                color: Theme.primaryColor
                text: page.workouts.join("\n")
            }

            BigButton {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Save as a copy")
                onActivated: page.choose("copy")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("The original and the workouts stay as they are.")
            }

            BigButton {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Save anyway")
                onActivated: page.choose("anyway")
            }

            BigButton {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Cancel")
                onActivated: page.choose("cancel")
            }
        }

        VerticalScrollDecorator { }
    }
}

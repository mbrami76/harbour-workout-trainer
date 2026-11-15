// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// The confirmation before an exercise is deleted (R-EXR-4). When workouts use the exercise the
// dialog names them and says what happens to their steps. The metronome of the exercise is never
// deleted.
Dialog {
    id: dialog

    property string exerciseName
    // The names of the workouts that use it.
    property var workouts: []

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
                textFormat: Text.PlainText
                color: Theme.highlightColor
                text: dialog.workouts.length === 0
                      ? qsTr("Delete the exercise \"%1\"? This cannot be undone.").arg(dialog.exerciseName)
                      : qsTr("%n workout(s) use the exercise \"%1\". If you delete it, its steps are removed from them. This cannot be undone.", "", dialog.workouts.length).arg(dialog.exerciseName)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: dialog.workouts.length > 0
                wrapMode: Text.Wrap
                textFormat: Text.PlainText
                color: Theme.primaryColor
                text: dialog.workouts.join("\n")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("Its metronome is not deleted.")
            }
        }

        VerticalScrollDecorator { }
    }
}

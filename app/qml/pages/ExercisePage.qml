// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// One exercise (R-EXR-1, R-EXR-8, R-EXR-10, R-MUS-1, R-MUS-2): its name and its file, as the session
// shows it too. The pull-down menu edits, clones or deletes it.
Page {
    id: page

    property string exerciseKey
    // Said after an export.
    property string notice
    // The exercise as a map; empty when it does not exist (any more).
    readonly property var exercise: file.exercise
    readonly property bool found: file.found

    allowedOrientations: Orientation.All

    // Writes the exercise, without its tempo, to a file the user can share (R-EXR-13).
    function exportFile() {
        var r = exerciseLibrary.exportExercise(exerciseKey)
        notice = r.status === "ok"
            ? qsTr("Saved to %1. To suggest the exercise for the library, or a new icon, send the file to the author (see About).").arg(r.path)
            : qsTr("The exercise could not be exported.")
    }

    function clone() {
        var r = exerciseLibrary.clone(exerciseKey)
        var list = pageStack.previousPage(page)
        if (list && list.notice !== undefined)
            list.notice = r.status === "ok" ? qsTr("Created \"%1\".").arg(r.name) : qsTr("The exercise could not be copied.")
        pageStack.pop()
    }

    function confirmDelete() {
        var name = exercise.name
        var dialog = pageStack.push(Qt.resolvedUrl("DeleteExerciseDialog.qml"), {
            exerciseName: name,
            workouts: exerciseLibrary.workoutsUsing(exerciseKey)
        })
        dialog.accepted.connect(function () {
            var r = exerciseLibrary.remove(exerciseKey)
            var list = pageStack.previousPage(page)
            if (list && list.notice !== undefined)
                list.notice = r.status === "ok" ? qsTr("Deleted \"%1\".").arg(name) : qsTr("The exercise could not be deleted.")
            gone = r.status === "ok"
        })
    }

    // Set after a delete: the page leaves when it is shown again, after the dialog has closed.
    property bool gone: false

    onStatusChanged: {
        if (status === PageStatus.Active && gone) {
            pageStack.pop()
        } else if (status === PageStatus.Active) {
            file.reload()
        }
    }

    SilicaFlickable {
        id: flick
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            visible: page.found
            MenuItem {
                text: qsTr("Delete")
                onClicked: page.confirmDelete()
            }
            MenuItem {
                text: qsTr("Clone")
                onClicked: page.clone()
            }
            MenuItem {
                text: qsTr("Export")
                onClicked: page.exportFile()
            }
            MenuItem {
                text: qsTr("Edit")
                onClicked: pageStack.push(Qt.resolvedUrl("ExerciseEditPage.qml"), { exerciseKey: page.exerciseKey })
            }
        }

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader { title: page.found ? MuscleText.plain(page.exercise.name) : qsTr("Exercise") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.notice !== ""
                wrapMode: Text.Wrap
                textFormat: Text.PlainText
                color: Theme.highlightColor
                text: page.notice
            }

            ExerciseFile {
                id: file
                width: parent.width
                exerciseKey: page.exerciseKey
            }
        }

        VerticalScrollDecorator { }
    }
}

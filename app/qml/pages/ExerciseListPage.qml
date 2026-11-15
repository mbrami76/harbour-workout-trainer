// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import "../components"

// The exercise library (R-MOD-1, R-EXR-1, R-EXR-6): every exercise by name, starter exercises
// included, with its icon in its colour, its muscle groups as text and a mark when it has a tempo.
// The list can be searched by name and limited to one muscle group. A tap opens the detail page; a
// press and hold opens a menu to edit, clone or delete.
Page {
    id: page

    allowedOrientations: Orientation.All

    // Shown under the list after a clone or a delete.
    property string notice

    function open(key) {
        notice = ""
        pageStack.push(Qt.resolvedUrl("ExercisePage.qml"), { exerciseKey: key })
    }

    function create() {
        notice = ""
        pageStack.push(Qt.resolvedUrl("ExerciseEditPage.qml"), { exerciseKey: "" })
    }

    // Adds the exercise of a file chosen with the picker (R-EXR-13); nothing is added unless the
    // whole file is valid.
    function importFile(path) {
        var r = exerciseLibrary.importExercise(path)
        if (r.status === "ok") {
            var text = qsTr("Imported \"%1\".").arg(r.name)
            for (var i = 0; i < r.notices.length; ++i)
                text += " " + MuscleText.importNotice(r.notices[i])
            notice = text
        } else {
            notice = qsTr("Nothing was imported.") + " " + MuscleText.importProblem(r.problems.length > 0 ? r.problems[0] : "")
        }
    }

    Component {
        id: pickerPage
        FilePickerPage {
            nameFilters: ["*.json"]
            onSelectedContentPropertiesChanged: {
                if (selectedContentProperties && selectedContentProperties.filePath)
                    page.importFile(selectedContentProperties.filePath)
            }
        }
    }

    function edit(key) {
        notice = ""
        pageStack.push(Qt.resolvedUrl("ExerciseEditPage.qml"), { exerciseKey: key })
    }

    function clone(e) {
        var r = exerciseLibrary.clone(e.key)
        notice = r.status === "ok" ? qsTr("Created \"%1\".").arg(r.name) : qsTr("The exercise could not be copied.")
    }

    function confirmDelete(e) {
        var dialog = pageStack.push(Qt.resolvedUrl("DeleteExerciseDialog.qml"), {
            exerciseName: e.name,
            workouts: exerciseLibrary.workoutsUsing(e.key)
        })
        dialog.accepted.connect(function () {
            notice = exerciseLibrary.remove(e.key).status === "ok"
                ? qsTr("Deleted \"%1\".").arg(e.name)
                : qsTr("The exercise could not be deleted.")
        })
    }

    Component.onCompleted: {
        exerciseLibrary.searchText = ""
        exerciseLibrary.groupFilter = ""
        exerciseLibrary.refresh()
        metronomeLibrary.refresh()
    }
    onStatusChanged: {
        if (status === PageStatus.Active)
            exerciseLibrary.refresh()
    }

    SilicaListView {
        id: list
        anchors.fill: parent
        model: exerciseLibrary.exercises
        cacheBuffer: height

        PullDownMenu {
            MenuItem {
                text: qsTr("Import exercise")
                onClicked: {
                    page.notice = ""
                    pageStack.push(pickerPage)
                }
            }
            MenuItem {
                text: qsTr("New exercise")
                onClicked: page.create()
            }
        }

        header: Column {
            width: list.width

            PageHeader { title: qsTr("Exercises") }

            SearchField {
                id: search
                width: parent.width
                placeholderText: qsTr("Search by name")
                text: exerciseLibrary.searchText
                inputMethodHints: Qt.ImhNoPredictiveText
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
                onTextChanged: exerciseLibrary.searchText = text
            }

            ComboBox {
                id: groupBox
                width: parent.width
                label: qsTr("Muscle group")
                currentIndex: exerciseLibrary.muscleKeys.indexOf(exerciseLibrary.groupFilter) + 1
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("All groups")
                        onClicked: exerciseLibrary.groupFilter = ""
                    }
                    Repeater {
                        model: exerciseLibrary.muscleKeys
                        delegate: MenuItem {
                            text: MuscleText.label(modelData)
                            onClicked: exerciseLibrary.groupFilter = modelData
                        }
                    }
                }
            }

            BackgroundItem {
                width: parent.width
                height: Math.max(Theme.itemSizeLarge, Metrics.mm(9))
                onClicked: page.create()
                Accessible.role: Accessible.Button
                Accessible.name: qsTr("New exercise")

                Label {
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    text: qsTr("New exercise")
                    font.pixelSize: Theme.fontSizeLarge
                    color: highlighted ? Theme.highlightColor : Theme.primaryColor
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: visible ? implicitHeight + Theme.paddingLarge : 0
                visible: exerciseLibrary.exercises.length === 0
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
                text: exerciseLibrary.totalCount === 0
                      ? qsTr("The library is empty. Use New exercise to make one.")
                      : qsTr("No exercise matches the search and the muscle group.")
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
            contentHeight: Math.max(Theme.itemSizeLarge, Metrics.mm(11) + 2 * Theme.paddingMedium,
                                    texts.height + 2 * Theme.paddingMedium)
            onClicked: page.open(modelData.key)

            Accessible.role: Accessible.Button
            Accessible.name: modelData.name

            IconBadge {
                id: badge
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                iconKey: modelData.icon
                colourKey: modelData.colour
            }

            Column {
                id: texts
                anchors {
                    left: badge.right
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
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
                // The muscle groups wrap to at most two lines and end in an ellipsis.
                Label {
                    width: parent.width
                    text: MuscleText.names(modelData.muscles)
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
                // A mark for an exercise that has a tempo, on a line of its own below the texts.
                Item {
                    width: parent.width
                    height: modelData.hasTempo ? tempoMark.height + Theme.paddingSmall : 0
                    visible: modelData.hasTempo

                    Rectangle {
                        id: tempoMark
                        anchors {
                            left: parent.left
                            top: parent.top
                            topMargin: Theme.paddingSmall
                        }
                        width: tempoLabel.implicitWidth + 2 * Theme.paddingSmall
                        height: tempoLabel.implicitHeight + Theme.paddingSmall
                        radius: height / 2
                        color: "transparent"
                        border.width: Math.max(1, Metrics.mm(0.3))
                        border.color: Theme.secondaryColor

                        Label {
                            id: tempoLabel
                            anchors.centerIn: parent
                            text: qsTr("Rhythm")
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                        }
                    }
                }
            }

            menu: Component {
                ContextMenu {
                    MenuItem {
                        text: qsTr("Edit")
                        onClicked: page.edit(modelData.key)
                    }
                    MenuItem {
                        text: qsTr("Clone")
                        onClicked: page.clone(modelData)
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
}

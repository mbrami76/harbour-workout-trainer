// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// Chooses the exercise of a step (R-WRK-13, R-EXR-4): the library searched by name and limited to
// one muscle group, with "New exercise" as the first choice. New exercise opens the exercise
// editor; when the new exercise is saved it becomes the exercise of the step and this page closes,
// when the editor is cancelled this page is shown again as it was. The workout being built is kept
// in workoutBuilder, so nothing of it is lost on the way.
Page {
    id: page

    // The step the choice is for.
    property int block
    property int step
    // The exercise the step has now, marked in the list.
    property string chosenKey

    allowedOrientations: Orientation.All

    // The groups with the least time over the last 5 sessions (R-DSH-4); empty with fewer than 5 sessions.
    property var suggested: []

    function marked(muscles) {
        for (var i = 0; i < muscles.length; ++i) {
            if (suggested.indexOf(muscles[i]) >= 0)
                return true
        }
        return false
    }

    function choose(key) {
        workoutBuilder.setStepExercise(block, step, key)
        pageStack.pop()
    }

    // The key of an exercise that was just saved in the editor; the page chooses it as soon as it
    // is shown again, because a page cannot be closed while the editor is still closing.
    property string newKey

    function createExercise() {
        newKey = ""
        var editor = pageStack.push(Qt.resolvedUrl("ExerciseEditPage.qml"), { exerciseKey: "" })
        editor.saved.connect(function (key) {
            page.newKey = key
        })
    }

    Timer {
        id: chooseNew
        interval: 50
        onTriggered: {
            var key = page.newKey
            page.newKey = ""
            if (key !== "")
                page.choose(key)
        }
    }

    Component.onCompleted: {
        exerciseLibrary.searchText = ""
        exerciseLibrary.groupFilter = ""
        exerciseLibrary.refresh()
        suggested = dashboard.suggestedGroups()
    }
    onStatusChanged: {
        if (status === PageStatus.Active) {
            exerciseLibrary.refresh()
            suggested = dashboard.suggestedGroups()
            if (newKey !== "")
                chooseNew.restart()
        }
    }

    SilicaListView {
        id: list
        anchors.fill: parent
        model: exerciseLibrary.exercises
        cacheBuffer: height

        header: Column {
            width: list.width

            PageHeader { title: qsTr("Choose an exercise") }

            SearchField {
                width: parent.width
                placeholderText: qsTr("Search by name")
                text: exerciseLibrary.searchText
                inputMethodHints: Qt.ImhNoPredictiveText
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
                onTextChanged: exerciseLibrary.searchText = text
            }

            ComboBox {
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
                onClicked: page.createExercise()
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
            visible: page.suggested.length > 0
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryHighlightColor
            text: qsTr("! marks an exercise that works a muscle group that was little trained in the last 5 sessions.")
        }

        delegate: ListItem {
            id: item
            width: ListView.view.width
            contentHeight: Math.max(Theme.itemSizeLarge, Metrics.mm(11) + 2 * Theme.paddingMedium)
            readonly property bool suggestion: page.marked(modelData.muscles)
            onClicked: page.choose(modelData.key)

            Accessible.role: Accessible.Button
            Accessible.name: suggestion ? qsTr("%1, works a muscle group that was little trained").arg(modelData.name)
                                        : modelData.name

            IconBadge {
                id: badge
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                iconKey: modelData.icon
                colourKey: modelData.colour
            }

            Glyph {
                id: mark
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                width: visible ? Metrics.mm(5) : 0
                height: Metrics.mm(5)
                visible: modelData.key === page.chosenKey
                kind: "check"
                color: Theme.highlightColor
            }

            Column {
                anchors {
                    left: badge.right
                    leftMargin: Theme.paddingLarge
                    right: mark.left
                    rightMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    width: parent.width
                    text: item.suggestion ? qsTr("! %1").arg(modelData.name) : modelData.name
                    textFormat: Text.PlainText
                    font.pixelSize: Theme.fontSizeMedium
                    color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                    truncationMode: TruncationMode.Fade
                }
                Label {
                    width: parent.width
                    text: modelData.hasTempo ? MuscleText.names(modelData.muscles) + ", " + qsTr("with rhythm")
                                             : MuscleText.names(modelData.muscles)
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    truncationMode: TruncationMode.Fade
                }
            }
        }

        VerticalScrollDecorator { }
    }
}

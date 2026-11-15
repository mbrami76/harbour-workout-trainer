// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// One workout read only (R-WRK-1, R-WRK-9, R-WRK-10): its blocks and steps with exercise names,
// durations or repetitions, the tempo of the exercises that have one, the rests, the total planned
// time and the repetition steps that the total leaves out. Edit and Start rest at the bottom
// (R-UI-13); Start opens the session.
Page {
    id: page

    property int workoutId
    property var info: ({})
    readonly property bool found: info.found === true

    allowedOrientations: Orientation.All

    function load() {
        info = workoutLibrary.detail(workoutId)
    }

    function edit() {
        pageStack.push(Qt.resolvedUrl("WorkoutBuilderPage.qml"), { workoutId: workoutId })
    }

    function start() {
        pageStack.push(Qt.resolvedUrl("SessionPage.qml"), { workoutId: workoutId })
    }

    Component.onCompleted: load()
    onStatusChanged: {
        if (status === PageStatus.Active)
            load()
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
        model: page.found ? page.info.rows : []
        cacheBuffer: height

        PullDownMenu {
            MenuItem {
                text: qsTr("Export workout")
                onClicked: pageStack.push(Qt.resolvedUrl("ExportResultPage.qml"),
                                          { result: exchangeBridge.exportWorkout(page.workoutId) })
            }
        }

        header: Column {
            width: list.width

            PageHeader {
                title: page.found ? page.info.header.name : qsTr("Workout")
                titleColor: Theme.highlightColor
            }

            Row {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingLarge
                visible: page.found

                IconBadge {
                    id: badge
                    iconKey: page.found ? page.info.header.icon : ""
                    iconSet: "workout"
                    colourKey: page.found ? page.info.header.colour : ""
                }

                Column {
                    width: parent.width - Metrics.mm(11) - Theme.paddingLarge
                    anchors.verticalCenter: parent.verticalCenter

                    Label {
                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: Theme.fontSizeMedium
                        color: Theme.primaryColor
                        text: page.found ? WorkoutText.totalText(page.info.total) : ""
                    }
                    Label {
                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryHighlightColor
                        text: page.found
                              ? qsTr("%n block(s)", "", page.info.header.blockCount) + ", "
                                + qsTr("%n step(s)", "", page.info.header.stepCount)
                              : ""
                    }
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: visible ? implicitHeight + Theme.paddingMedium : 0
                visible: page.found && page.info.total.uncounted > 0
                wrapMode: Text.Wrap
                textFormat: Text.PlainText
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.highlightColor
                text: page.found ? WorkoutText.uncountedText(page.info.total) : ""
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: visible ? implicitHeight + Theme.paddingLarge : 0
                visible: !page.found
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
                text: qsTr("This workout no longer exists.")
            }

            Item {
                width: 1
                height: Theme.paddingMedium
            }
        }

        delegate: Loader {
            width: ListView.view.width
            sourceComponent: modelData.kind === "block" ? blockRow
                           : (modelData.kind === "step" ? stepRow : restRow)
            property var row: modelData
        }

        VerticalScrollDecorator { }
    }

    Component {
        id: blockRow
        Item {
            property var row: parent ? parent.row : ({})
            height: blockLabel.height + Theme.paddingLarge
            Label {
                id: blockLabel
                x: Theme.horizontalPageMargin
                y: Theme.paddingLarge
                width: parent.width - 2 * Theme.horizontalPageMargin
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.highlightColor
                wrapMode: Text.Wrap
                text: WorkoutText.blockTitle(row) + ": " + WorkoutText.blockDetails(row)
            }
        }
    }

    Component {
        id: stepRow
        Item {
            property var row: parent ? parent.row : ({})
            height: Math.max(Metrics.mm(9), stepColumn.height + 2 * Theme.paddingSmall)

            Rectangle {
                id: typeBar
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(2, Metrics.mm(1))
                height: parent.height - 2 * Theme.paddingSmall
                radius: width / 2
                color: StepStyle.color(row.type)
            }

            Column {
                id: stepColumn
                anchors {
                    left: typeBar.right
                    leftMargin: Theme.paddingMedium
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                Label {
                    width: parent.width
                    textFormat: Text.PlainText
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.primaryColor
                    wrapMode: Text.Wrap
                    text: WorkoutText.stepTitle(row, StepStyle.label(row.type))
                }
                Label {
                    width: parent.width
                    textFormat: Text.PlainText
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    wrapMode: Text.Wrap
                    text: WorkoutText.stepDetails(row)
                }
            }
        }
    }

    Component {
        id: restRow
        Item {
            property var row: parent ? parent.row : ({})
            height: restLabel.height + Theme.paddingMedium
            Label {
                id: restLabel
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("Rest between blocks %1").arg(WorkoutText.duration(row.sec))
            }
        }
    }

    Item {
        id: bar
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: buttons.height + 2 * Theme.paddingMedium

        Row {
            id: buttons
            anchors {
                left: parent.left
                right: parent.right
                margins: Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }
            spacing: Theme.paddingMedium
            enabled: page.found

            BigButton {
                width: (parent.width - parent.spacing) / 2
                text: qsTr("Edit")
                onActivated: page.edit()
            }
            BigButton {
                width: (parent.width - parent.spacing) / 2
                text: qsTr("Start")
                selected: true
                onActivated: page.start()
            }
        }
    }
}

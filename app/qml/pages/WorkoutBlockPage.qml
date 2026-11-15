// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// One block of the workout being built (R-WRK-2, R-WRK-5, R-WRK-6): the rounds, the rest between
// rounds, and the actions that add a step, move the block up or down, duplicate it or remove it.
// Every change goes straight to the draft of workoutBuilder; the page keeps nothing of its own.
Page {
    id: page

    property int block
    property var info: ({})
    readonly property var limits: workoutBuilder.limits
    readonly property bool found: info.rounds !== undefined

    allowedOrientations: Orientation.All

    function reload() {
        info = workoutBuilder.block(block)
    }

    function moved(p) {
        if (p.block !== undefined)
            block = p.block
        reload()
    }

    function addStep() {
        var p = workoutBuilder.addStep(block)
        if (p.block !== undefined)
            pageStack.push(Qt.resolvedUrl("WorkoutStepPage.qml"), { block: p.block, step: p.step })
    }

    function remove() {
        Remorse.popupAction(page, qsTr("Removing the block"), function () {
            workoutBuilder.removeBlock(block)
            pageStack.pop()
        })
    }

    Component.onCompleted: reload()
    onBlockChanged: reload()
    onStatusChanged: {
        if (status === PageStatus.Active)
            reload()
    }

    Connections {
        target: workoutBuilder
        onChanged: page.reload()
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Block %1 of %2").arg(page.block + 1).arg(workoutBuilder.header.blockCount)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.found && page.info.problems.length > 0
                wrapMode: Text.Wrap
                color: Theme.errorColor
                text: page.found && page.info.problems.length > 0 ? WorkoutText.problem(page.info.problems[0], page.limits) : ""
            }

            NumberStepper {
                id: rounds
                visible: page.found
                label: qsTr("Rounds")
                minimum: page.limits.minRounds
                maximum: page.limits.maxRounds
                valueText: String(value)
                onEdited: workoutBuilder.setRounds(page.block, value)
            }
            Binding {
                target: rounds
                property: "value"
                value: page.found ? page.info.rounds : 1
            }

            NumberStepper {
                id: roundRest
                visible: page.found
                label: qsTr("Rest between rounds")
                minimum: page.limits.minRestSec
                maximum: page.limits.maxRestSec
                valueText: WorkoutText.duration(value)
                onEdited: workoutBuilder.setRestBetweenRounds(page.block, value)
            }
            Binding {
                target: roundRest
                property: "value"
                value: page.found ? page.info.restBetweenRoundsSec : 0
            }

            SectionHeader { text: qsTr("Steps") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
                text: page.found ? qsTr("%n step(s) in this block", "", page.info.stepCount) : ""
            }

            SectionHeader { text: qsTr("Actions") }

            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingMedium
                visible: page.found

                BigButton {
                    width: parent.width
                    text: qsTr("Add step")
                    onActivated: page.addStep()
                }

                Row {
                    width: parent.width
                    spacing: Theme.paddingMedium
                    BigButton {
                        width: (parent.width - parent.spacing) / 2
                        text: qsTr("Move up")
                        enabled: page.block > 0
                        opacity: enabled ? 1 : 0.4
                        onActivated: page.moved(workoutBuilder.moveBlock(page.block, -1))
                    }
                    BigButton {
                        width: (parent.width - parent.spacing) / 2
                        text: qsTr("Move down")
                        enabled: page.block < workoutBuilder.header.blockCount - 1
                        opacity: enabled ? 1 : 0.4
                        onActivated: page.moved(workoutBuilder.moveBlock(page.block, 1))
                    }
                }

                Row {
                    width: parent.width
                    spacing: Theme.paddingMedium
                    BigButton {
                        width: (parent.width - parent.spacing) / 2
                        text: qsTr("Duplicate")
                        onActivated: page.moved(workoutBuilder.duplicateBlock(page.block))
                    }
                    BigButton {
                        width: (parent.width - parent.spacing) / 2
                        text: qsTr("Remove")
                        color: Theme.errorColor
                        onActivated: page.remove()
                    }
                }
            }
        }

        VerticalScrollDecorator { }
    }
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// One step of the workout being built (R-WRK-3, R-WRK-4, R-WRK-6, R-WRK-10, R-WRK-11, R-WRK-14): its
// type, its exercise (chosen in the picker), how it ends and the number, how the repetitions are
// timed, whether it waits for a tap, and
// the actions that move it up or down or to another block, duplicate it or remove it. Every change
// goes straight to the draft of workoutBuilder; the page keeps only the place of the step.
Page {
    id: page

    property int block
    property int step
    property var row: ({})
    // Said after a duplicate.
    property string notice
    readonly property var limits: workoutBuilder.limits
    readonly property bool found: row.type !== undefined
    readonly property bool isRest: found && row.type === "rest"
    // The pace choices are for repetitions of an exercise without a rhythm (R-WRK-14).
    readonly property bool paceShown: found && !isRest && !row.timed && !row.hasRhythm
    // The rhythm of the step's exercise as the list of rhythms shows it; empty for none.
    property var rhythm: ({})

    allowedOrientations: Orientation.All

    function reload() {
        row = workoutBuilder.step(block, step)
        rhythm = ({})
        if (row.hasRhythm) {
            metronomeLibrary.refresh()
            var exercise = exerciseLibrary.get(row.exerciseKey)
            var all = metronomeLibrary.metronomes
            for (var i = 0; i < all.length; ++i) {
                if (exercise.metronomeId !== undefined && all[i].id === exercise.metronomeId)
                    rhythm = all[i]
            }
        }
    }

    function moved(p) {
        if (p.block !== undefined) {
            block = p.block
            step = p.step
        }
        reload()
    }

    function pick() {
        pageStack.push(Qt.resolvedUrl("ExercisePickerPage.qml"), { block: block, step: step, chosenKey: row.exerciseKey })
    }

    function remove() {
        Remorse.popupAction(page, qsTr("Removing the step"), function () {
            workoutBuilder.removeStep(block, step)
            pageStack.pop()
        })
    }

    Component.onCompleted: reload()
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
                title: qsTr("Block %1, step %2").arg(page.block + 1).arg(page.step + 1)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.found && page.row.problems.length > 0
                wrapMode: Text.Wrap
                color: Theme.errorColor
                text: page.found && page.row.problems.length > 0 ? WorkoutText.problem(page.row.problems[0], page.limits) : ""
            }

            SectionHeader { text: qsTr("Type") }

            Grid {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                columns: 2
                spacing: Theme.paddingMedium
                visible: page.found

                Repeater {
                    model: ["work", "rest", "warmup", "cooldown"]
                    delegate: BigButton {
                        width: (parent.width - parent.spacing) / 2
                        text: StepStyle.label(modelData)
                        color: StepStyle.color(modelData)
                        selected: page.found && page.row.type === modelData
                        Accessible.checked: selected
                        onActivated: workoutBuilder.setStepType(page.block, page.step, modelData)
                    }
                }
            }

            SectionHeader {
                text: qsTr("Exercise")
                visible: page.found && !page.isRest
            }

            BackgroundItem {
                width: parent.width
                height: Math.max(Theme.itemSizeLarge, Metrics.mm(9), exerciseColumn.height + 2 * Theme.paddingMedium)
                visible: page.found && !page.isRest
                onClicked: page.pick()
                Accessible.role: Accessible.Button
                Accessible.name: page.found && page.row.exerciseName !== ""
                                 ? qsTr("Exercise %1, choose another").arg(page.row.exerciseName)
                                 : qsTr("Choose an exercise")

                Column {
                    id: exerciseColumn
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    Label {
                        width: parent.width
                        textFormat: Text.PlainText
                        font.pixelSize: Theme.fontSizeLarge
                        color: highlighted ? Theme.highlightColor : Theme.primaryColor
                        wrapMode: Text.Wrap
                        text: page.found && page.row.exerciseName !== "" ? page.row.exerciseName : qsTr("Choose an exercise")
                    }
                    Label {
                        width: parent.width
                        visible: text !== ""
                        textFormat: Text.PlainText
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                        wrapMode: Text.Wrap
                        text: !page.found ? ""
                              : (page.row.hasRhythm ? WorkoutText.tempo(page.row) : qsTr("This exercise has no rhythm."))
                    }
                }
            }

            SectionHeader { text: qsTr("Ends by") }

            // An exercise with a rhythm ends by repetitions only (R-WRK-14).
            Row {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingMedium
                visible: page.found && !page.isRest && !page.row.hasRhythm

                BigButton {
                    width: (parent.width - parent.spacing) / 2
                    text: qsTr("Duration")
                    selected: page.found && page.row.timed
                    Accessible.checked: selected
                    onActivated: workoutBuilder.setStepTimed(page.block, page.step, true)
                }
                BigButton {
                    width: (parent.width - parent.spacing) / 2
                    text: qsTr("Repetitions")
                    selected: page.found && !page.row.timed
                    Accessible.checked: selected
                    onActivated: workoutBuilder.setStepTimed(page.block, page.step, false)
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.isRest
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
                text: qsTr("A rest ends by duration.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.found && !page.isRest && page.row.hasRhythm
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
                text: qsTr("The exercise has a rhythm: the step ends by repetitions and runs them at the rhythm.")
            }

            // The rhythm of the exercise: its name and the strip of one repetition.
            Row {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: Math.max(Metrics.mm(5), rhythmName.implicitHeight) + Theme.paddingSmall
                spacing: Theme.paddingMedium
                visible: page.found && !page.isRest && page.row.hasRhythm && page.rhythm.id !== undefined

                PaceStrip {
                    width: (parent.width - parent.spacing) / 2
                    anchors.verticalCenter: parent.verticalCenter
                    units: page.rhythm.phaseUnits !== undefined ? page.rhythm.phaseUnits : []
                }
                Label {
                    id: rhythmName
                    width: (parent.width - parent.spacing) / 2
                    anchors.verticalCenter: parent.verticalCenter
                    textFormat: Text.PlainText
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    truncationMode: TruncationMode.Fade
                    text: page.rhythm.name !== undefined ? page.rhythm.name : ""
                }
            }

            NumberStepper {
                id: amount
                visible: page.found
                label: page.found && page.row.timed ? qsTr("Duration") : qsTr("Repetitions")
                minimum: page.found && page.row.timed ? page.limits.minDurationSec : page.limits.minReps
                maximum: page.found && page.row.timed ? page.limits.maxDurationSec : page.limits.maxReps
                steps: page.found && page.row.timed ? [-60, -10, -1, 1, 10, 60] : [-10, -1, 1, 10]
                valueText: page.found && page.row.timed ? WorkoutText.duration(value) : String(value)
                onEdited: {
                    if (page.row.timed)
                        workoutBuilder.setStepDuration(page.block, page.step, value)
                    else
                        workoutBuilder.setStepReps(page.block, page.step, value)
                }
            }
            Binding {
                target: amount
                property: "value"
                value: page.found ? (page.row.timed ? page.row.durationSec : page.row.reps) : 0
            }

            // How the repetitions of an exercise without a rhythm are timed (R-WRK-14).
            SectionHeader {
                text: qsTr("Pace")
                visible: page.paceShown
            }

            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingMedium
                visible: page.paceShown

                Repeater {
                    model: [
                        { key: "none", name: qsTr("Tap on done") },
                        { key: "total", name: qsTr("Auto-pace: total time") },
                        { key: "per-rep", name: qsTr("Manual-pace: time per repetition") }
                    ]
                    delegate: BigButton {
                        width: parent.width
                        height: Metrics.buttonSize
                        minimumSize: Metrics.buttonSize
                        fontPixelSize: Theme.fontSizeSmall
                        selected: page.found && page.row.pace === modelData.key
                        Accessible.checked: selected
                        text: modelData.name
                        onActivated: workoutBuilder.setStepPace(page.block, page.step, modelData.key, 0)
                    }
                }
            }

            NumberStepper {
                id: paceAmount
                visible: page.paceShown && page.found && page.row.pace !== "none"
                label: page.found && page.row.pace === "total" ? qsTr("Total time") : qsTr("Time per repetition")
                minimum: page.found && page.row.pace === "total" ? page.row.reps * 1 : 1
                maximum: page.found && page.row.pace === "total" ? page.row.reps * 120 : 120
                // The value is in half seconds; the limits of the core (0.5 s to 60 s for one
                // repetition) decide, and the bridge clamps what is out of them.
                steps: page.found && page.row.pace === "total" ? [-20, -2, -1, 1, 2, 20] : [-10, -2, -1, 1, 2, 10]
                valueText: qsTr("%1 s").arg((value / 2).toFixed(1))
                onEdited: workoutBuilder.setStepPace(page.block, page.step, page.row.pace, value * 500)
            }
            Binding {
                target: paceAmount
                property: "value"
                value: page.found && page.row.pace !== "none" ? Math.round(page.row.paceMs / 500) : 0
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.found && page.row.paceIgnored
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                text: qsTr("A pace stored for this step is not used while the exercise has a rhythm.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.found && !page.row.timed && !page.isRest
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
                text: {
                    if (!page.found || page.row.timed)
                        return ""
                    if (page.row.rhythmUsed)
                        return qsTr("The step takes %1.").arg(WorkoutText.duration(Math.round(page.row.plannedMs / 1000)))
                    if (page.row.paceUsed)
                        return WorkoutText.paceWords(page.row.pace, page.row.reps, page.row.repMs) + ". "
                               + qsTr("The step takes %1.").arg(WorkoutText.duration(Math.round(page.row.plannedMs / 1000)))
                    return qsTr("The step ends when you tap Done, so its time is not counted in the total.")
                }
            }

            SectionHeader { text: qsTr("Start") }

            TextSwitch {
                width: parent.width
                height: Math.max(implicitHeight, Metrics.mm(9))
                visible: page.found && page.row.startModeAllowed
                text: qsTr("Wait for a tap before this step")
                description: qsTr("Otherwise the step starts by itself when the step before it ends.")
                checked: page.found && page.row.startOnTap
                automaticCheck: false
                onClicked: workoutBuilder.setStepStartOnTap(page.block, page.step, !checked)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.found && !page.row.startModeAllowed
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
                text: qsTr("The first step of the workout always starts when the session starts.")
            }

            SectionHeader { text: qsTr("Place") }

            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingMedium
                visible: page.found

                Row {
                    width: parent.width
                    spacing: Theme.paddingMedium
                    BigButton {
                        width: (parent.width - parent.spacing) / 2
                        text: qsTr("Move up")
                        enabled: page.step > 0
                        opacity: enabled ? 1 : 0.4
                        onActivated: page.moved(workoutBuilder.moveStep(page.block, page.step, -1))
                    }
                    BigButton {
                        width: (parent.width - parent.spacing) / 2
                        text: qsTr("Move down")
                        enabled: page.step < workoutBuilder.block(page.block).stepCount - 1
                        opacity: enabled ? 1 : 0.4
                        onActivated: page.moved(workoutBuilder.moveStep(page.block, page.step, 1))
                    }
                }

                ComboBox {
                    id: moveBox
                    x: -Theme.horizontalPageMargin
                    width: parent.width + 2 * Theme.horizontalPageMargin
                    label: qsTr("Move to block")
                    value: qsTr("Block %1").arg(page.block + 1)
                    enabled: workoutBuilder.header.blockCount > 1
                    menu: ContextMenu {
                        Repeater {
                            model: workoutBuilder.header.blockCount
                            delegate: MenuItem {
                                text: qsTr("Block %1").arg(index + 1)
                                visible: index !== page.block
                                onClicked: page.moved(workoutBuilder.moveStepToBlock(page.block, page.step, index))
                            }
                        }
                    }
                }

                Row {
                    width: parent.width
                    spacing: Theme.paddingMedium
                    BigButton {
                        width: (parent.width - parent.spacing) / 2
                        text: qsTr("Duplicate")
                        onActivated: {
                            page.moved(workoutBuilder.duplicateStep(page.block, page.step))
                            page.notice = qsTr("Duplicated. This page now shows the copy.")
                        }
                    }
                    BigButton {
                        width: (parent.width - parent.spacing) / 2
                        text: qsTr("Remove")
                        color: Theme.errorColor
                        onActivated: page.remove()
                    }
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.notice !== ""
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                text: page.notice
            }
        }

        VerticalScrollDecorator { }
    }
}

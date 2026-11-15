// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// The file of an exercise over a running session (R-TMR-12). The session goes on underneath: the
// timer, the voice and the sounds are not touched. A bar at the bottom keeps the time of the step and
// the pause and stop controls within reach, laid out from the screen and not from the system font
// size. The page shows the exercise it was opened for until it is closed, with the close button or
// the back gesture, also when the session moves on. It scrolls when the file does not fit.
Page {
    id: page

    property string exerciseKey

    allowedOrientations: Orientation.All

    readonly property string phaseKind: sessionBridge.phaseKind
    readonly property string typeKind: phaseKind === "leadin" ? "leadin" : (sessionBridge.current.type !== undefined ? sessionBridge.current.type : "work")
    readonly property real gap: Metrics.mm(2)
    readonly property real cellHeight: Metrics.buttonSize + Metrics.labelSize * 1.3
    readonly property real lineHeight: Metrics.labelSize * 1.9
    readonly property real barHeight: lineHeight + cellHeight + 3 * gap
    // The time that matters now: the lead-in or the step still to run, or what a step without a
    // planned end shows.
    readonly property string timeText: {
        if (phaseKind === "leadin")
            return timeFormatter.countdown(Math.ceil(sessionBridge.leadInRemainingMs / 1000) * 1000)
        if (phaseKind === "timed" || phaseKind === "tempo")
            return timeFormatter.countdown(sessionBridge.stepRemainingMs)
        if (phaseKind === "reps")
            return qsTr("Time so far: %1").arg(timeFormatter.elapsed(sessionBridge.stepElapsedMs))
        if (phaseKind === "waiting")
            return qsTr("Tap to start")
        return ""
    }

    // The session finishes while the file is open: back to the session page, which shows the end. A
    // stop is handled by the session page, which leaves for the workout.
    Connections {
        target: sessionBridge
        onStateChanged: {
            if (sessionBridge.isFinished)
                pageStack.pop()
        }
    }

    // Portrait: the file above the bar. Landscape: the file on the left and the bar in a column on the
    // right, so that the file keeps the height of the screen.
    SilicaFlickable {
        id: flick
        width: page.isPortrait ? page.width : bar.x - page.gap
        height: page.isPortrait ? bar.y : page.height
        clip: true
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: file.found ? MuscleText.plain(file.exercise.name) : qsTr("Exercise")
            }

            ExerciseFile {
                id: file
                width: parent.width
                exerciseKey: page.exerciseKey
            }
        }

        VerticalScrollDecorator { }
    }

    Item {
        id: bar
        width: page.isPortrait ? page.width * 0.95 : Math.min(page.width * 0.95 * 0.44, Metrics.mm(84))
        x: page.isPortrait ? page.width * 0.025 : page.width * 0.975 - width
        height: page.barHeight
        y: page.height * 0.975 - height

        // The step and the time, in the colour of the step type (R-UI-3).
        Item {
            id: line
            width: parent.width
            height: page.lineHeight

            Label {
                id: typeLabel
                height: parent.height
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: Metrics.labelSize * 1.5
                font.bold: true
                color: StepStyle.color(page.typeKind)
                text: StepStyle.label(page.typeKind)
            }
            Label {
                x: typeLabel.width + page.gap
                width: parent.width - x
                height: parent.height
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: Metrics.labelSize * 1.5
                fontSizeMode: Text.Fit
                minimumPixelSize: Metrics.labelSize
                color: sessionBridge.isPaused ? Theme.highlightColor : Theme.primaryColor
                truncationMode: TruncationMode.Fade
                text: sessionBridge.isPaused ? qsTr("Paused") : page.timeText
            }
        }

        Row {
            y: line.height + page.gap
            width: parent.width
            height: page.cellHeight
            spacing: page.gap

            CaptionedControl {
                width: (parent.width - 2 * parent.spacing) / 3
                caption: qsTr("Exercise file")
                BigButton {
                    anchors.fill: parent
                    minimumSize: Metrics.buttonSize
                    text: qsTr("Close")
                    onActivated: pageStack.pop()
                }
            }
            CaptionedControl {
                width: (parent.width - 2 * parent.spacing) / 3
                caption: sessionBridge.playAction === "resume" ? qsTr("Resume")
                    : (sessionBridge.playAction === "start" ? qsTr("Start") : qsTr("Pause"))
                BigButton {
                    anchors.fill: parent
                    minimumSize: Metrics.buttonSize
                    enabled: sessionBridge.playAction !== "none"
                    opacity: enabled ? 1 : 0.4
                    text: sessionBridge.playAction === "resume" ? qsTr("Resume")
                        : (sessionBridge.playAction === "start" ? qsTr("Start the step") : qsTr("Pause"))
                    glyph: sessionBridge.playAction === "pause" ? "pause" : "play"
                    onActivated: {
                        if (sessionBridge.playAction === "pause")
                            sessionBridge.pause()
                        else if (sessionBridge.playAction === "resume")
                            sessionBridge.resume()
                        else if (sessionBridge.playAction === "start")
                            sessionBridge.startWaitingStep()
                    }
                }
            }
            CaptionedControl {
                width: (parent.width - 2 * parent.spacing) / 3
                caption: qsTr("Stop")
                HoldButton {
                    anchors.fill: parent
                    minimumSize: Metrics.buttonSize
                    color: Theme.secondaryHighlightColor
                    text: qsTr("Hold to stop")
                    glyph: "stop"
                    onConfirmed: sessionBridge.stop()
                }
            }
        }
    }
}

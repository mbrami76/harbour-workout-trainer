// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// A workout session in progress (R-UI-2, R-UI-12 to R-UI-16, R-TMR-1 to R-TMR-4, R-TMR-11). One screen
// without scrolling, laid out from the screen size and not from the system font size, within 95 % of
// the width and 93 % of the height. Portrait, from top to bottom: the status line; the workout name
// with its neutral bar and the elapsed and remaining time; the block and the round; the current step
// (type, pictogram, exercise, file button, the time and its bar); the next step; the three buttons.
// Landscape: two columns; on the left the status line, the workout and the block, on the right the
// step, the next step and the buttons. The sizes come from one scale (Metrics.runSize), the colours
// from the ambience with two built-in schemes as the fallback (RunScheme).
Item {
    id: run

    property bool portrait: true
    // False forces the built-in colour schemes; `fallbackDark` picks which one is used without the ambience.
    property bool useAmbience: true
    property bool fallbackDark: true
    // Opens the file of the exercise that the session offers (R-TMR-12).
    signal showExerciseFile()

    readonly property var cur: sessionBridge.current
    readonly property var nxt: sessionBridge.next
    readonly property string phaseKind: sessionBridge.phaseKind
    readonly property bool countdown: Settings.displayMode !== "direct"
    readonly property string typeKind: phaseKind === "leadin" ? "leadin" : (cur.type !== undefined ? cur.type : "work")
    readonly property bool hasExercise: cur.exerciseKey !== undefined && cur.exerciseKey !== ""
    readonly property bool hasNext: nxt.type !== undefined
    readonly property bool nextHasExercise: hasNext && nxt.exerciseKey !== ""
    readonly property bool paused: sessionBridge.isPaused

    RunScheme {
        id: colours
        useAmbience: run.useAmbience
        darkOverride: run.fallbackDark
    }

    // Shrinks the sizes on a screen shorter than the layout was drawn for: 1 for a screen of about
    // 155 mm (portrait) or 70 mm (landscape) or more, down to 0.7. The buttons stay at least 11 mm
    // high (R-UI-5), the texts at least the legible minimum (R-UI-12).
    readonly property real fit: Math.max(0.7, Math.min(1, portrait ? height * 0.93 / Metrics.mm(124)
                                                                   : height * 0.93 / Metrics.mm(62)))
    readonly property real gap: Metrics.mm(portrait ? 2 : 1.5) * fit
    readonly property real tight: Metrics.mm(1)
    function size(step) {
        return Math.max(Metrics.labelSize, Metrics.runSize(step) * fit)
    }
    readonly property real buttonHeight: portrait ? Math.max(Metrics.mm(11), Metrics.mm(14) * fit) : Metrics.mm(11)
    readonly property real barThickness: Metrics.mm(6.25)
    // The gap between the digits and the progress bar (R-UI-7); they never overlap.
    readonly property real barGap: Metrics.mm(2)
    readonly property real rowHeight: portrait ? Math.max(Metrics.mm(10), Metrics.mm(12) * fit) : Metrics.mm(9.5)
    // The name of the exercise under that row: two lines, three when the name needs them.
    readonly property real nextHeight: portrait ? size(1) * 1.5 : size(0) * 1.4
    // The time is the largest element but not more: its line is at most this high.
    readonly property real maxDigitsHeight: Metrics.mm(19) * fit
    // The time is never reduced below this share of its size to make room for the name of the exercise.
    readonly property real minDigitsShare: 0.6
    // The gap between the line "Next" and the bar of the step (R-UI-14).
    readonly property real nextGap: Metrics.mm(2)
    // The lines the name of the exercise may take: two in landscape, three in portrait.
    readonly property int nameLines: portrait ? 3 : 2
    readonly property real nameSize: portrait ? size(2) : size(1)

    // The flash of the beats of a paced step, behind everything.
    BeatPulse {
        id: pulse
        anchors.fill: parent
        clock: sessionBridge
    }

    function fire(kind, atMs) {
        pulse.fire(kind, atMs)
    }

    function clearPulse() {
        pulse.clear()
    }

    // Without the ambience the background is drawn here.
    Rectangle {
        anchors.fill: parent
        visible: colours.drawsBackground
        gradient: Gradient {
            GradientStop { position: 0.0; color: colours.builtIn.background[0] }
            GradientStop { position: 0.35; color: colours.builtIn.background[1] }
            GradientStop { position: 0.7; color: colours.builtIn.background[2] }
            GradientStop { position: 1.0; color: colours.builtIn.background[3] }
        }
    }

    // 95 % of the page wide, ending at 95 % of its height.
    Item {
        id: frame
        x: run.width * 0.025
        y: run.height * 0.02
        width: run.width * 0.95
        height: run.height * 0.93

        // Landscape has two columns on one grid: the information on the left, the step and the
        // controls on the right, both starting at the top and ending at the bottom of the frame.
        readonly property real leftWidth: run.portrait ? width : Math.min(width * 0.46, Metrics.mm(95))
        readonly property real rightX: run.portrait ? 0 : leftWidth + run.gap * 2
        readonly property real rightWidth: width - rightX

        // The information about the workout.
        Column {
            id: info
            width: frame.leftWidth
            spacing: run.tight

            // The status line (R-UI-15): the time of day, the date and the battery.
            Item {
                id: statusRow
                width: parent.width
                height: run.size(0) * 1.4

                Label {
                    id: clockLabel
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: run.size(0)
                    font.bold: true
                    color: colours.text
                    text: statusLine.time
                }
                Label {
                    anchors.left: clockLabel.right
                    anchors.leftMargin: run.gap
                    anchors.right: batteryRow.visible ? batteryRow.left : parent.right
                    anchors.rightMargin: run.gap
                    anchors.baseline: clockLabel.baseline
                    font.pixelSize: run.size(0)
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: Metrics.labelSize
                    elide: Text.ElideRight
                    color: colours.secondary
                    text: statusLine.date
                }
                // Shown only when the charge can be read.
                Row {
                    id: batteryRow
                    anchors.right: parent.right
                    height: parent.height
                    spacing: run.tight
                    visible: statusLine.batteryPercent >= 0

                    Item {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Metrics.mm(6)
                        height: Metrics.mm(3)

                        Rectangle {
                            id: batteryBody
                            width: parent.width - Metrics.mm(0.6)
                            height: parent.height
                            radius: Metrics.mm(0.5)
                            color: "transparent"
                            border.width: Math.max(1, Metrics.mm(0.3))
                            border.color: colours.text

                            Rectangle {
                                x: Metrics.mm(0.5)
                                y: Metrics.mm(0.5)
                                width: Math.max(0, (parent.width - Metrics.mm(1)) * statusLine.batteryPercent / 100)
                                height: parent.height - Metrics.mm(1)
                                radius: Metrics.mm(0.3)
                                color: colours.text
                            }
                        }
                        Rectangle {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            width: Metrics.mm(0.6)
                            height: parent.height * 0.4
                            color: colours.text
                        }
                    }
                    Label {
                        height: parent.height
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: run.size(0)
                        font.bold: true
                        color: colours.text
                        text: qsTr("%1%").arg(statusLine.batteryPercent)
                    }
                }
            }

            // The workout name; the label "Paused" beside it in portrait (R-UI-2).
            Item {
                width: parent.width
                height: run.size(1) * 1.3

                Label {
                    id: workoutName
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: run.size(1)
                    font.bold: true
                    fontSizeMode: Text.Fit
                    minimumPixelSize: Metrics.labelSize
                    elide: Text.ElideRight
                    color: colours.text
                    text: sessionBridge.workoutName
                }
            }

            // The neutral bar: the share of the workout done (not the colours of the step bars).
            Rectangle {
                width: parent.width
                height: Metrics.mm(2.2) * run.fit
                radius: height / 2
                color: colours.barTrack
                border.width: Math.max(1, Metrics.mm(0.3))
                border.color: colours.outline
                clip: true

                Rectangle {
                    readonly property real doneMs: sessionBridge.workoutElapsedMs
                    readonly property real leftMs: sessionBridge.workoutRemainingMs
                    width: parent.width * (doneMs + leftMs > 0 ? doneMs / (doneMs + leftMs) : 0)
                    height: parent.height
                    radius: parent.radius
                    color: colours.barFill
                }
            }
            Item {
                width: parent.width
                height: run.size(0) * 1.3

                Label {
                    anchors.left: parent.left
                    width: parent.width * 0.5 - run.gap
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: run.size(0)
                    fontSizeMode: Text.Fit
                    minimumPixelSize: Metrics.labelSize
                    elide: Text.ElideRight
                    color: colours.secondary
                    text: qsTr("Elapsed %1").arg(timeFormatter.countdown(sessionBridge.workoutElapsedMs))
                }
                Label {
                    anchors.right: parent.right
                    width: parent.width * 0.5 - run.gap
                    height: parent.height
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: run.size(0)
                    fontSizeMode: Text.Fit
                    minimumPixelSize: Metrics.labelSize
                    elide: Text.ElideLeft
                    color: colours.secondary
                    visible: sessionBridge.workoutRemainingMs > 0 || !sessionBridge.workoutRemainingOpen
                    text: (sessionBridge.workoutRemainingOpen ? qsTr("Timed left %1") : qsTr("Left %1"))
                              .arg(timeFormatter.countdown(sessionBridge.workoutRemainingMs))
                }
            }

            Rectangle {
                width: parent.width
                height: Math.max(1, Metrics.mm(0.2))
                color: colours.barTrack
            }

            // The position: the block on the left, the round on the right (R-TMR-6).
            Item {
                width: parent.width
                height: (run.portrait ? run.size(1) : run.size(2)) * 1.3

                Label {
                    anchors.left: parent.left
                    width: parent.width * 0.5 - run.gap
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: run.portrait ? run.size(1) : run.size(2)
                    font.bold: true
                    fontSizeMode: Text.Fit
                    minimumPixelSize: Metrics.labelSize
                    elide: Text.ElideRight
                    visible: sessionBridge.blockCount > 1
                    color: colours.text
                    text: qsTr("Block %1 of %2").arg(sessionBridge.blockNumber).arg(sessionBridge.blockCount)
                }
                Label {
                    anchors.right: parent.right
                    width: sessionBridge.blockCount > 1 ? parent.width * 0.5 - run.gap : parent.width
                    height: parent.height
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: run.portrait ? run.size(1) : run.size(2)
                    font.bold: true
                    fontSizeMode: Text.Fit
                    minimumPixelSize: Metrics.labelSize
                    elide: Text.ElideLeft
                    color: colours.text
                    text: qsTr("Round %1 / %2").arg(sessionBridge.roundNumber).arg(sessionBridge.roundCount)
                }
            }
        }

        // The three buttons, at the bottom of the frame (R-UI-13): play and pause, skip, stop. The
        // same size, icon only.
        Row {
            id: controls
            x: frame.rightX
            y: frame.height - run.buttonHeight
            width: frame.rightWidth
            height: run.buttonHeight
            spacing: run.gap

            readonly property real cell: (width - 2 * spacing) / 3

            RunButton {
                width: controls.cell
                height: parent.height
                scheme: colours
                tapEnabled: sessionBridge.playAction !== "none"
                pulsing: sessionBridge.playAction === "start"
                glyph: sessionBridge.playAction === "pause" ? "pause" : "play"
                text: sessionBridge.playAction === "pause" ? qsTr("Pause")
                    : (sessionBridge.playAction === "start" ? qsTr("Start the step") : qsTr("Resume"))
                onTapped: {
                    if (sessionBridge.playAction === "pause")
                        sessionBridge.pause()
                    else if (sessionBridge.playAction === "resume")
                        sessionBridge.resume()
                    else if (sessionBridge.playAction === "start")
                        sessionBridge.startWaitingStep()
                }
            }
            RunButton {
                width: controls.cell
                height: parent.height
                scheme: colours
                tapEnabled: sessionBridge.skipKind !== "none"
                holdEnabled: sessionBridge.skipBlockEnabled
                glyph: "next"
                text: sessionBridge.skipKind === "done" ? qsTr("Done, next step")
                    : (sessionBridge.skipKind === "end-set" ? qsTr("End the set, next step")
                                                            : qsTr("Next step, hold for the next block"))
                onTapped: sessionBridge.skip()
                onHeld: sessionBridge.skipBlock()
            }
            RunButton {
                width: controls.cell
                height: parent.height
                scheme: colours
                holdEnabled: sessionBridge.stopEnabled
                glyph: "stop"
                text: qsTr("Hold to stop")
                onHeld: sessionBridge.stop()
            }
        }

        // The next step.
        Item {
            id: nextRow
            x: frame.rightX
            y: controls.y - run.gap - height
            width: frame.rightWidth
            height: run.nextHeight

            Label {
                id: nextCaption
                height: parent.height
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: run.size(1)
                color: colours.secondary
                text: qsTr("Next")
            }
            Label {
                id: nextType
                x: nextCaption.width + run.gap
                height: parent.height
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: run.size(1)
                font.bold: true
                visible: run.hasNext
                color: run.hasNext ? colours.stepColor(run.nxt.type) : colours.text
                text: run.hasNext ? StepStyle.label(run.nxt.type) : ""
            }
            Label {
                x: nextType.x + (nextType.visible ? nextType.width + run.gap : 0)
                width: parent.width - x
                height: parent.height
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: run.size(1)
                fontSizeMode: Text.Fit
                minimumPixelSize: Metrics.labelSize
                elide: Text.ElideRight
                color: colours.text
                text: {
                    if (!run.hasNext)
                        return qsTr("End of the workout")
                    var amount = WorkoutText.ending({ timed: run.nxt.ending === "duration",
                                                     durationSec: run.nxt.durationSec, reps: run.nxt.reps })
                    return run.nextHasExercise ? run.nxt.exerciseName + ", " + amount : amount
                }
            }
        }

        // The current step: the row with its type, pictogram, name and the button of the file, and under
        // it what the phase shows. It fills the space between the information and the next step in
        // landscape, and sits above the next step in portrait.
        Item {
            id: stepGroup
            x: frame.rightX
            width: frame.rightWidth

            // Measures the name at its normal size, with the lines it may take.
            Text {
                id: nameProbe
                visible: false
                width: parent.width
                wrapMode: Text.WordWrap
                maximumLineCount: run.nameLines
                font.pixelSize: run.nameSize
                font.bold: true
                text: run.hasExercise ? run.cur.exerciseName : ""
            }

            readonly property real freeTop: run.portrait ? info.height + run.gap : 0
            readonly property real freeHeight: nextRow.y - run.nextGap - freeTop
            readonly property bool timed: run.phaseKind === "timed"
            readonly property bool digitsOnly: timed || run.phaseKind === "leadin"
            readonly property real barSpace: timed ? run.barThickness + run.barGap : 0
            // The name takes the lines it needs, up to its limit, but leaves the time at least its
            // minimum share of the normal size; when there is no room it shrinks to the legible
            // minimum and is cut. One line at the legible minimum is the least.
            readonly property real nameWanted: run.hasExercise ? nameProbe.contentHeight : 0
            readonly property real nameLeast: run.hasExercise ? Metrics.labelSize * 1.25 : 0
            readonly property real nameHeight: run.hasExercise
                ? Math.max(nameLeast, Math.min(nameWanted, freeHeight - run.rowHeight - 2 * run.gap - barSpace
                                                           - run.maxDigitsHeight * run.minDigitsShare))
                : 0
            readonly property real headHeight: run.rowHeight + (run.hasExercise ? run.tight + nameHeight : 0)
            // The room for the digits: what the group may take, less the head and the bar.
            readonly property real digitsRoom: Math.min(run.maxDigitsHeight, Math.max(Metrics.mm(8), freeHeight - headHeight - run.gap
                                                         - barSpace))
            readonly property real digitsHeight: Math.min(digitsRoom, timedDigits.implicitHeight)
            readonly property real wantedHeight: headHeight + run.gap + (digitsOnly ? digitsHeight + barSpace : freeHeight)
            height: Math.min(freeHeight, wantedHeight)
            y: run.portrait ? nextRow.y - run.nextGap - height : freeTop

            // The type, the pictogram, the name and the file button (R-TMR-12). The button is at the
            // top, away from the thumb, so that it is not pressed by mistake during an exercise.
            Item {
                id: stepRow
                width: parent.width
                height: run.rowHeight

                Rectangle {
                    id: pill
                    anchors.verticalCenter: parent.verticalCenter
                    width: pillLabel.implicitWidth + run.gap * 2
                    height: pillLabel.implicitHeight + run.tight * 2
                    radius: height / 2
                    color: colours.stepColor(run.typeKind)
                }
                Label {
                    id: pillLabel
                    x: run.gap
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: run.size(1)
                    font.bold: true
                    color: luminance(pill.color) > 0.179 ? "#000000" : "#ffffff"
                    text: StepStyle.label(run.typeKind)

                    function luminance(c) {
                        function channel(v) { return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4) }
                        return 0.2126 * channel(c.r) + 0.7152 * channel(c.g) + 0.0722 * channel(c.b)
                    }
                }
                IconBadge {
                    id: badge
                    x: pill.width + run.gap
                    anchors.verticalCenter: parent.verticalCenter
                    width: Metrics.mm(10) * run.fit
                    visible: run.hasExercise
                    iconKey: run.hasExercise ? run.cur.icon : ""
                    colourKey: run.hasExercise ? run.cur.colour : ""
                }
                // The state of the session, on the first line beside the file button.
                Label {
                    id: pausedInRow
                    anchors.right: fileButton.visible ? fileButton.left : parent.right
                    anchors.rightMargin: fileButton.visible ? run.gap : 0
                    anchors.baseline: pillLabel.baseline
                    visible: run.paused
                    font.pixelSize: run.size(1)
                    font.bold: true
                    color: colours.work
                    text: qsTr("Paused")
                }
                RunButton {
                    id: fileButton
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: run.rowHeight
                    height: width
                    scheme: colours
                    tapEnabled: true
                    visible: sessionBridge.exerciseFileKey !== ""
                    iconSize: Metrics.mm(5)
                    glyph: "info"
                    text: qsTr("Exercise file")
                    onTapped: run.showExerciseFile()
                }
            }


            // The whole name of the exercise under that line: it wraps (three lines at most) and shrinks to
            // the legible minimum before it is cut.
            Label {
                id: stepName
                y: run.rowHeight + run.tight
                width: parent.width
                height: stepGroup.nameHeight
                visible: run.hasExercise
                verticalAlignment: Text.AlignTop
                wrapMode: Text.WordWrap
                maximumLineCount: run.nameLines
                font.pixelSize: run.nameSize
                font.bold: true
                fontSizeMode: Text.Fit
                minimumPixelSize: Metrics.labelSize
                elide: Text.ElideRight
                color: colours.text
                text: run.hasExercise ? run.cur.exerciseName : ""
            }

            // What the phase shows.
            Item {
                id: region
                y: stepGroup.headHeight + run.gap
                width: parent.width
                height: parent.height - y

                // Lead-in: the countdown to the start of the step (R-TMR-1).
                Item {
                    anchors.fill: parent
                    visible: run.phaseKind === "leadin"

                    TimeDigits {
                        width: parent.width
                        height: implicitHeight
                        maxHeight: stepGroup.digitsRoom
                        color: colours.digits
                        outlineColor: colours.digitsOutline
                        showModeLabel: false
                        toggleOnTap: false
                        parts: timeFormatter.countdownParts(Math.ceil(sessionBridge.leadInRemainingMs / 1000) * 1000)
                    }
                }

                // The voice is being prepared before the session starts (R-ALR-10).
                Item {
                    anchors.fill: parent
                    visible: run.phaseKind === "preparing"

                    Column {
                        anchors.centerIn: parent
                        width: parent.width
                        spacing: run.gap * 2

                        Label {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            font.pixelSize: run.size(3)
                            font.bold: true
                            color: colours.text
                            text: qsTr("Preparing the voice")
                        }
                        ProgressBar {
                            width: parent.width
                            minimumValue: 0
                            maximumValue: 1
                            value: sessionBridge.prepareProgress
                        }
                        Button {
                            anchors.horizontalCenter: parent.horizontalCenter
                            height: Math.max(implicitHeight, run.buttonHeight)
                            text: qsTr("Start without voice")
                            onClicked: sessionBridge.startWithoutVoice()
                        }
                    }
                }

                // A step that waits for a tap (R-WRK-11): the play button starts it.
                Item {
                    anchors.fill: parent
                    visible: run.phaseKind === "waiting"

                    Label {
                        anchors.centerIn: parent
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        font.pixelSize: run.size(4)
                        font.bold: true
                        color: colours.text
                        text: qsTr("Tap to start")
                    }
                    Label {
                        y: parent.height / 2 + run.size(4) * 1.2
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: run.size(1)
                        color: colours.secondary
                        text: run.cur.ending === "duration" ? timeFormatter.countdown(run.cur.durationSec * 1000)
                            : WorkoutText.sessionPace(run.cur)
                    }
                }

                // A timed step: the digits with the progress bar under them (R-UI-7, R-UI-8).
                Item {
                    id: timedBlock
                    width: parent.width
                    height: parent.height
                    visible: stepGroup.timed

                    TimeDigits {
                        id: timedDigits
                        width: parent.width
                        height: implicitHeight
                        maxHeight: stepGroup.digitsRoom
                        color: colours.digits
                        outlineColor: colours.digitsOutline
                        showModeLabel: false
                        parts: run.countdown ? timeFormatter.countdownParts(sessionBridge.stepRemainingMs)
                                             : timeFormatter.elapsedParts(sessionBridge.stepPlannedMs - sessionBridge.stepRemainingMs)
                    }
                    ProgressStrip {
                        y: timedDigits.height + run.barGap
                        width: parent.width
                        height: run.barThickness
                        markColor: colours.text
                        mode: Settings.displayMode
                        remainingMs: sessionBridge.stepRemainingMs
                        periodMs: sessionBridge.stepPlannedMs
                        running: sessionBridge.running
                        fraction: sessionBridge.stepPlannedMs > 0 ? 1 - sessionBridge.stepRemainingMs / sessionBridge.stepPlannedMs : 0
                    }
                }

                // A repetition step without a tempo: the target count, large (R-TMR-3). The captions give way
                // before the count falls below its minimum share of the normal size: "Time so far" first.
                Item {
                    id: reps
                    anchors.fill: parent
                    visible: run.phaseKind === "reps"

                    readonly property real line: run.size(1) * 1.4
                    readonly property real minCount: run.maxDigitsHeight * run.minDigitsShare
                    readonly property bool showSoFar: height - 2 * line - run.gap >= minCount
                    readonly property bool showCaption: height - line - run.gap >= minCount

                    Label {
                        id: repsCaption
                        width: parent.width
                        height: reps.showCaption ? reps.line : 0
                        visible: reps.showCaption
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: run.size(1)
                        color: colours.secondary
                        text: qsTr("Repetitions to do")
                    }
                    LedText {
                        y: repsCaption.height
                        width: parent.width
                        height: parent.height - repsCaption.height - (reps.showSoFar ? reps.line + run.gap : 0)
                        color: colours.digits
                        text: String(run.cur.reps !== undefined ? run.cur.reps : 0)
                    }
                    Label {
                        y: parent.height - height
                        width: parent.width
                        height: reps.line
                        visible: reps.showSoFar
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: run.size(1)
                        color: colours.secondary
                        text: qsTr("Time so far: %1").arg(timeFormatter.elapsed(sessionBridge.stepElapsedMs))
                    }
                }

                // A paced repetition step: what the metronome run shows (R-WRK-10).
                TempoDisplay {
                    anchors.fill: parent
                    visible: run.phaseKind === "tempo"
                    fit: run.fit
                    repetition: sessionBridge.repetition
                    repetitionCount: sessionBridge.repetitionCount
                    phase: sessionBridge.phase
                    phaseCount: sessionBridge.phaseCount
                    phaseName: sessionBridge.phaseName
                    fraction: sessionBridge.repetitionFraction
                    starts: sessionBridge.phaseStarts
                    remainingMs: sessionBridge.stepRemainingMs
                }
            }
        }
    }
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import QtFeedback 5.0
import "../components"

// A metronome run (R-MET-1, R-MET-3 to R-MET-6), laid out from the screen size (R-UI-12). A
// full-screen flash on every beat (R-MET-4) sits behind the content; the content is the repetition
// number in the digit font (counting up, or the repetitions left; one tap on it switches, R-UI-8), the
// phase as a number and a text, the set position and the remaining time. The lead-in and the rest
// between sets show their own countdown (the rest with the bar of R-UI-7). Pause and resume are a
// tap; stop, add a set and remove a set are holds (R-TMR-11). In landscape the controls move to the
// right of the content.
Item {
    id: run

    property bool portrait: true
    // A try of a rhythm (R-MET-1): one set and no rest, so there is nothing to add or remove.
    property bool trying: false

    readonly property bool countdown: Settings.displayMode !== "direct"
    readonly property string section: metronomeBridge.section
    readonly property int phaseCount: metronomeBridge.phaseCount
    readonly property int phase: metronomeBridge.phase
    readonly property real gap: Metrics.mm(2)
    readonly property real buttonHeight: Metrics.buttonSize
    readonly property real barThickness: Metrics.mm(6.25)
    readonly property real overlap: -Metrics.mm(2)
    readonly property real titleSize: Metrics.labelSize * Metrics.runTitleFactor
    readonly property real textSize: Metrics.labelSize * Metrics.runTextFactor

    // The flash, behind everything, over the whole page.
    BeatPulse {
        id: pulse
        anchors.fill: parent
    }

    Connections {
        target: metronomeBridge
        onBeat: {
            pulse.fire(kind, atMs)
            Sounds.play(Sounds.clickOf(kind), atMs)
            if (Settings.vibration && vibrationOk.supported)
                (kind === "first" ? strongTick : weakTick).play()
        }
        onLeadInTick: Sounds.play("lead-in-tick")
        onRestTick: Sounds.play("tick")
        onRestStarted: Sounds.play("rest-start")
        onSetStarted: {
            if (set > 1)
                Sounds.play("round-change")
        }
        onFinished: Sounds.play("workout-complete")
        onStateChanged: {
            if (!metronomeBridge.running)
                pulse.clear()
        }
    }

    // Asked for `supported` only; the lengths of the effects are the platform's (R-ALR-4): the
    // weakest press for the beats between and the last one, the normal press for the first beat.
    ThemeEffect { id: vibrationOk; effect: ThemeEffect.PressWeak }
    ThemeEffect { id: weakTick; effect: ThemeEffect.PressWeak }
    ThemeEffect { id: strongTick; effect: ThemeEffect.Press }

    // 95 % of the page wide, ending at 95 % of its height.
    Item {
        id: frame
        x: run.width * 0.025
        y: run.height * 0.02
        width: run.width * 0.95
        height: run.height * 0.93

        readonly property real columnWidth: run.portrait ? width : Math.min(width * 0.36, Metrics.mm(60))
        readonly property real controlsHeight: run.trying ? run.buttonHeight
                                                         : 2 * run.buttonHeight + 2 * run.gap + run.textSize * 1.3

        // The name of the saved metronome that is run, above the set position; nothing for a run
        // from fresh settings.
        readonly property real nameBlock: metronomeBridge.name !== "" ? run.textSize * 1.4 : 0

        Label {
            id: savedName
            x: run.portrait ? 0 : frame.width - frame.columnWidth
            width: frame.columnWidth
            height: frame.nameBlock
            visible: metronomeBridge.name !== ""
            horizontalAlignment: run.portrait ? Text.AlignHCenter : Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: run.textSize
            color: Theme.secondaryHighlightColor
            truncationMode: TruncationMode.Fade
            text: metronomeBridge.name
        }

        Label {
            id: setLabel
            x: run.portrait ? 0 : frame.width - frame.columnWidth
            y: frame.nameBlock
            width: frame.columnWidth
            height: run.titleSize * 1.4
            horizontalAlignment: run.portrait ? Text.AlignHCenter : Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: run.titleSize
            font.bold: true
            color: Theme.highlightColor
            text: run.section === "leadin" ? qsTr("Get ready")
                : (run.section === "rest" ? qsTr("Rest")
                : (run.trying ? qsTr("Try") : qsTr("Set %1 of %2").arg(metronomeBridge.set).arg(metronomeBridge.setCount)))
        }

        // The content of the run, left of the controls in landscape.
        Item {
            id: region
            x: 0
            y: run.portrait ? frame.nameBlock + setLabel.height + run.gap : 0
            width: run.portrait ? frame.width : frame.width - frame.columnWidth - run.gap
            height: run.portrait ? frame.height - frame.nameBlock - setLabel.height - frame.controlsHeight - 2 * run.gap
                                 : frame.height

            // Lead-in and rest: the countdown with the digits large; the rest also has the bar.
            Item {
                id: waiting
                anchors.fill: parent
                visible: run.section !== "set"

                readonly property bool inRest: run.section === "rest"
                readonly property real waitLeft: inRest ? metronomeBridge.restRemainingMs : metronomeBridge.leadInRemainingMs
                readonly property real waitTotal: inRest ? metronomeBridge.restMs : 0

                Item {
                    id: waitBlock
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: (parent.height - height) / 2
                    width: parent.width
                    height: waitDigits.height + (waiting.inRest ? waitStrip.height - run.overlap : 0)

                    ProgressStrip {
                        id: waitStrip
                        visible: waiting.inRest
                        y: waitDigits.height - run.overlap
                        width: parent.width
                        height: run.barThickness
                        mode: Settings.displayMode
                        remainingMs: waiting.waitLeft
                        periodMs: waiting.waitTotal
                        running: metronomeBridge.running
                        fraction: waiting.waitTotal > 0 ? 1 - waiting.waitLeft / waiting.waitTotal : 0
                    }

                    TimeDigits {
                        id: waitDigits
                        width: parent.width
                        height: implicitHeight
                        maxHeight: Math.max(1, region.height * 0.7 - (waiting.inRest ? run.barThickness - run.overlap : 0))
                        showModeLabel: false
                        toggleOnTap: false
                        parts: timeFormatter.countdownParts(Math.ceil(waiting.waitLeft / 1000) * 1000)
                    }
                }
            }

            // A set: the repetition number, the phase and the bars (the shared display).
            TempoDisplay {
                id: playing
                anchors.fill: parent
                visible: run.section === "set"
                repetition: metronomeBridge.repetition
                repetitionCount: metronomeBridge.repetitionCount
                phase: run.phase
                phaseCount: run.phaseCount
                phaseName: metronomeBridge.phaseName
                fraction: metronomeBridge.repetitionFraction
                starts: metronomeBridge.phaseStarts
                remainingMs: metronomeBridge.remainingMs
            }
        }

        Column {
            id: controls
            spacing: run.gap
            x: run.portrait ? 0 : frame.width - frame.columnWidth
            y: frame.height - frame.controlsHeight
            width: frame.columnWidth

            Row {
                width: parent.width
                height: run.buttonHeight
                spacing: run.gap

                BigButton {
                    width: (parent.width - parent.spacing) / 2
                    height: parent.height
                    minimumSize: run.buttonHeight
                    text: metronomeBridge.running ? qsTr("Pause") : qsTr("Resume")
                    glyph: metronomeBridge.running ? "pause" : "play"
                    onActivated: {
                        if (metronomeBridge.running)
                            metronomeBridge.pause()
                        else
                            metronomeBridge.resume()
                    }
                }

                HoldButton {
                    width: (parent.width - parent.spacing) / 2
                    height: parent.height
                    minimumSize: run.buttonHeight
                    color: Theme.secondaryHighlightColor
                    text: qsTr("Hold to stop")
                    glyph: "stop"
                    onConfirmed: metronomeBridge.stop()
                }
            }

            Row {
                width: parent.width
                height: run.trying ? 0 : run.buttonHeight
                visible: !run.trying
                spacing: run.gap

                HoldButton {
                    width: (parent.width - parent.spacing) / 2
                    height: parent.height
                    minimumSize: run.buttonHeight
                    enabled: metronomeBridge.canAddSet
                    opacity: enabled ? 1 : 0.4
                    text: qsTr("Hold to add a set")
                    glyph: "add"
                    onConfirmed: metronomeBridge.addSet()
                }

                HoldButton {
                    width: (parent.width - parent.spacing) / 2
                    height: parent.height
                    minimumSize: run.buttonHeight
                    enabled: metronomeBridge.canRemoveSet
                    opacity: enabled ? 1 : 0.4
                    text: qsTr("Hold to remove a set")
                    glyph: "remove"
                    onConfirmed: metronomeBridge.removeSet()
                }
            }

            // The limits of the two buttons above.
            Label {
                width: parent.width
                height: run.trying ? 0 : run.textSize * 1.3
                visible: !run.trying
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: Math.max(Metrics.labelSize, run.textSize * 0.8)
                color: Theme.secondaryColor
                truncationMode: TruncationMode.Fade
                text: !metronomeBridge.canAddSet ? qsTr("At most 99 sets")
                    : (!metronomeBridge.canRemoveSet ? qsTr("No set left to remove") : "")
            }
        }
    }
}

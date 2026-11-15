// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// The countdown of the timer screen (R-MOD-3), laid out from the size it is given (R-UI-12): a
// label, the digits with the progress bar under them (overlapped a little), a row with the duration buttons while the
// countdown is ready (the digits and the bar take its space while it runs) and the two icon
// buttons (play or pause, reset). At zero it shows "Time is up" until it is reset. The digits shrink
// when the height is short.
Item {
    id: section

    Connections {
        target: countdownBridge
        onLastSecondsTick: Sounds.play("tick")
        onReachedZero: Sounds.play("timer-end")
    }

    readonly property bool countdown: Settings.displayMode !== "direct"
    readonly property bool finished: countdownBridge.state === 3
    readonly property bool ready: countdownBridge.state === 0
    // A short area scales the gaps, the bar and the buttons down (the buttons never below 9 mm).
    readonly property real fit: Math.max(0.55, Math.min(1, height / Metrics.mm(64)))
    readonly property real gap: Metrics.mm(2) * fit
    readonly property real labelHeight: Metrics.labelSize * 1.4
    readonly property real buttonHeight: Math.max(Metrics.mm(9), Metrics.buttonSize * fit)
    readonly property real barThickness: Metrics.mm(6.25) * fit
    // How far the bar reaches up behind the digits; negative is the distance between them.
    readonly property real overlap: -Metrics.mm(2) * fit
    readonly property real captionHeight: Metrics.labelSize * 1.3
    readonly property real infoHeight: captionHeight + buttonHeight
    // The row of the duration buttons exists only while the countdown is ready (and shows the
    // end message when it has finished); while it runs the digits and the bar take its space.
    readonly property bool infoShown: ready || finished
    readonly property real regionHeight: Math.max(0, height - labelHeight - buttonHeight - 2 * gap
                                                  - (infoShown ? infoHeight + gap : 0))

    function setDuration(total) {
        var clamped = Math.max(Settings.minCountdownSec, Math.min(Settings.maxCountdownSec, total))
        if (countdownBridge.setDuration(clamped))
            Settings.setCountdownSec(clamped)
    }

    Label {
        id: heading
        width: parent.width
        height: section.labelHeight
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: Metrics.labelSize
        color: Theme.secondaryHighlightColor
        text: qsTr("Countdown")
    }

    Item {
        id: region
        y: heading.height + section.gap
        width: parent.width
        height: section.regionHeight

        // The digits with the bar under them; the bar is drawn first and its upper part lies behind
        // the lower edge of the digits.
        Item {
            id: block
            anchors.centerIn: parent
            width: parent.width
            height: digits.height + strip.height - section.overlap

            ProgressStrip {
                id: strip
                y: digits.height - section.overlap
                width: parent.width
                height: section.barThickness
                mode: Settings.displayMode
                remainingMs: countdownBridge.remainingMs
                periodMs: countdownBridge.durationSec * 1000
                running: countdownBridge.running
                fraction: countdownBridge.durationSec > 0 ? countdownBridge.elapsedMs / (countdownBridge.durationSec * 1000) : 0
            }

            TimeDigits {
                id: digits
                width: parent.width
                height: implicitHeight
                maxHeight: Math.max(1, region.height - section.barThickness + section.overlap)
                showModeLabel: false
                parts: section.countdown ? timeFormatter.countdownParts(countdownBridge.remainingMs)
                                         : timeFormatter.elapsedParts(countdownBridge.elapsedMs)
            }
        }
    }

    // The duration while ready (three pairs of buttons: hours, minutes, seconds); the state
    // otherwise.
    Item {
        id: infoRow
        y: region.y + region.height + section.gap
        width: parent.width
        height: section.infoHeight
        visible: section.infoShown

        Row {
            anchors.fill: parent
            spacing: section.gap
            visible: section.ready

            Repeater {
                model: [
                    { caption: qsTr("Hours"), step: 3600, fewer: qsTr("Fewer hours"), more: qsTr("More hours") },
                    { caption: qsTr("Minutes"), step: 60, fewer: qsTr("Fewer minutes"), more: qsTr("More minutes") },
                    { caption: qsTr("Seconds"), step: 1, fewer: qsTr("Fewer seconds"), more: qsTr("More seconds") }
                ]
                delegate: Item {
                    width: (parent.width - 2 * section.gap) / 3
                    height: parent.height

                    Label {
                        width: parent.width
                        height: section.captionHeight
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: Metrics.labelSize
                        color: Theme.highlightColor
                        truncationMode: TruncationMode.Fade
                        text: modelData.caption
                    }
                    BigButton {
                        y: section.captionHeight
                        x: 0
                        width: (parent.width - section.gap / 2) / 2
                        height: section.buttonHeight
                        minimumSize: section.buttonHeight
                        repeating: true
                        text: modelData.fewer
                        glyph: "remove"
                        onActivated: section.setDuration(countdownBridge.durationSec - modelData.step)
                    }
                    BigButton {
                        y: section.captionHeight
                        x: parent.width - width
                        width: (parent.width - section.gap / 2) / 2
                        height: section.buttonHeight
                        minimumSize: section.buttonHeight
                        repeating: true
                        text: modelData.more
                        glyph: "add"
                        onActivated: section.setDuration(countdownBridge.durationSec + modelData.step)
                    }
                }
            }
        }

        Label {
            anchors.fill: parent
            visible: section.finished
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: Metrics.labelSize * 1.4
            font.bold: true
            color: StepStyle.color("work")
            text: qsTr("Time is up")
        }
    }

    Row {
        y: region.y + region.height + section.gap + (section.infoShown ? section.infoHeight + section.gap : 0)
        width: parent.width
        height: section.buttonHeight
        spacing: section.gap

        BigButton {
            width: (parent.width - parent.spacing) / 2
            height: parent.height
            minimumSize: section.buttonHeight
            enabled: !section.finished
            opacity: enabled ? 1 : 0.4
            text: section.ready ? qsTr("Start countdown")
                                : (countdownBridge.running ? qsTr("Pause countdown") : qsTr("Resume countdown"))
            glyph: countdownBridge.running ? "pause" : "play"
            onClicked: {
                if (section.ready)
                    countdownBridge.start()
                else if (countdownBridge.running)
                    countdownBridge.pause()
                else
                    countdownBridge.resume()
            }
        }

        // A running or paused countdown is reset with a hold; a finished one with a tap.
        HoldButton {
            width: (parent.width - parent.spacing) / 2
            height: parent.height
            minimumSize: section.buttonHeight
            enabled: !section.ready
            opacity: enabled ? 1 : 0.4
            holdRequired: countdownBridge.inProgress
            color: Theme.secondaryHighlightColor
            text: qsTr("Reset countdown")
            glyph: "refresh"
            onConfirmed: {
                countdownBridge.reset()
                // A reset returns to the count-down display so that a changed duration shows.
                if (Settings.displayMode === "direct")
                    Settings.setDisplayMode("countdown")
            }
        }
    }
}

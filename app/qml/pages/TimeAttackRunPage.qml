// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// A time attack run (R-MOD-4). The big round button in the lower half of the screen is the
// Start button first, then the Stop button, so the first tap starts the run. Stopping stores the
// result under the challenge and shows it, with a mark when it is a new best (also for the first
// result). Pause and resume are on a small button; a run paused by the app stays paused (R-MOD-5).
// Stopping a paused run stores the time reached at the pause. Leaving the page during a run is not
// possible: the run ends with the stop button.
Page {
    id: page

    // The result of the run that has just ended: {ok, durationMs, newBest}, empty before.
    property var result: ({})
    readonly property bool finished: result.durationMs !== undefined
    readonly property bool ready: timeAttackBridge.state === 0

    allowedOrientations: Orientation.All
    backNavigation: !timeAttackBridge.inProgress
    forwardNavigation: false

    Connections {
        target: timeAttackBridge
        onStarted: Sounds.play("work-start")
        onFinished: Sounds.play("timer-end")
    }

    KeepAwake {
        session: timeAttackBridge.inProgress
        onActiveLost: {
            if (timeAttackBridge.running)
                timeAttackBridge.pause()
        }
    }

    readonly property real gap: Metrics.mm(2)
    // The best time of the challenge as it was when the run was ready; it stays while the run goes
    // on and the stored best may change.
    property real bestMs: 0
    readonly property bool hasBest: bestMs > 0 && !finished
    // A short screen scales the bar and the best time down.
    readonly property real fit: Math.max(0.6, Math.min(1, height / Metrics.mm(150)))
    readonly property real barThickness: Metrics.mm(6.25) * fit
    readonly property real bestLabelHeight: Metrics.labelSize * 2.2 * fit
    readonly property real bestSpace: hasBest ? barThickness + bestLabelHeight + 2 * gap : 0
    // The number of the extra seconds past the best time that have pulsed.
    property int lastExtra: 0
    readonly property real elapsed: timeAttackBridge.elapsedMs

    Binding {
        target: page
        property: "bestMs"
        value: timeAttackBridge.challenge.bestMs === undefined ? 0 : timeAttackBridge.challenge.bestMs
        when: page.ready
    }

    // Once the best time is passed the bar pulses once a second, five times; then the time turns
    // the colour of the bar. Before that the bar follows the rules of the other timers, applied to
    // the best time.
    readonly property bool overBest: hasBest && elapsed > bestMs + 5000
    onElapsedChanged: {
        if (!hasBest || !timeAttackBridge.running)
            return
        var extra = Math.floor((elapsed - bestMs) / 1000)
        if (extra < 1) {
            lastExtra = 0
        } else if (extra <= 5 && extra > lastExtra) {
            lastExtra = extra
            bestBar.pulse(bestBar.bigPulse, 1000)
        }
    }

    readonly property real sideMargin: width * 0.05
    // In landscape the time and the small controls take the left half, the big button the right.
    readonly property real contentRight: isPortrait ? sideMargin : width * 0.5 + sideMargin
    // The big button is the accent colour; its text is black or white, whichever reads better.
    readonly property color accent: Theme.highlightColor
    readonly property real accentLuminance: {
        function channel(v) { return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4) }
        return 0.2126 * channel(accent.r) + 0.7152 * channel(accent.g) + 0.0722 * channel(accent.b)
    }
    readonly property color onAccent: accentLuminance > 0.18 ? "#000000" : "#ffffff"

    Label {
        id: title
        anchors {
            top: parent.top
            topMargin: page.height * 0.03
            left: parent.left
            right: parent.right
            leftMargin: page.sideMargin
            rightMargin: page.contentRight
        }
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: Metrics.labelSize * Metrics.runTitleFactor
        color: Theme.highlightColor
        truncationMode: TruncationMode.Fade
        text: timeAttackBridge.challenge.name === undefined ? "" : timeAttackBridge.challenge.name
    }

    // The time: the run so far, or the result.
    TimeDigits {
        id: digits
        anchors {
            top: title.bottom
            topMargin: page.gap
            left: parent.left
            right: parent.right
            leftMargin: page.sideMargin
            rightMargin: page.contentRight
        }
        height: implicitHeight
        maxHeight: Math.max(1, smallControls.y - 2 * page.gap - title.y - title.height - page.bestSpace)
        toggleOnTap: false
        showModeLabel: false
        color: page.overBest ? bestBar.barColor : Theme.primaryColor
        visible: !page.finished
        // Tenths of a second follow as one more small part.
        parts: timeFormatter.elapsedParts(timeAttackBridge.elapsedMs).concat(
                   [{ text: "." + Math.floor((timeAttackBridge.elapsedMs % 1000) / 100), large: false }])
    }

    // The best time with a bar that fills until it is reached.
    ProgressStrip {
        id: bestBar
        visible: page.hasBest
        anchors {
            top: digits.bottom
            topMargin: page.gap
            left: parent.left
            right: parent.right
            leftMargin: page.sideMargin
            rightMargin: page.contentRight
        }
        height: page.barThickness
        mode: "direct"
        sounds: true
        periodMs: page.bestMs
        remainingMs: Math.max(0, page.bestMs - page.elapsed)
        running: timeAttackBridge.running
        fraction: page.bestMs > 0 ? Math.min(1, page.elapsed / page.bestMs) : 0
    }
    LedText {
        visible: page.hasBest
        anchors {
            top: bestBar.bottom
            topMargin: page.gap
            left: parent.left
            right: parent.right
            leftMargin: page.sideMargin
            rightMargin: page.contentRight
        }
        height: page.bestLabelHeight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: Theme.primaryColor
        text: qsTr("best:  %1 !!!").arg(timeFormatter.precise(page.bestMs))
    }

    // The result of a finished run.
    Column {
        anchors {
            top: title.bottom
            topMargin: page.gap
            left: parent.left
            right: parent.right
            leftMargin: page.sideMargin
            rightMargin: page.contentRight
        }
        spacing: page.gap
        visible: page.finished

        // The result stays in the digit font, as large as the width allows.
        LedText {
            width: parent.width
            height: Metrics.labelSize * 5
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: Theme.primaryColor
            text: page.finished ? timeFormatter.precise(page.result.durationMs) : ""
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Metrics.labelSize * Metrics.runTitleFactor
            font.bold: true
            color: StepStyle.color("work")
            visible: page.finished && page.result.newBest
            text: qsTr("New best!")
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            color: Theme.highlightColor
            visible: page.finished && !page.result.ok
            text: qsTr("The result could not be saved.")
        }
    }

    // Small controls above the big button.
    Row {
        id: smallControls
        anchors {
            bottom: page.isPortrait ? bigButton.top : parent.bottom
            bottomMargin: page.isPortrait ? page.gap : page.height * 0.07
            left: parent.left
            right: parent.right
            leftMargin: page.sideMargin
            rightMargin: page.contentRight
        }
        height: Metrics.buttonSize
        spacing: page.gap

        BigButton {
            width: page.finished ? parent.width : (timeAttackBridge.inProgress ? parent.width : 0)
            height: parent.height
            minimumSize: Metrics.buttonSize
            fontPixelSize: Metrics.labelSize * Metrics.runTextFactor
            visible: page.finished || timeAttackBridge.inProgress
            text: page.finished ? qsTr("Run again") : (timeAttackBridge.running ? qsTr("Pause") : qsTr("Resume"))
            glyph: page.finished ? "refresh"
                                      : (timeAttackBridge.running ? "pause" : "play")
            onClicked: {
                if (page.finished)
                    page.result = ({})
                else if (timeAttackBridge.running)
                    timeAttackBridge.pause()
                else
                    timeAttackBridge.resume()
            }
        }
    }

    // The big button: a round button in the lower half of the screen in portrait, in the right half
    // in landscape; a tap anywhere in that area counts. Red with a white square while running.
    MouseArea {
        id: bigButton
        anchors {
            right: parent.right
            bottom: parent.bottom
            left: page.isPortrait ? parent.left : undefined
            top: page.isPortrait ? undefined : parent.top
        }
        width: page.isPortrait ? page.width : page.width * 0.5
        height: page.isPortrait ? page.height * 0.5 : page.height

        readonly property bool stopping: !page.finished && !page.ready
        readonly property real margin: Math.max(Metrics.mm(6), Math.min(width, height) * 0.08)
        readonly property real diameter: Math.min(width, height) - 2 * margin
        readonly property color stopColor: "#b3261e"
        readonly property color startColor: "#2e7d32"

        Accessible.role: Accessible.Button
        Accessible.name: page.ready ? qsTr("Start") : qsTr("Stop")
        // After the run has ended the button fades away; Run again brings it back.
        enabled: !page.finished
        opacity: page.finished ? 0 : 1
        Behavior on opacity { NumberAnimation { duration: 400 } }

        onClicked: {
            if (page.ready) {
                timeAttackBridge.start()
            } else {
                page.result = timeAttackBridge.stop()
            }
        }

        Rectangle {
            anchors.centerIn: parent
            width: bigButton.diameter
            height: width
            radius: width / 2
            color: {
                var base = bigButton.stopping ? bigButton.stopColor : bigButton.startColor
                return bigButton.pressed ? Qt.darker(base, 1.3) : base
            }

            // The stop symbol: a white square.
            Rectangle {
                anchors.centerIn: parent
                visible: bigButton.stopping
                width: parent.width * 0.36
                height: width
                radius: width * 0.08
                color: "#ffffff"
            }
            // The start symbol: a white play triangle.
            Glyph {
                anchors.centerIn: parent
                visible: !bigButton.stopping
                width: parent.width * 0.5
                height: width
                kind: "play"
                color: "#ffffff"
            }
        }
    }
}

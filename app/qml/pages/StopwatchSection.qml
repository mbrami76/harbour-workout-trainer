// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// The stopwatch of the timer screen (R-MOD-3), laid out from the size it is given (R-UI-12): a
// label, digits that count up with tenths of a second, always small (no bar, a stopwatch has no
// set time, R-UI-7) and the two icon buttons (play or pause, reset).
Item {
    id: section

    Connections {
        target: stopwatchBridge
        onStarted: Sounds.play("work-start")
    }

    readonly property bool ready: stopwatchBridge.state === 0
    // A short area scales the gaps and the buttons down (the buttons never below 9 mm).
    readonly property real fit: Math.max(0.55, Math.min(1, height / Metrics.mm(36)))
    readonly property real gap: Metrics.mm(2) * fit
    readonly property real labelHeight: Metrics.labelSize * 1.4
    readonly property real buttonHeight: Math.max(Metrics.mm(9), Metrics.buttonSize * fit)

    Label {
        id: heading
        width: parent.width
        height: section.labelHeight
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: Metrics.labelSize
        color: Theme.secondaryHighlightColor
        text: qsTr("Stopwatch")
    }

    Item {
        id: region
        y: heading.height + section.gap
        width: parent.width
        height: Math.max(0, section.height - section.labelHeight - section.buttonHeight - 2 * section.gap)

        TimeDigits {
            anchors.centerIn: parent
            width: parent.width
            height: implicitHeight
            maxHeight: Math.max(1, region.height)
            toggleOnTap: false
            showModeLabel: false
            // Tenths of a second follow as one more small part, whatever the size of the others.
            parts: timeFormatter.elapsedParts(stopwatchBridge.elapsedMs).concat(
                       [{ text: "." + Math.floor((stopwatchBridge.elapsedMs % 1000) / 100), large: false }])
        }
    }

    Row {
        y: region.y + region.height + section.gap
        width: parent.width
        height: section.buttonHeight
        spacing: section.gap

        BigButton {
            width: (parent.width - parent.spacing) / 2
            height: parent.height
            minimumSize: section.buttonHeight
            text: section.ready ? qsTr("Start stopwatch")
                                : (stopwatchBridge.running ? qsTr("Pause stopwatch") : qsTr("Resume stopwatch"))
            glyph: stopwatchBridge.running ? "pause" : "play"
            onClicked: {
                if (section.ready)
                    stopwatchBridge.start()
                else if (stopwatchBridge.running)
                    stopwatchBridge.pause()
                else
                    stopwatchBridge.resume()
            }
        }

        // A running or paused stopwatch is reset with a hold (R-TMR-11).
        HoldButton {
            width: (parent.width - parent.spacing) / 2
            height: parent.height
            minimumSize: section.buttonHeight
            enabled: !section.ready
            opacity: enabled ? 1 : 0.4
            color: Theme.secondaryHighlightColor
            text: qsTr("Reset stopwatch")
            glyph: "refresh"
            onConfirmed: stopwatchBridge.reset()
        }
    }
}

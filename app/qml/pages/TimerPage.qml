// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// The timer mode (R-MOD-3): a countdown and a stopwatch on one screen, apart or together. Neither
// writes a session log. The display stays on while either runs, also paused (R-TMR-7); both pause
// when the app is no longer active or the page is left and stay paused (R-TMR-8, R-TMR-9, R-MOD-5).
Page {
    id: page

    allowedOrientations: Orientation.All

    Component.onCompleted: {
        if (countdownBridge.state === 0)
            countdownBridge.setDuration(Settings.countdownSec)
        countdownBridge.setTickSeconds(Settings.tickSeconds)
    }

    // Leaving the page pauses a running timer; it does not resume by itself.
    onStatusChanged: {
        if (status === PageStatus.Deactivating) {
            if (countdownBridge.running)
                countdownBridge.pause()
            if (stopwatchBridge.running)
                stopwatchBridge.pause()
        }
    }

    KeepAwake {
        session: countdownBridge.inProgress || stopwatchBridge.inProgress
        onActiveLost: {
            if (countdownBridge.running)
                countdownBridge.pause()
            if (stopwatchBridge.running)
                stopwatchBridge.pause()
        }
    }

    // Everything sits in a frame that is 95 % of the page wide and ends at 95 % of its height
    // (R-UI-12).
    Item {
        id: frame
        x: page.width * 0.025
        y: page.height * 0.02
        width: page.width * 0.95
        height: page.height * 0.93

        // A short page scales the gaps and the buttons down (the buttons never below 9 mm).
        readonly property real fit: Math.max(0.6, Math.min(1, height / Metrics.mm(120)))
        readonly property real gap: Metrics.mm(2) * fit
        readonly property real buttonSize: Math.max(Metrics.mm(9), Metrics.buttonSize * fit)
        readonly property real areaHeight: height - bothRow.height - gap

        // The buttons for both timers, at the bottom and apart from the frames of the timers.
        Row {
            id: bothRow
            y: parent.height - height
            width: parent.width
            height: frame.buttonSize
            spacing: frame.gap

            BigButton {
                id: bothStart
                width: (parent.width - 2 * parent.spacing) / 3
                height: parent.height
                minimumSize: frame.buttonSize
                text: qsTr("Start both timers")
                glyph: "play"
                onClicked: {
                    if (countdownBridge.state === 0)
                        countdownBridge.start()
                    else if (countdownBridge.state === 2)
                        countdownBridge.resume()
                    if (stopwatchBridge.state === 0)
                        stopwatchBridge.start()
                    else if (stopwatchBridge.state === 2)
                        stopwatchBridge.resume()
                }
            }
            BigButton {
                width: bothStart.width
                height: parent.height
                minimumSize: frame.buttonSize
                text: qsTr("Pause both timers")
                glyph: "pause"
                onClicked: {
                    if (countdownBridge.running)
                        countdownBridge.pause()
                    if (stopwatchBridge.running)
                        stopwatchBridge.pause()
                }
            }
            // A timer in progress is reset with a hold, a finished one with a tap.
            HoldButton {
                width: bothStart.width
                height: parent.height
                minimumSize: frame.buttonSize
                enabled: countdownBridge.inProgress || stopwatchBridge.inProgress
                opacity: enabled ? 1 : 0.4
                holdRequired: (countdownBridge.inProgress && countdownBridge.state !== 3)
                              || stopwatchBridge.inProgress
                color: Theme.secondaryHighlightColor
                text: qsTr("Reset both timers")
                glyph: "refresh"
                onConfirmed: {
                    countdownBridge.reset()
                    stopwatchBridge.reset()
                    if (Settings.displayMode === "direct")
                        Settings.setDisplayMode("countdown")
                }
            }
        }

        // Stacked in portrait (the countdown gets the larger share), side by side in landscape. Each
        // timer sits inside a frame that separates its buttons from the ones for both timers.
        readonly property real inset: gap * 1.2
        readonly property real countdownWidth: page.isPortrait ? width : (width - gap) / 2
        readonly property real countdownHeight: page.isPortrait ? (areaHeight - gap) * 0.62 : areaHeight
        readonly property real stopwatchWidth: countdownWidth
        readonly property real stopwatchHeight: page.isPortrait ? (areaHeight - gap) * 0.38 : areaHeight
        readonly property real stopwatchX: page.isPortrait ? 0 : countdownWidth + gap
        readonly property real stopwatchY: page.isPortrait ? countdownHeight + gap : 0

        Rectangle {
            width: frame.countdownWidth
            height: frame.countdownHeight
            radius: frame.gap * 2
            color: "transparent"
            border.width: Math.max(1, Metrics.mm(0.4))
            border.color: Theme.rgba(Theme.primaryColor, 0.5)
        }
        CountdownSection {
            x: frame.inset
            y: frame.inset
            width: frame.countdownWidth - 2 * frame.inset
            height: frame.countdownHeight - 2 * frame.inset
        }
        Rectangle {
            x: frame.stopwatchX
            y: frame.stopwatchY
            width: frame.stopwatchWidth
            height: frame.stopwatchHeight
            radius: frame.gap * 2
            color: "transparent"
            border.width: Math.max(1, Metrics.mm(0.4))
            border.color: Theme.rgba(Theme.primaryColor, 0.5)
        }
        StopwatchSection {
            x: frame.stopwatchX + frame.inset
            y: frame.stopwatchY + frame.inset
            width: frame.stopwatchWidth - 2 * frame.inset
            height: frame.stopwatchHeight - 2 * frame.inset
        }
    }
}

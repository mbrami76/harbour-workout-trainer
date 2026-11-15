// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "."

// One of the three buttons of the session screen (R-UI-2): a symbol drawn in the symbol colour of the
// scheme, no text. A press that ends within Metrics.tapMaxMs is a tap; with `holdEnabled` a press
// that lasts Metrics.holdMs is a hold, with a progress that fills while the finger stays (R-TMR-11),
// and a press that ends in between is cancelled with no effect. `text` is the accessible name only.
// With `pulsing` a soft green halo breathes behind the button.
MouseArea {
    id: button

    property var scheme
    property string text
    property string glyph
    property bool tapEnabled: false
    property bool holdEnabled: false
    property bool pulsing: false
    property real iconSize: Metrics.mm(6)
    // 0 to 1, the share of the hold that has passed.
    property real progress: 0
    property bool fired: false
    property real pressedAt: 0

    signal tapped()
    signal held()

    enabled: tapEnabled || holdEnabled
    opacity: enabled ? 1 : 0.4

    onPressed: {
        fired = false
        pressedAt = Date.now()
        if (holdEnabled)
            fill.restart()
    }
    onReleased: finish()
    onCanceled: reset()
    onProgressChanged: {
        if (progress >= 1 && !fired) {
            fired = true
            fill.stop()
            progress = 0
            held()
        }
    }

    function reset() {
        fill.stop()
        progress = 0
    }

    function finish() {
        var wasHold = fired
        var quick = Date.now() - pressedAt <= Metrics.tapMaxMs
        reset()
        if (!wasHold && quick && tapEnabled)
            tapped()
    }

    NumberAnimation {
        id: fill
        target: button
        property: "progress"
        from: 0
        to: 1
        duration: Metrics.holdMs
    }

    // The soft halo that calls the user while a step waits for the tap (R-UI-7).
    Rectangle {
        id: halo
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        radius: Theme.paddingSmall
        visible: button.pulsing
        color: "transparent"
        border.width: Math.max(2, Metrics.mm(1))
        border.color: "#4fc14a"
        opacity: 0
        scale: 1

        SequentialAnimation {
            running: button.pulsing
            loops: Animation.Infinite
            ParallelAnimation {
                NumberAnimation { target: halo; property: "scale"; from: 1; to: 1.18; duration: 900; easing.type: Easing.OutQuad }
                NumberAnimation { target: halo; property: "opacity"; from: 0.8; to: 0; duration: 900 }
            }
            PauseAnimation { duration: 300 }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.paddingSmall
        color: button.scheme.buttonFill
        border.width: Math.max(1, Metrics.mm(0.3))
        border.color: button.scheme.buttonBorder
        clip: true

        Rectangle {
            width: parent.width * button.progress
            height: parent.height
            color: Theme.rgba(button.scheme.buttonBorder, 0.55)
        }
    }

    Glyph {
        anchors.centerIn: parent
        width: button.iconSize
        height: button.iconSize
        kind: button.glyph
        color: button.scheme.symbol
    }

    Accessible.role: Accessible.Button
    Accessible.name: button.text
}

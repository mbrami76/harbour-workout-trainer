// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "."

// A button that must be held (R-TMR-11), or tapped when `holdRequired` is false (a timer that is idle
// or finished): a visible progress fills while the finger stays on it,
// releasing early cancels with no effect, and `confirmed` is emitted once when the hold is
// complete. It is at least 9 mm high (R-UI-5): the larger of the theme's large item size and nine
// millimetres as the platform reports them (screenMetrics, from the physical dots per inch).
MouseArea {
    id: button

    property string text
    // How long the button must be held: the time of every long press (R-TMR-11).
    readonly property int holdMs: Metrics.holdMs
    property color color: Theme.highlightColor
    // 0 to 1, the share of the hold that has passed.
    property real progress: 0

    // False for a button whose action needs only a tap, for example the reset of a finished timer.
    property bool holdRequired: true

    signal confirmed()

    property real minimumSize: Math.max(Theme.itemSizeLarge, 9 * screenMetrics.pixelsPerMm)
    // Pixel size of the text; the timer pages set it from the screen.
    property real fontPixelSize: Theme.fontSizeLarge
    // A symbol instead of the text, one of the glyphs, for example "play". The text then serves
    // as the accessible name only.
    property string glyph
    property real iconSize: Metrics.mm(6)
    // Set when the hold completed, until the finger is lifted: the signal comes once per press.
    property bool fired: false

    implicitWidth: glyph !== "" ? minimumSize : Math.max(label.implicitWidth + 2 * Theme.paddingLarge, minimumSize)
    implicitHeight: minimumSize

    onPressed: {
        fired = false
        if (holdRequired)
            fill.restart()
    }
    onClicked: {
        if (!holdRequired)
            confirmed()
    }
    onReleased: cancel()
    onCanceled: cancel()
    onProgressChanged: {
        if (progress >= 1 && !fired) {
            fired = true
            fill.stop()
            progress = 0
            confirmed()
        }
    }

    function cancel() {
        fill.stop()
        progress = 0
    }

    NumberAnimation {
        id: fill
        target: button
        property: "progress"
        from: 0
        to: 1
        duration: button.holdMs
    }

    Rectangle {
        id: background
        anchors.fill: parent
        radius: Theme.paddingSmall
        color: Theme.rgba(button.color, 0.15)
        border.width: Math.max(1, Theme.paddingSmall / 4)
        border.color: Theme.rgba(button.color, 0.7)
        clip: true

        Rectangle {
            width: parent.width * button.progress
            height: parent.height
            color: Theme.rgba(button.color, 0.55)
        }
    }

    Glyph {
        anchors.centerIn: parent
        visible: button.glyph !== ""
        width: button.iconSize
        height: button.iconSize
        kind: button.glyph
        color: button.enabled ? Theme.primaryColor : Theme.secondaryColor
    }

    Accessible.role: Accessible.Button
    Accessible.name: button.text

    Label {
        id: label
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.paddingMedium
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: button.fontPixelSize
        color: Theme.primaryColor
        visible: button.glyph === ""
        text: button.text
        truncationMode: TruncationMode.Fade
    }
}

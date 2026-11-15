// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "."

// A plain tap button for the controls of a timer. With `repeating` it emits `activated` on the
// press and again about ten times a second after half a second of holding; without it,
// `activated` comes with the tap. It is at least 9 mm high (R-UI-5): the larger
// of the theme's large item size and nine millimetres as the platform reports them.
MouseArea {
    id: button

    property string text
    property color color: Theme.highlightColor
    property bool repeating: false
    // A chosen one of a group of buttons: filled stronger.
    property bool selected: false

    signal activated()

    property real minimumSize: Math.max(Theme.itemSizeLarge, 9 * screenMetrics.pixelsPerMm)
    // Pixel size of the text; the timer pages set it from the screen.
    property real fontPixelSize: Theme.fontSizeLarge
    // A symbol instead of the text, one of the glyphs, for example "play". The text then serves
    // as the accessible name only.
    property string glyph
    property real iconSize: Metrics.mm(6)

    onPressed: {
        if (repeating) {
            activated()
            repeatDelay.restart()
        }
    }
    onReleased: stopRepeat()
    onCanceled: stopRepeat()
    onClicked: {
        if (!repeating)
            activated()
    }

    function stopRepeat() {
        repeatDelay.stop()
        repeatTimer.stop()
    }

    Timer {
        id: repeatDelay
        interval: 500
        onTriggered: repeatTimer.start()
    }
    Timer {
        id: repeatTimer
        interval: 100
        repeat: true
        onTriggered: button.activated()
    }

    implicitWidth: glyph !== "" ? minimumSize : Math.max(label.implicitWidth + 2 * Theme.paddingLarge, minimumSize)
    implicitHeight: minimumSize

    Rectangle {
        anchors.fill: parent
        radius: Theme.paddingSmall
        color: Theme.rgba(button.color, (button.pressed || button.selected) ? 0.45 : 0.2)
        border.width: Math.max(1, Theme.paddingSmall / 4) * (button.selected ? 2 : 1)
        border.color: Theme.rgba(button.color, button.selected ? 1 : 0.7)
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

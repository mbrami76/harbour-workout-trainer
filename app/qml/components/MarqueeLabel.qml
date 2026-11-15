// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0

// A single line of text that stays still when it fits and scrolls along when it does not. The text
// runs out at the left, restarts from the right and loops, with a pause before each pass.
Item {
    id: marquee

    property string text
    property color color: Theme.secondaryColor
    property real pixelSize: Theme.fontSizeSmall
    readonly property bool overflowing: label.implicitWidth > width + 1

    implicitHeight: label.implicitHeight
    clip: true

    Label {
        id: label
        text: marquee.text
        color: marquee.color
        font.pixelSize: marquee.pixelSize
        wrapMode: Text.NoWrap
        truncationMode: TruncationMode.None
    }

    SequentialAnimation {
        running: marquee.overflowing && marquee.visible
        loops: Animation.Infinite
        onRunningChanged: if (!running) label.x = 0
        PauseAnimation { duration: 1500 }
        NumberAnimation {
            target: label
            property: "x"
            from: 0
            to: marquee.width - label.implicitWidth - Theme.paddingLarge
            duration: Math.max(1500, (label.implicitWidth - marquee.width) * 25)
        }
        PauseAnimation { duration: 1200 }
        PropertyAction { target: label; property: "x"; value: 0 }
    }
}

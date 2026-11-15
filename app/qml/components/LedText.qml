// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.4
import Sailfish.Silica 1.0
import "."

// A line of text in the digit font with the unlit segments shown dimly behind it (R-UI-10): the
// same text with every letter and digit replaced by an 8, in the text colour at low strength, drawn
// under the text with the same size and alignment. The unlit segments appear only in the
// seven-segment font. The text is fitted to the item.
Item {
    id: led

    property string text
    property color color: Theme.primaryColor
    property int horizontalAlignment: Text.AlignHCenter
    property int verticalAlignment: Text.AlignVCenter
    // Strength of the unlit segments, as a share of the text colour.
    property real ghostOpacity: DigitFont.unlitStrength

    readonly property string ghostText: text.replace(/[A-Za-z0-9]/g, "8")

    Text {
        anchors.fill: parent
        visible: DigitFont.family === DigitFont.segmentFamily
        horizontalAlignment: led.horizontalAlignment
        verticalAlignment: led.verticalAlignment
        font.family: DigitFont.family
        font.pixelSize: led.height
        fontSizeMode: Text.Fit
        minimumPixelSize: 1
        color: Theme.rgba(led.color, led.ghostOpacity)
        text: led.ghostText
    }
    Text {
        anchors.fill: parent
        horizontalAlignment: led.horizontalAlignment
        verticalAlignment: led.verticalAlignment
        font.family: DigitFont.family
        font.pixelSize: led.height
        fontSizeMode: Text.Fit
        minimumPixelSize: 1
        color: led.color
        text: led.text
    }
}

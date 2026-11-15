// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.4
import Sailfish.Silica 1.0
import "."

// The digits of a time on one line (R-UI-1, R-UI-8 to R-UI-11). `parts` comes from timeFormatter
// (the parts of the core): each part has its text with the separator and tells whether it is
// large. The large part is drawn as wide as the item allows (and not higher than maxHeight), the
// small parts at half that size on the same baseline. Every character sits in a cell of fixed
// width, so nothing shifts while a digit changes. Every character has a thin outline that contrasts
// with the text: black around white text on a dark ambience, white around black text on a light one,
// so it stands out from the bar behind it. A tap switches between
// count-down and direct display (Settings.displayMode).
Item {
    id: digits

    // [{text, large}], for example [{text: "05", large: true}, {text: ":09", large: false}].
    property var parts: []
    property color color: Theme.primaryColor
    // Upper bound of the size of the large part, in pixels.
    property real maxFontSize: 10000
    // Upper bound of the height of the line of digits, 0 for none.
    property real maxHeight: 0
    // The colour of the outline: black for light text, white for dark text.
    property color outlineColor: luminance(color) > 0.4 ? "#000000" : "#ffffff"
    // Strength of the unlit segments, as a share of the text colour.
    property real ghostOpacity: DigitFont.unlitStrength
    property bool toggleOnTap: true
    property bool showModeLabel: true
    // The width of the outline as a share of the size of a character.
    readonly property real outlineRatio: 0.012
    // Share of the large size that the small parts get.
    readonly property real smallRatio: 0.5
    readonly property string mode: Settings.displayMode

    function luminance(c) {
        function channel(v) { return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4) }
        return 0.2126 * channel(c.r) + 0.7152 * channel(c.g) + 0.0722 * channel(c.b)
    }

    // Advances at a reference size of 100 px, in the font that is in use.
    FontMetrics {
        id: reference
        font.pixelSize: 100
        font.family: DigitFont.family
    }
    readonly property real digitAdvance: reference.advanceWidth("0")
    readonly property real colonAdvance: reference.advanceWidth(":")

    // The colon and the decimal point take the narrow cell.
    function isSeparator(character) {
        return character === ":" || character === "."
    }

    // The characters in order with their size class.
    readonly property var cells: {
        var list = []
        for (var i = 0; i < parts.length; ++i) {
            for (var j = 0; j < parts[i].text.length; ++j)
                list.push({ character: parts[i].text.charAt(j), large: parts[i].large })
        }
        return list
    }

    // Width of the line in units of the size of the large part.
    readonly property real emWidth: {
        var w = 0
        for (var i = 0; i < cells.length; ++i) {
            var advance = (isSeparator(cells[i].character) ? colonAdvance : digitAdvance) / 100
            w += cells[i].large ? advance : advance * smallRatio
        }
        return w
    }

    // Height of the line of the reference size, in units of the size.
    readonly property real lineRatio: reference.height / 100

    // The size of the large part: as big as the width allows, up to the upper bounds.
    readonly property real fontSize: {
        var size = Math.min(maxFontSize, width / Math.max(0.1, emWidth))
        if (maxHeight > 0)
            size = Math.min(size, maxHeight / Math.max(0.1, lineRatio))
        return Math.max(1, size)
    }
    readonly property real smallSize: fontSize * smallRatio
    // The baseline of the line of digits, from the top of the item.
    readonly property real baselineY: largeMetrics.ascent

    FontMetrics {
        id: largeMetrics
        font.pixelSize: digits.fontSize
        font.family: DigitFont.family
    }
    FontMetrics {
        id: smallMetrics
        font.pixelSize: digits.smallSize
        font.family: DigitFont.family
    }

    implicitHeight: row.height + (showModeLabel ? modeLabel.height + Metrics.mm(1) : 0)

    Row {
        id: row
        anchors.horizontalCenter: parent.horizontalCenter
        height: largeMetrics.height
        Repeater {
            model: digits.cells.length
            delegate: Item {
                id: cellItem
                readonly property var cell: digits.cells[index]
                readonly property real size: cell.large ? digits.fontSize : digits.smallSize
                // The outline is four copies of the character in the outline colour, shifted
                // diagonally; the Outline style of each adds a pixel all round.
                readonly property real shift: Math.max(1, size * digits.outlineRatio)
                width: (digits.isSeparator(cell.character) ? digits.colonAdvance : digits.digitAdvance) * size / 100
                height: largeMetrics.height
                // Small characters share the baseline of the large ones.
                y: cell.large ? 0 : largeMetrics.ascent - smallMetrics.ascent

                // The unlit segments: an 8 in a faint text colour under the character.
                Text {
                    width: cellItem.width
                    horizontalAlignment: Text.AlignHCenter
                    visible: DigitFont.family === DigitFont.segmentFamily
                    font.pixelSize: cellItem.size
                    font.family: DigitFont.family
                    color: Theme.rgba(digits.color, digits.ghostOpacity)
                    text: digits.isSeparator(cellItem.cell.character) ? cellItem.cell.character : "8"
                }
                Repeater {
                    model: [[-1, -1], [1, -1], [-1, 1], [1, 1]]
                    delegate: Text {
                        x: modelData[0] * cellItem.shift
                        y: modelData[1] * cellItem.shift
                        width: cellItem.width
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: cellItem.size
                        font.family: DigitFont.family
                        color: digits.outlineColor
                        style: Text.Outline
                        styleColor: digits.outlineColor
                        text: cellItem.cell.character
                    }
                }
                Text {
                    width: cellItem.width
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: cellItem.size
                    font.family: DigitFont.family
                    color: digits.color
                    text: cellItem.cell.character
                }
            }
        }
    }

    Label {
        id: modeLabel
        visible: digits.showModeLabel
        anchors {
            top: row.bottom
            topMargin: Metrics.mm(1)
            horizontalCenter: parent.horizontalCenter
        }
        font.pixelSize: Metrics.labelSize
        color: Theme.secondaryColor
        text: digits.mode === "direct" ? qsTr("Time elapsed") : qsTr("Time left")
    }

    MouseArea {
        anchors.fill: parent
        enabled: digits.toggleOnTap
        onClicked: Settings.setDisplayMode(digits.mode === "direct" ? "countdown" : "direct")
    }
}

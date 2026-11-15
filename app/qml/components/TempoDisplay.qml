// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "."

// What a paced set shows (R-MET-4, R-WRK-10), shared by the metronome run and the workout session: the
// repetition number in the digit font (counting up, or the repetitions left; one tap switches,
// R-UI-8), the name of the phase with its position, the repetition bar with its numbered diamonds
// and the remaining time. Sized from the item and from physical units, never from the system font
// size (R-UI-12). The item is meant to fill the region it is given.
Item {
    id: display

    property int repetition: 0
    property int repetitionCount: 0
    property int phase: 0
    property int phaseCount: 0
    property string phaseName
    // The elapsed share of the running repetition, 0 to 1.
    property real fraction: 0
    // Where each phase starts in a repetition, as shares 0 to 1.
    property var starts: [0]
    property real remainingMs: 0
    // Shrinks the texts on a short screen; the bar and its diamonds keep their size (R-MET-4).
    property real fit: 1
    // A low region (landscape, or a short screen): the repetition number, the phase and the
    // remaining time sit side by side above the bar.
    property bool compact: height < Metrics.mm(50)

    readonly property bool countdown: Settings.displayMode !== "direct"
    readonly property real gap: Metrics.mm(2)
    readonly property real titleSize: Math.max(Metrics.labelSize, Metrics.labelSize * Metrics.runTitleFactor * fit)
    readonly property real textSize: Math.max(Metrics.labelSize, Metrics.labelSize * Metrics.runTextFactor * fit)
    readonly property real lineHeight: textSize * 1.4
    readonly property real nameHeight: titleSize * 1.3
    readonly property real phaseSpace: phaseCount > 0 ? nameHeight + gap : 0
    readonly property real remainingHeight: height * 0.13
    // What the repetition number may take: the rest of the height.
    // A low region: the top row holds the repetition number, its caption and the remaining time on
    // one centre line, the phase line is the second row and the bar the last, all from the same left
    // edge (R-UI-14).
    readonly property real rowHeight: Math.max(1, height - repetitionBar.height - (phaseCount > 0 ? nameHeight + gap : 0) - 2 * gap)
    readonly property real repHeight: compact ? rowHeight : Math.max(height * 0.12, height - lineHeight * 2 - phaseSpace
                                               - repetitionBar.height - remainingHeight - 4 * gap)

    Label {
        id: repCaption
        visible: !display.compact
        width: parent.width
        height: display.lineHeight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: display.textSize
        color: Theme.secondaryHighlightColor
        text: display.countdown ? qsTr("Repetitions left") : qsTr("Repetition")
    }

    LedText {
        id: repNumber
        y: display.compact ? 0 : repCaption.height
        width: display.compact ? parent.width * 0.3 : parent.width
        height: display.repHeight
        color: Theme.primaryColor
        // Counting down shows the repetitions left including the one that is on.
        text: String(display.countdown ? display.repetitionCount - display.repetition + 1 : display.repetition)

        MouseArea {
            anchors.fill: parent
            onClicked: Settings.setDisplayMode(display.countdown ? "direct" : "countdown")
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Switch between repetitions done and repetitions left")
        }
    }

    Label {
        id: ofLabel
        x: display.compact ? repNumber.width + display.gap : 0
        y: display.compact ? (display.rowHeight - height) / 2 : repNumber.y + repNumber.height
        width: display.compact ? parent.width * 0.2 : parent.width
        height: display.lineHeight
        horizontalAlignment: display.compact ? Text.AlignLeft : Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: display.textSize
        color: Theme.secondaryColor
        text: display.compact && display.countdown ? qsTr("left of %1").arg(display.repetitionCount)
                                                     : qsTr("of %1").arg(display.repetitionCount)
    }

    // The name of the phase, or "Phase n" without one, large and fitted to the width, with its
    // position beside it (R-MET-4). The colour of the flash is never the only cue (R-UI-3).
    Item {
        id: phaseLine
        x: 0
        y: display.compact ? display.rowHeight + display.gap : ofLabel.y + ofLabel.height + display.gap
        width: parent.width
        height: display.nameHeight
        visible: display.phaseCount > 0

        Label {
            id: positionLabel
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: display.textSize
            color: Theme.secondaryColor
            text: qsTr("%1 of %2").arg(display.phase).arg(display.phaseCount)
        }

        Label {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - positionLabel.width - display.gap
            height: parent.height
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: parent.height * 0.8
            fontSizeMode: Text.Fit
            minimumPixelSize: Metrics.labelSize
            font.bold: true
            color: Theme.highlightColor
            text: display.phaseName !== "" ? display.phaseName : qsTr("Phase %1").arg(display.phase)
        }
    }

    RepetitionBar {
        id: repetitionBar
        y: display.compact ? parent.height - height
                           : (display.phaseCount > 0 ? phaseLine.y + phaseLine.height : ofLabel.y + ofLabel.height) + display.gap
        width: parent.width
        fraction: display.fraction
        starts: display.starts.length > 0 ? display.starts : [0]
        // Without phases the single diamond is reached as the repetition begins; with phases the
        // first `phase` diamonds are.
        reached: display.phaseCount > 0 ? display.phase : 1
    }

    TimeDigits {
        x: display.compact ? repNumber.width + ofLabel.width + 2 * display.gap : 0
        y: display.compact ? (display.rowHeight - height) / 2 : parent.height - height
        width: parent.width - x
        height: implicitHeight
        maxHeight: display.compact ? display.rowHeight : display.remainingHeight
        showModeLabel: false
        toggleOnTap: false
        parts: timeFormatter.countdownParts(display.remainingMs)
    }
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// A run in progress, laid out from the screen size (R-UI-12): the period label (the kind is also
// shown by colour, R-UI-3), the digits of the current period with a progress bar under them, the
// number of completed repetitions below the bar in the digit font, pause and resume with a tap
// and stop with a hold (R-TMR-11),
// both as icon buttons. In landscape the controls move to the right of the digits.
Item {
    id: run

    property bool portrait: true

    // The sounds start from the events of the core (R-ALR-1, R-ALR-12).
    Connections {
        target: intervalBridge
        onPeriodStarted: Sounds.play(kind === "work" ? "work-start" : "rest-start")
        onLastSecondsTick: Sounds.play("tick")
    }

    readonly property bool countdown: Settings.displayMode !== "direct"
    readonly property string kind: intervalBridge.periodKind
    readonly property color kindColor: StepStyle.color(kind)
    readonly property real gap: Metrics.mm(2)
    readonly property real buttonHeight: Metrics.buttonSize
    readonly property real barThickness: Metrics.mm(6.25)
    readonly property real overlap: -Metrics.mm(2)
    readonly property real titleSize: Metrics.labelSize * Metrics.runTitleFactor

    // 95 % of the page wide, ending at 95 % of its height.
    Item {
        id: frame
        x: run.width * 0.025
        y: run.height * 0.02
        width: run.width * 0.95
        height: run.height * 0.93

        // Portrait: the label on top, the digits and bar below it, the controls at the bottom. Landscape: the
        // digits on the left, the label at the top and the controls at the bottom of a column on the right.
        readonly property real columnWidth: run.portrait ? width : Math.min(width * 0.36, Metrics.mm(60))
        readonly property real controlsHeight: run.buttonHeight

        Label {
            id: periodLabel
            x: run.portrait ? 0 : frame.width - frame.columnWidth
            y: 0
            width: frame.columnWidth
            height: run.titleSize * 1.4
            horizontalAlignment: run.portrait ? Text.AlignHCenter : Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: run.titleSize
            font.bold: true
            color: run.kindColor
            text: StepStyle.label(run.kind)
        }

        Item {
            id: region
            x: 0
            y: run.portrait ? periodLabel.height + run.gap : 0
            width: run.portrait ? frame.width : frame.width - frame.columnWidth - run.gap
            height: run.portrait ? frame.height - periodLabel.height - frame.controlsHeight - 2 * run.gap
                                 : frame.height

            // The repetition counter below the bar, right aligned, half as wide as the digits. A
            // repetition counts when its recovery has finished (or its work period, without recovery).
            readonly property int repetitions: intervalBridge.completedWork - (run.kind === "recovery" ? 1 : 0)
            readonly property real repHeight: height * 0.16
            readonly property real repSpace: repHeight + run.gap

            LedText {
                id: repetitions
                x: parent.width - width - Metrics.mm(3)
                y: block.y + block.height + run.gap
                width: parent.width * 0.5
                height: region.repHeight
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                color: Theme.primaryColor
                text: qsTr("r: %1").arg(region.repetitions < 10 ? "0" + region.repetitions : region.repetitions)
            }

            Item {
                id: block
                anchors.horizontalCenter: parent.horizontalCenter
                y: (parent.height - region.repSpace - height) / 2
                width: parent.width
                height: digits.height + strip.height - run.overlap

                ProgressStrip {
                    id: strip
                    y: digits.height - run.overlap
                    width: parent.width
                    height: run.barThickness
                    mode: Settings.displayMode
                    remainingMs: intervalBridge.remainingMs
                    periodMs: intervalBridge.periodMs
                    running: intervalBridge.running
                    fraction: intervalBridge.periodMs > 0 ? intervalBridge.elapsedMs / intervalBridge.periodMs : 0
                }

                TimeDigits {
                    id: digits
                    width: parent.width
                    height: implicitHeight
                    maxHeight: Math.max(1, region.height - region.repSpace - run.barThickness + run.overlap)
                    showModeLabel: false
                    parts: run.countdown ? timeFormatter.countdownParts(intervalBridge.remainingMs)
                                         : timeFormatter.elapsedParts(intervalBridge.elapsedMs)
                }
            }
        }

        Column {
            id: controls
            spacing: run.gap
            x: run.portrait ? 0 : frame.width - frame.columnWidth
            y: frame.height - frame.controlsHeight
            width: frame.columnWidth

            Row {
                width: parent.width
                height: run.buttonHeight
                spacing: run.gap

                BigButton {
                    width: (parent.width - parent.spacing) / 2
                    height: parent.height
                    minimumSize: run.buttonHeight
                    text: intervalBridge.running ? qsTr("Pause") : qsTr("Resume")
                    glyph: intervalBridge.running ? "pause" : "play"
                    onClicked: {
                        if (intervalBridge.running)
                            intervalBridge.pause()
                        else
                            intervalBridge.resume()
                    }
                }

                HoldButton {
                    width: (parent.width - parent.spacing) / 2
                    height: parent.height
                    minimumSize: run.buttonHeight
                    color: Theme.secondaryHighlightColor
                    text: qsTr("Hold to stop")
                    glyph: "stop"
                    onConfirmed: intervalBridge.stop()
                }
            }
        }
    }
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0

// A whole number of seconds set with four big buttons (minus 10, minus 1, plus 1, plus 10) instead
// of a keyboard. A button held down repeats. The value stays inside minimum and maximum.
Item {
    id: stepper

    property string label
    property int value: 0
    property int minimum: 0
    property int maximum: 3600
    // The value as shown, from timeFormatter.
    property string valueText
    // The steps of the buttons, from the smallest to the largest (four by default).
    property var steps: [-10, -1, 1, 10]

    signal edited(int value)

    height: column.height
    width: parent ? parent.width : Screen.width

    function change(delta) {
        var next = Math.max(minimum, Math.min(maximum, value + delta))
        if (next !== value) {
            value = next
            edited(next)
        }
    }

    Column {
        id: column
        width: parent.width
        spacing: Theme.paddingMedium

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            color: Theme.secondaryHighlightColor
            font.pixelSize: Theme.fontSizeMedium
            text: stepper.label
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            font.pixelSize: Theme.fontSizeHuge
            color: Theme.highlightColor
            text: stepper.valueText
        }

        Row {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            spacing: Theme.paddingMedium

            Repeater {
                model: stepper.steps
                delegate: BigButton {
                    width: (parent.width - (stepper.steps.length - 1) * Theme.paddingMedium) / stepper.steps.length
                    // Minus is U+2212. The sign and the number are one string for the translator.
                    text: modelData < 0 ? "\u2212" + (-modelData) : "+" + modelData
                    onPressed: {
                        stepper.change(modelData)
                        repeatDelay.restart()
                    }
                    onReleased: stop()
                    onCanceled: stop()

                    function stop() {
                        repeatDelay.stop()
                        repeatTimer.stop()
                    }

                    // After a pause of half a second a held button repeats about ten times a second.
                    Timer {
                        id: repeatDelay
                        interval: 500
                        onTriggered: repeatTimer.start()
                    }
                    Timer {
                        id: repeatTimer
                        interval: 100
                        repeat: true
                        onTriggered: stepper.change(modelData)
                    }
                }
            }
        }
    }
}

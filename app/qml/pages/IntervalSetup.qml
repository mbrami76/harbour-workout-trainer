// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// The two durations of the interval timer, set with big steppers, and the Start button. The values
// start from the last use (R-MOD-2) and are saved when a run starts.
SilicaFlickable {
    id: setup

    property bool portrait: true
    property int workSec: Settings.intervalWorkSec
    property int recoverySec: Settings.intervalRecoverySec

    contentHeight: column.height + Theme.paddingLarge

    Column {
        id: column
        width: parent.width
        spacing: Theme.paddingLarge

        PageHeader { title: qsTr("Interval timer") }

        // One below the other in portrait, side by side in landscape, so that Start stays in view.
        Grid {
            width: parent.width
            columns: setup.portrait ? 1 : 2
            spacing: Theme.paddingLarge

            NumberStepper {
                width: (parent.width - (parent.columns - 1) * parent.spacing) / parent.columns
                label: qsTr("Work")
                minimum: Settings.minIntervalWorkSec
                maximum: Settings.maxIntervalWorkSec
                value: setup.workSec
                valueText: timeFormatter.elapsed(value * 1000)
                onEdited: setup.workSec = value
            }

            NumberStepper {
                width: (parent.width - (parent.columns - 1) * parent.spacing) / parent.columns
                label: qsTr("Recovery")
                minimum: Settings.minIntervalRecoverySec
                maximum: Settings.maxIntervalRecoverySec
                value: setup.recoverySec
                valueText: timeFormatter.elapsed(value * 1000)
                onEdited: setup.recoverySec = value
            }
        }

        BigButton {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            height: Math.max(implicitHeight, Theme.itemSizeExtraLarge)
            text: qsTr("Start")
            onClicked: {
                intervalBridge.tickSeconds = Settings.tickSeconds
                Settings.setIntervalWorkSec(setup.workSec)
                Settings.setIntervalRecoverySec(setup.recoverySec)
                intervalBridge.start(setup.workSec, setup.recoverySec)
            }
        }
    }

    VerticalScrollDecorator { }
}

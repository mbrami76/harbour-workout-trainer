// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// The interval timer (R-MOD-2): work and recovery alternate until the user stops. No lead-in, no
// sound yet, no session log. The page shows the setup until a run starts and the run view while
// it is in progress, paused or not.
Page {
    id: page

    allowedOrientations: Orientation.All
    // A run is ended with the hold button only (R-TMR-11), not by leaving the page.
    backNavigation: !intervalBridge.inProgress
    forwardNavigation: false

    // The display stays on from the start to the stop of a run, also while it is paused
    // (R-TMR-7); the run pauses when the app is no longer active and stays paused (R-TMR-8, R-TMR-9).
    KeepAwake {
        session: intervalBridge.inProgress
        onActiveLost: {
            if (intervalBridge.running)
                intervalBridge.pause()
        }
    }

    IntervalSetup {
        anchors.fill: parent
        visible: !intervalBridge.inProgress
        portrait: page.isPortrait
    }

    IntervalRun {
        anchors.fill: parent
        visible: intervalBridge.inProgress
        portrait: page.isPortrait
    }
}

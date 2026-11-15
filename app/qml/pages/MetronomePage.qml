// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// A run of the metronome of the core (R-MOD-7, R-MOD-8, R-MET-1), silent for the log: no session
// log is written. The page shows the setup until a run starts and the run view while it is in
// progress, paused or not. Three uses: Auto-pace (mode "auto"), Manual-pace (mode "manual") and the
// editor of a rhythm with its try (mode "rhythm", with a saved rhythm or a new one).
Page {
    id: page

    // "auto", "manual" or "rhythm".
    property string mode: "auto"
    // The saved rhythm to open, 0 for none; a new rhythm when `creating`.
    property int savedId: 0
    property bool creating: false
    // Open the rhythm at its try section.
    property bool tryFirst: false

    allowedOrientations: Orientation.All
    // A run is ended with the hold button only (R-TMR-11), not by leaving the page.
    backNavigation: !metronomeBridge.inProgress
    forwardNavigation: false

    // The display stays on from the start to the stop of a run, also while it is paused
    // (R-TMR-7); the run pauses when the app is no longer active and stays paused (R-TMR-8, R-TMR-9).
    KeepAwake {
        session: metronomeBridge.inProgress
        onActiveLost: {
            if (metronomeBridge.running)
                metronomeBridge.pause()
        }
    }

    PaceSetup {
        anchors.fill: parent
        visible: page.mode !== "rhythm" && !metronomeBridge.inProgress
        enabled: visible
        portrait: page.isPortrait
        manual: page.mode === "manual"
    }

    RhythmEditor {
        anchors.fill: parent
        visible: page.mode === "rhythm" && !metronomeBridge.inProgress
        enabled: visible
        portrait: page.isPortrait
        savedId: page.savedId
        creating: page.creating
        focusTry: page.tryFirst
    }

    MetronomeRun {
        anchors.fill: parent
        visible: metronomeBridge.inProgress
        portrait: page.isPortrait
        trying: page.mode === "rhythm"
    }
}

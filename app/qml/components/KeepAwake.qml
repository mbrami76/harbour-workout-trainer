// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Nemo.KeepAlive 1.2

// Display and lifecycle rules of a running session (R-TMR-7 to R-TMR-9). While `session` is true
// and the app is active, the display is kept on. The app is active when it is in the foreground
// and unlocked; page visibility plays no part, because with the lock screen up a page is still
// visible. `activeLost()` is emitted when the app stops being active, which is when a session must
// pause. It stays paused when the app becomes active again: that is up to the user.
Item {
    id: keepAwake

    // True from the start of a session to its end or stop, paused or not.
    property bool session: false
    // Whether the display is being kept on right now.
    readonly property bool preventing: blanking.preventBlanking

    signal activeLost()

    DisplayBlanking {
        id: blanking
        preventBlanking: keepAwake.session && Qt.application.active
    }

    Connections {
        target: Qt.application
        onActiveChanged: {
            if (!Qt.application.active)
                keepAwake.activeLost()
        }
    }
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.6
import Sailfish.Silica 1.0
import "../components"

// The quick modes that need no preparation (R-MOD-6): Timer, Interval timer, Time attack, Auto-pace
// and Manual-pace as large icons in the normal colour, with the rules of the first page for the size,
// the labels and the position at the bottom of the screen (R-MOD-1, R-UI-13).
Page {
    id: page

    allowedOrientations: Orientation.All

    readonly property var modes: [
        { title: qsTr("Timer"), icon: "timer", page: "TimerPage.qml", props: {} },
        { title: qsTr("Interval timer"), icon: "interval", page: "IntervalPage.qml", props: {} },
        { title: qsTr("Time attack"), icon: "time-attack", page: "TimeAttackListPage.qml", props: {} },
        { title: qsTr("Auto-pace"), icon: "auto-pace", page: "MetronomePage.qml", props: { mode: "auto" } },
        { title: qsTr("Manual-pace"), icon: "manual-pace", page: "MetronomePage.qml", props: { mode: "manual" } }
    ]
    // Auto-pace and Manual-pace above, the three timers in the lowest row.
    readonly property var rowList: [[3, 4], [0, 1, 2]]

    PageHeader {
        id: header
        title: qsTr("Quick start")
    }

    ModeGrid {
        anchors.fill: parent
        modes: page.modes
        rowList: page.rowList
        columns: 3
        headerHeight: header.height
        onActivated: pageStack.push(Qt.resolvedUrl(mode.page), mode.props)
    }
}

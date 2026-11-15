// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// The cover of the app (R-UI-17): the title, a faint silhouette of the icon, and in front either
// the paused session or timer or the time since the last completed workout. The app pauses when it
// goes to the background, so a paused run is what the cover shows. The cover reads what the pages
// and the bridges already hold; it has no actions and runs no timer of its own except the minute
// refresh of the elapsed time, which runs only while the cover is visible.
CoverBackground {
    id: cover

    readonly property bool visibleNow: status === Cover.Active || status === Cover.Activating

    RunScheme { id: colours }

    // The one state the cover draws. `kind` is the paused run to show: "", "workout", "interval",
    // "countdown", "stopwatch", "timeattack" or "metronome".
    QtObject {
        id: state

        // The paused run that paused last, kept by its kind.
        property string latest: ""
        readonly property string kind: {
            if (latest === "workout" && sessionBridge.isPaused) return latest
            if (latest === "interval" && intervalBridge.state === 2) return latest
            if (latest === "countdown" && countdownBridge.state === 2) return latest
            if (latest === "stopwatch" && stopwatchBridge.state === 2) return latest
            if (latest === "timeattack" && timeAttackBridge.state === 2) return latest
            if (latest === "metronome" && metronomeBridge.state === 4) return latest
            // The first paused run found, when none was seen pausing (the app started paused).
            if (sessionBridge.isPaused) return "workout"
            if (intervalBridge.state === 2) return "interval"
            if (countdownBridge.state === 2) return "countdown"
            if (stopwatchBridge.state === 2) return "stopwatch"
            if (timeAttackBridge.state === 2) return "timeattack"
            if (metronomeBridge.state === 4) return "metronome"
            return ""
        }
        readonly property bool paused: kind !== ""
        readonly property bool workout: kind === "workout"

        readonly property string kindText: {
            switch (kind) {
            case "workout": return qsTr("Workout")
            case "interval": return qsTr("Interval timer")
            case "countdown": return qsTr("Timer")
            case "stopwatch": return qsTr("Stopwatch")
            case "timeattack": return qsTr("Time attack")
            case "metronome": return qsTr("Metronome")
            }
            return ""
        }
        readonly property string timeText: {
            switch (kind) {
            case "workout": return timeFormatter.elapsed(sessionBridge.workoutElapsedMs)
            case "interval": return timeFormatter.countdown(intervalBridge.remainingMs)
            case "countdown": return timeFormatter.countdown(countdownBridge.remainingMs)
            case "stopwatch": return timeFormatter.elapsed(stopwatchBridge.elapsedMs)
            case "timeattack": return timeFormatter.elapsed(timeAttackBridge.elapsedMs)
            case "metronome": return timeFormatter.countdown(metronomeBridge.remainingMs)
            }
            return ""
        }
        readonly property string workoutName: workout ? sessionBridge.workoutName : ""
        readonly property string blockText: workout && sessionBridge.blockCount > 1
            ? qsTr("Block %1 of %2").arg(sessionBridge.blockNumber).arg(sessionBridge.blockCount) : ""
        readonly property string stepText: workout
            ? qsTr("Step %1 of %2").arg(sessionBridge.stepNumber).arg(sessionBridge.stepCount) : ""
        readonly property string exerciseName: workout && sessionBridge.current.exerciseName
            ? sessionBridge.current.exerciseName : ""
        readonly property real progress: {
            if (!workout)
                return 0
            var done = sessionBridge.workoutElapsedMs
            var left = sessionBridge.workoutRemainingMs
            return done + left > 0 ? done / (done + left) : 0
        }
    }

    Connections { target: sessionBridge; onStateChanged: if (sessionBridge.isPaused) state.latest = "workout" }
    Connections { target: intervalBridge; onStateChanged: if (intervalBridge.state === 2) state.latest = "interval" }
    Connections { target: countdownBridge; onStateChanged: if (countdownBridge.state === 2) state.latest = "countdown" }
    Connections { target: stopwatchBridge; onStateChanged: if (stopwatchBridge.state === 2) state.latest = "stopwatch" }
    Connections { target: timeAttackBridge; onStateChanged: if (timeAttackBridge.state === 2) state.latest = "timeattack" }
    Connections { target: metronomeBridge; onStateChanged: if (metronomeBridge.state === 4) state.latest = "metronome" }

    // Milliseconds since the end of the last completed workout, -1 for none; read again when the
    // cover appears and every minute while it is visible.
    property real sinceMs: -1
    function refresh() { sinceMs = dashboard.sinceLastCompletedMs() }
    onVisibleNowChanged: if (visibleNow) refresh()
    Component.onCompleted: refresh()

    Timer {
        interval: 60000
        repeat: true
        running: cover.visibleNow && !state.paused
        onTriggered: cover.refresh()
    }

    readonly property string agoText: {
        if (sinceMs < 0)
            return qsTr("No workout completed yet")
        var a = timeFormatter.ago(sinceMs)
        switch (a.kind) {
        case "minutes": return qsTr("%1 min ago").arg(a.major)
        case "hoursMinutes": return qsTr("%1 h %2 min ago").arg(a.major).arg(a.minor)
        case "daysHours": return qsTr("%1 d %2 h ago").arg(a.major).arg(a.minor)
        }
        return qsTr("just now")
    }

    // The faint silhouette of the icon, in the text colour at about 12 % opacity.
    Image {
        id: silhouette
        visible: false
        source: Qt.resolvedUrl("../../app-icon/app-icon-mono.png")
        sourceSize.width: 512
        sourceSize.height: 512
    }
    ShaderEffect {
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height) * 0.95
        height: width
        opacity: 0.12
        property variant source: silhouette
        property color tint: colours.text
        fragmentShader: "
            varying highp vec2 qt_TexCoord0;
            uniform sampler2D source;
            uniform lowp vec4 tint;
            uniform lowp float qt_Opacity;
            void main() {
                lowp float a = texture2D(source, qt_TexCoord0).a;
                gl_FragColor = vec4(tint.rgb * a, a) * qt_Opacity;
            }"
    }

    Label {
        id: title
        anchors {
            top: parent.top
            topMargin: Theme.paddingMedium
            left: parent.left
            right: parent.right
        }
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: Theme.fontSizeSmall
        font.bold: true
        color: colours.text
        elide: Text.ElideRight
        text: qsTr("Workout Trainer")
    }

    // A paused run.
    Column {
        visible: state.paused
        anchors {
            top: title.bottom
            topMargin: Theme.paddingMedium
            left: parent.left
            right: parent.right
            margins: Theme.paddingMedium
        }
        spacing: Theme.paddingSmall

        Glyph {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Theme.iconSizeMedium
            height: width
            kind: "pause"
            color: colours.symbol
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeExtraSmall
            color: colours.secondary
            elide: Text.ElideRight
            text: state.kindText
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeLarge
            font.bold: true
            color: colours.text
            fontSizeMode: Text.Fit
            minimumPixelSize: Theme.fontSizeSmall
            text: state.timeText
        }
        Label {
            width: parent.width
            visible: state.workout && text !== ""
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeExtraSmall
            color: colours.text
            elide: Text.ElideRight
            text: state.workoutName
        }
        Label {
            width: parent.width
            visible: text !== ""
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeExtraSmall
            color: colours.secondary
            elide: Text.ElideRight
            text: state.blockText
        }
        Label {
            width: parent.width
            visible: text !== ""
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeExtraSmall
            color: colours.secondary
            elide: Text.ElideRight
            text: state.stepText
        }
        Label {
            width: parent.width
            visible: text !== ""
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeExtraSmall
            color: colours.text
            elide: Text.ElideRight
            text: state.exerciseName
        }
        Rectangle {
            visible: state.workout
            width: parent.width
            height: Math.max(2, Theme.paddingSmall * 0.6)
            radius: height / 2
            color: colours.barTrack

            Rectangle {
                width: parent.width * state.progress
                height: parent.height
                radius: parent.radius
                color: colours.barFill
            }
        }
    }

    // Nothing runs: the time since the last completed workout.
    Label {
        visible: !state.paused
        anchors {
            centerIn: parent
        }
        width: parent.width - 2 * Theme.paddingMedium
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        font.pixelSize: Theme.fontSizeMedium
        color: colours.text
        text: cover.agoText
    }
}

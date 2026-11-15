// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import QtFeedback 5.0
import "../components"

// Runs one saved workout (R-TMR-1 to R-TMR-11). The session starts when the page opens. It is ended
// with the stop hold only (R-TMR-11), not by leaving the page (back navigation is refused while it
// is on). The display stays on from the start to the end or stop, also while it is paused
// (R-TMR-7); the session pauses when the app is no longer active and stays paused until the user
// lifts it (R-TMR-8, R-TMR-9). A finished session shows its total time; a stopped one returns to the
// workout.
Page {
    id: page

    property int workoutId
    // The start was refused (the workout cannot run any more).
    property bool failed: false

    allowedOrientations: Orientation.All
    backNavigation: !sessionBridge.inProgress
    forwardNavigation: false

    // The file of the exercise that the session offers, over the session (R-TMR-12). The exercise is
    // the one of the moment of the tap and stays when the session moves on.
    function openExerciseFile() {
        pageStack.push(Qt.resolvedUrl("ExerciseOverlayPage.qml"), { exerciseKey: sessionBridge.exerciseFileKey })
    }

    Component.onCompleted: {
        sessionBridge.leadInSec = Settings.leadInSec
        sessionBridge.tickSeconds = Settings.tickSeconds
        failed = !sessionBridge.start(workoutId)
    }

    // A finished session that is left by any route frees the bridge for the next one.
    Component.onDestruction: sessionBridge.close()

    KeepAwake {
        session: sessionBridge.inProgress
        onActiveLost: {
            if (sessionBridge.pausable)
                sessionBridge.pause()
        }
    }

    // Asked for `supported` only; the lengths of the effects are the platform's (R-ALR-4). The
    // lead-in ticks use the weakest press, the start of a work, warm-up or cool-down step the normal
    // press and the end of the workout the strongest; the beats of a paced step follow R-MET-3.
    ThemeEffect { id: supportProbe; effect: ThemeEffect.PressWeak }
    ThemeEffect { id: weak; effect: ThemeEffect.PressWeak }
    ThemeEffect { id: normal; effect: ThemeEffect.Press }
    ThemeEffect { id: strong; effect: ThemeEffect.PressStrong }
    readonly property bool vibrate: Settings.vibration && supportProbe.supported
    // A new round starts with the round sound instead of the sound of its first step.
    property bool roundSounded: false

    Connections {
        target: sessionBridge
        onBeat: {
            run.fire(kind, atMs)
            Sounds.play(Sounds.clickOf(kind), atMs)
            if (page.vibrate)
                (kind === "first" ? normal : weak).play()
        }
        onLeadInTick: {
            Sounds.play("lead-in-tick")
            if (page.vibrate)
                weak.play()
        }
        onLastSecondsTick: Sounds.play("tick")
        onWaitCue: Sounds.play("wait-for-tap")
        onRoundChanged: {
            page.roundSounded = Sounds.allowed("round-change") && Qt.application.active
            Sounds.play("round-change")
        }
        onStepStarted: {
            if (page.roundSounded)
                page.roundSounded = false
            else
                Sounds.play(type === "rest" ? "rest-start" : "work-start")
            if (page.vibrate && type !== "rest")
                normal.play()
        }
        onFinished: {
            Sounds.play("workout-complete")
            if (page.vibrate)
                strong.play()
        }
        onStopped: {
            run.clearPulse()
            // The exercise file may be open over this page: both are left.
            pageStack.pop(pageStack.previousPage(page))
        }
        onStateChanged: {
            if (!sessionBridge.running)
                run.clearPulse()
        }
    }

    SessionRun {
        id: run
        anchors.fill: parent
        visible: !sessionBridge.isFinished && !page.failed
        portrait: page.isPortrait
        onShowExerciseFile: page.openExerciseFile()
    }

    // The end of the workout: the total time and the way back. The total time is on the line of its
    // caption and on the same baseline, and the way back is below, clear of it.
    Item {
        id: finish
        anchors.fill: parent
        visible: sessionBridge.isFinished

        readonly property real gap: Metrics.mm(3)
        readonly property real backTop: height * 0.95 - Metrics.buttonSize

        Column {
            id: finishText
            x: parent.width * 0.025
            y: parent.height * 0.05
            width: parent.width * 0.95
            spacing: finish.gap

            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                font.pixelSize: Metrics.labelSize * 2
                font.bold: true
                color: Theme.highlightColor
                text: qsTr("Workout complete")
            }
            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                font.pixelSize: Metrics.labelSize * 1.5
                color: Theme.secondaryHighlightColor
                text: sessionBridge.workoutName
            }
        }

        Item {
            id: totalRow
            x: parent.width * 0.025
            y: finishText.y + finishText.height + finish.gap
            width: parent.width * 0.95
            height: Math.max(1, Math.min(parent.height * 0.2, finish.backTop - y - finish.gap))

            Label {
                id: totalCaption
                y: totalDigits.y + totalDigits.baselineY - baselineOffset
                font.pixelSize: Metrics.labelSize * 1.4
                color: Theme.secondaryColor
                text: qsTr("Total time")
            }
            TimeDigits {
                id: totalDigits
                x: totalCaption.width + finish.gap
                width: parent.width - x
                height: implicitHeight
                maxHeight: totalRow.height
                showModeLabel: false
                toggleOnTap: false
                parts: timeFormatter.elapsedParts(sessionBridge.totalSec * 1000)
            }
        }

        BigButton {
            x: parent.width * 0.025
            y: finish.backTop
            width: parent.width * 0.95
            minimumSize: Metrics.buttonSize
            text: qsTr("Back to the workout")
            onActivated: {
                sessionBridge.close()
                pageStack.pop()
            }
        }
    }

    // The workout could not be started.
    Label {
        anchors.centerIn: parent
        width: parent.width * 0.9
        visible: page.failed
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: Theme.highlightColor
        text: qsTr("This workout cannot be run.")
    }
}

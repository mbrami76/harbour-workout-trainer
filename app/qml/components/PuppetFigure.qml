// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.6
import Sailfish.Silica 1.0
import "."
import "puppetfigure.js" as Figure

// The cue for muscle groups (R-MUS-1): a neutral figure drawn from simple shapes, no pictures and
// no text, with the regions of `groups` highlighted. The figure turns about its vertical axis so
// that every region can be seen from the front, the side and the back, and the highlight follows
// the visible face.
//
// Choosing groups turns the figure in half a second to the face that shows them, holds it for
// three seconds and then the continuous turn goes on. With `turning` off the figure does not turn:
// it stands on the back if all chosen groups are back groups, else on the front.
//
// Cost: the continuous turn is a 33 ms timer that runs only while the item is visible and the app
// is active, and the angle follows the elapsed time, so a late tick does not slow the turn. A
// hidden, covered or paused figure runs no timer and draws nothing. The canvas renders into a
// framebuffer object on a hardware GL stack and into an image on software GL (screenMetrics).
Item {
    id: figure

    // Keys of the muscle groups to highlight; empty for none.
    property var groups: []
    // The colour of the highlight: the theme by default, or the colour of the exercise.
    property color highlightColor: Theme.highlightColor
    property bool turning: true
    // Turn angle in radians: 0 shows the front, pi the back.
    property real angle: 0
    // Said to the accessibility tools instead of a picture.
    property string description

    readonly property real unitAspect: 128 / 118
    implicitWidth: Metrics.mm(30)
    implicitHeight: implicitWidth / unitAspect

    Accessible.role: Accessible.Graphic
    Accessible.name: description

    // The seconds in a full turn at the continuous speed.
    readonly property real turnSeconds: 7
    readonly property bool animating: turning && visible && Qt.application.active

    function selection() {
        var set = {}
        for (var i = 0; i < groups.length; ++i)
            set[groups[i]] = true
        return set
    }

    // The face that shows the chosen groups: back only if every group is a back group.
    function faceAngle() {
        if (groups.length === 0)
            return 0
        for (var i = 0; i < groups.length; ++i) {
            if (Figure.faceFor(groups[i]) !== "back")
                return 0
        }
        return Math.PI
    }

    // The angle that shows the face, nearest to the current angle, so that the turn is short.
    function nearest(face) {
        var full = 2 * Math.PI
        return face + Math.round((angle - face) / full) * full
    }

    function show() {
        faceAnimation.stop()
        holdTimer.stop()
        if (!turning) {
            angle = faceAngle()
            return
        }
        if (groups.length === 0)
            return
        faceAnimation.to = nearest(faceAngle())
        faceAnimation.restart()
        holdTimer.restart()
    }

    onGroupsChanged: show()
    onTurningChanged: show()
    Component.onCompleted: show()

    NumberAnimation {
        id: faceAnimation
        target: figure
        property: "angle"
        duration: 500
        easing.type: Easing.OutQuad
    }

    // The pause after a turn to a face; a single shot costs nothing while it waits.
    Timer {
        id: holdTimer
        interval: 3000
    }

    Timer {
        id: turnTimer
        property real last: 0
        interval: 33
        repeat: true
        running: figure.animating && !holdTimer.running && !faceAnimation.running
        onRunningChanged: last = Date.now()
        onTriggered: {
            var now = Date.now()
            figure.angle = (figure.angle + (now - last) / 1000 * 2 * Math.PI / figure.turnSeconds) % (2 * Math.PI)
            last = now
        }
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        renderTarget: screenMetrics.softwareRendering ? Canvas.Image : Canvas.FramebufferObject
        renderStrategy: Canvas.Immediate

        // Colours go to the canvas as CSS strings: colour values inside an object make it fill
        // limbs with the outline colour.
        function css(c, alpha) {
            return "rgba(" + Math.round(c.r * 255) + "," + Math.round(c.g * 255) + "," + Math.round(c.b * 255) + "," + alpha + ")"
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            if (width <= 0 || height <= 0)
                return
            var accent = figure.highlightColor
            var ink = Theme.primaryColor
            // The base is opaque: a translucent fill and outline overlap along every edge and
            // show as a bevel. The ink is pulled a quarter of the way to mid grey, which keeps
            // 3:1 against both ambiences.
            var solid = Qt.rgba(ink.r * 0.75 + 0.125, ink.g * 0.75 + 0.125, ink.b * 0.75 + 0.125, 1)
            Figure.drawFigure(ctx, width, height, figure.angle, figure.selection(), {
                accent: css(accent, 1),
                edge: css(Qt.darker(accent, 2.0), 1),
                base: css(solid, 1),
                line: css(ink, 0.6),
                erase: true
            })
        }
    }

    Connections {
        target: figure
        onAngleChanged: canvas.requestPaint()
        onGroupsChanged: canvas.requestPaint()
        onHighlightColorChanged: canvas.requestPaint()
    }
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()
    Connections {
        target: Theme
        onPrimaryColorChanged: canvas.requestPaint()
    }
}

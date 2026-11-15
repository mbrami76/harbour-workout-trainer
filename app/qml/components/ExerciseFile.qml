// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "."

// The file of one exercise (R-EXR-1, R-EXR-8, R-EXR-10, R-MUS-1, R-MUS-2, R-TMR-12): its icon and
// level colour, the figure with its muscle groups highlighted, the rhythm if any, the instructions
// and the tips. It is the content of the exercise page of the library and of the overlay of a session;
// the page around it supplies the header, the scrolling and the width.
Column {
    id: file

    property string exerciseKey
    // The exercise as a map; empty when it does not exist (any more).
    property var exercise: ({})
    // The metronome of the exercise as the list of metronomes shows it; empty for none.
    property var metronome: ({})
    readonly property bool found: exercise.key !== undefined

    spacing: Theme.paddingLarge

    // Reads the exercise and its metronome again.
    function reload() {
        metronomeLibrary.refresh()
        exercise = exerciseLibrary.get(exerciseKey)
        metronome = ({})
        if (exercise.metronomeId > 0) {
            var all = metronomeLibrary.metronomes
            for (var i = 0; i < all.length; ++i) {
                if (all[i].id === exercise.metronomeId)
                    metronome = all[i]
            }
        }
    }

    onExerciseKeyChanged: reload()
    Component.onCompleted: reload()

    Label {
        x: Theme.horizontalPageMargin
        width: file.width - 2 * Theme.horizontalPageMargin
        visible: !file.found
        wrapMode: Text.Wrap
        color: Theme.secondaryHighlightColor
        text: qsTr("This exercise does not exist any more.")
    }

    Row {
        x: Theme.horizontalPageMargin
        visible: file.found
        spacing: Theme.paddingLarge

        IconBadge {
            anchors.verticalCenter: parent.verticalCenter
            iconKey: file.found ? file.exercise.icon : ""
            colourKey: file.found ? file.exercise.colour : ""
        }

        Label {
            anchors.verticalCenter: parent.verticalCenter
            width: file.width - 2 * Theme.horizontalPageMargin - Metrics.mm(11) - Theme.paddingLarge
            wrapMode: Text.Wrap
            color: Theme.secondaryHighlightColor
            text: qsTr("Colour: %1").arg(Swatches.label(file.found ? file.exercise.colour : ""))
        }
    }

    PuppetFigure {
        anchors.horizontalCenter: parent.horizontalCenter
        visible: file.found
        width: Math.min(file.width * 0.6, Screen.height * 0.3 * 128 / 118)
        height: width * 118 / 128
        groups: file.found ? file.exercise.muscles : []
        highlightColor: file.found && Swatches.known(file.exercise.colour)
                        ? Swatches.color(file.exercise.colour) : Theme.highlightColor
        description: file.found ? qsTr("Muscle groups: %1").arg(MuscleText.names(file.exercise.muscles)) : ""
    }

    Label {
        x: Theme.horizontalPageMargin
        width: file.width - 2 * Theme.horizontalPageMargin
        visible: file.found
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
        color: Theme.highlightColor
        text: file.found ? MuscleText.names(file.exercise.muscles) : ""
    }

    SectionHeader {
        visible: file.found
        text: qsTr("Rhythm")
    }

    Label {
        x: Theme.horizontalPageMargin
        width: file.width - 2 * Theme.horizontalPageMargin
        visible: file.found && file.metronome.id === undefined
        wrapMode: Text.Wrap
        color: Theme.secondaryColor
        text: qsTr("No rhythm")
    }

    Column {
        x: Theme.horizontalPageMargin
        width: file.width - 2 * Theme.horizontalPageMargin
        visible: file.found && file.metronome.id !== undefined
        spacing: Theme.paddingSmall

        Label {
            width: parent.width
            text: file.metronome.name !== undefined ? file.metronome.name : ""
            color: Theme.primaryColor
            truncationMode: TruncationMode.Fade
        }
        PaceStrip {
            width: parent.width
            units: file.metronome.phaseUnits !== undefined ? file.metronome.phaseUnits : []
        }
        Label {
            width: parent.width
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            text: file.metronome.id !== undefined ? MetronomeText.summary(file.metronome) : ""
        }
    }

    SectionHeader {
        visible: file.found && file.exercise.instructions !== ""
        text: qsTr("Instructions")
    }
    Label {
        x: Theme.horizontalPageMargin
        width: file.width - 2 * Theme.horizontalPageMargin
        visible: file.found && file.exercise.instructions !== ""
        wrapMode: Text.Wrap
        textFormat: Text.PlainText
        color: Theme.primaryColor
        text: file.found ? file.exercise.instructions : ""
    }

    SectionHeader {
        visible: file.found && file.exercise.tips !== ""
        text: qsTr("Form tips")
    }
    Label {
        x: Theme.horizontalPageMargin
        width: file.width - 2 * Theme.horizontalPageMargin
        visible: file.found && file.exercise.tips !== ""
        wrapMode: Text.Wrap
        textFormat: Text.PlainText
        color: Theme.primaryColor
        text: file.found ? file.exercise.tips : ""
    }
}

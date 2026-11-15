// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// The setup of the two quick paces (R-MOD-7, R-MOD-8): Auto-pace (the repetitions in a total
// time) and Manual-pace (the time of one repetition), both with sets, a rest between sets and the
// lead-in of the settings, one beat per repetition. Nothing is saved by name; the values of the last
// run are offered again and are stored when a run starts. A value that breaks a rule is explained
// and Start stays off; the rules are the ones of the core (the bridge's problem()), never copied here.
SilicaFlickable {
    id: setup

    property bool portrait: true
    // Manual-pace (the time of one repetition) when true, Auto-pace (a total time) when false.
    property bool manual: false

    property int reps: Settings.metronomeReps
    property int totalSec: Settings.metronomeTotalSec
    property int repHalfSec: Settings.metronomeRepHalfSec
    property int sets: Settings.metronomeSets
    property int restSec: Settings.metronomeRestSec

    readonly property var config: ({
        form: manual ? "time-per-rep" : "reps-total", reps: reps, totalSec: totalSec, repHalfSec: repHalfSec,
        phases: [], phaseNames: [], sets: sets, restSec: restSec, leadInSec: Settings.leadInSec,
        tickSeconds: Settings.tickSeconds, name: ""
    })
    readonly property string problem: metronomeBridge.problem(config)

    function halfSeconds(units) {
        return qsTr("%1 s").arg((units / 2).toFixed(1))
    }

    function problemText(code) {
        switch (code) {
        case "reps": return qsTr("The repetitions must be from 1 to 999.")
        case "rep-short": return qsTr("One repetition would last less than 0.5 s. Use fewer repetitions or more time.")
        case "rep-long": return qsTr("One repetition would last more than 60 s. Use more repetitions or less time.")
        case "sets": return qsTr("The sets must be from 1 to 99.")
        case "rest": return qsTr("The rest must be from 0 to 600 s.")
        case "lead-in": return qsTr("The get-ready time must be from 0 to 10 s.")
        }
        return ""
    }

    contentHeight: column.height + Theme.paddingLarge

    Column {
        id: column
        width: parent.width
        spacing: Theme.paddingLarge

        PageHeader {
            title: setup.manual ? qsTr("Manual-pace") : qsTr("Auto-pace")
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            wrapMode: Text.Wrap
            color: Theme.secondaryHighlightColor
            text: setup.manual ? qsTr("One beat for every repetition, at the time you set for one repetition.")
                               : qsTr("One beat for every repetition, spread evenly over the total time you set.")
        }

        Grid {
            width: parent.width
            columns: setup.portrait ? 1 : 2
            spacing: Theme.paddingLarge

            NumberStepper {
                width: (parent.width - (parent.columns - 1) * parent.spacing) / parent.columns
                visible: !setup.manual
                label: qsTr("Total time of a set")
                minimum: 1
                maximum: 59940
                steps: [-60, -1, 1, 60]
                value: setup.totalSec
                valueText: timeFormatter.elapsed(value * 1000)
                onEdited: setup.totalSec = value
            }

            NumberStepper {
                width: (parent.width - (parent.columns - 1) * parent.spacing) / parent.columns
                visible: setup.manual
                label: qsTr("Time per repetition")
                minimum: 0
                maximum: 130
                steps: [-10, -1, 1, 10]
                value: setup.repHalfSec
                valueText: setup.halfSeconds(value)
                onEdited: setup.repHalfSec = value
            }

            NumberStepper {
                width: (parent.width - (parent.columns - 1) * parent.spacing) / parent.columns
                label: qsTr("Repetitions")
                minimum: 1
                maximum: 999
                steps: [-10, -1, 1, 10]
                value: setup.reps
                valueText: String(value)
                onEdited: setup.reps = value
            }

            NumberStepper {
                width: (parent.width - (parent.columns - 1) * parent.spacing) / parent.columns
                label: qsTr("Sets")
                minimum: 1
                maximum: 99
                steps: [-10, -1, 1, 10]
                value: setup.sets
                valueText: String(value)
                onEdited: setup.sets = value
            }

            NumberStepper {
                width: (parent.width - (parent.columns - 1) * parent.spacing) / parent.columns
                label: qsTr("Rest between sets")
                minimum: 0
                maximum: 600
                steps: [-10, -1, 1, 10]
                value: setup.restSec
                valueText: timeFormatter.elapsed(value * 1000)
                onEdited: setup.restSec = value
            }
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            wrapMode: Text.Wrap
            color: Theme.secondaryColor
            text: qsTr("Get ready: %n second(s). Change it in the settings.", "", Settings.leadInSec)
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            visible: setup.problem !== ""
            wrapMode: Text.Wrap
            color: Theme.errorColor
            text: setup.problemText(setup.problem)
        }

        BigButton {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            height: Math.max(implicitHeight, Theme.itemSizeExtraLarge)
            enabled: setup.problem === ""
            opacity: enabled ? 1 : 0.4
            text: qsTr("Start")
            onActivated: {
                Settings.setMetronomeReps(setup.reps)
                Settings.setMetronomeTotalSec(setup.totalSec)
                Settings.setMetronomeRepHalfSec(setup.repHalfSec)
                Settings.setMetronomeSets(setup.sets)
                Settings.setMetronomeRestSec(setup.restSec)
                metronomeBridge.start(setup.config)
            }
        }
    }

    VerticalScrollDecorator { }
}

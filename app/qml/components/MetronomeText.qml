// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

pragma Singleton
import QtQuick 2.0

// The texts about saved metronomes, shown to the user as rhythms (R-MET-7, R-MET-8) shared by the list, the editor and the
// dialogs. The codes come from the core; the rules are never repeated here.
QtObject {
    // One line about a saved rhythm: the number of its phases and the length of one repetition.
    function summary(m) {
        return qsTr("%n phase(s)", "", m.phaseCount) + ", " + qsTr("%1 s per repetition").arg((m.repetitionMs / 1000).toFixed(1))
    }

    // The reason for a code that is about the name; empty for any other.
    function identityProblem(code) {
        switch (code) {
        case "name-empty": return qsTr("Give the rhythm a name.")
        case "name-too-long": return qsTr("The name is too long: use at most 80 characters.")
        case "identity": return qsTr("Another rhythm already has this name. Change it.")
        case "failed": return qsTr("The rhythms could not be read.")
        }
        return ""
    }

    // The names of a list of exercises, one per line.
    function names(list) {
        var lines = []
        for (var i = 0; i < list.length; ++i)
            lines.push(list[i].name)
        return lines.join("\n")
    }
}

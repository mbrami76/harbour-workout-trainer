// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0

// The confirmation before a challenge and all its results are deleted (R-MOD-4).
Dialog {
    property string challengeName

    allowedOrientations: Orientation.All

    Column {
        width: parent.width

        DialogHeader {
            acceptText: qsTr("Delete")
            cancelText: qsTr("Cancel")
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            wrapMode: Text.Wrap
            color: Theme.highlightColor
            text: qsTr("Delete the challenge \"%1\" and all its results? This cannot be undone.").arg(challengeName)
        }
    }
}

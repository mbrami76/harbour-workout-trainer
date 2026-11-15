// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0

// The confirmation before a workout is deleted (R-WRK-1). Nothing else uses a workout, so the
// question names the workout and says that it cannot be undone.
Dialog {
    id: dialog

    property string workoutName

    allowedOrientations: Orientation.All

    Column {
        width: parent.width
        spacing: Theme.paddingLarge

        DialogHeader {
            acceptText: qsTr("Delete")
            cancelText: qsTr("Cancel")
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            wrapMode: Text.Wrap
            textFormat: Text.PlainText
            color: Theme.highlightColor
            text: qsTr("Delete the workout \"%1\"? This cannot be undone.").arg(dialog.workoutName)
        }
    }
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0

// Stands in for a mode until its task builds the real page.
Page {
    property string title

    allowedOrientations: Orientation.All

    PageHeader {
        id: header
        title: parent.title
    }

    Label {
        anchors {
            left: parent.left
            right: parent.right
            margins: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: Theme.highlightColor
        text: qsTr("Not available yet")
    }
}

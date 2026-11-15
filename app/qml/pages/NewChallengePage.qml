// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// A name for a new challenge. The store checks it (not empty, at most 60 characters); a refusal is
// shown here and the page stays open.
Page {
    id: page

    allowedOrientations: Orientation.All

    property string error

    function errorText(code) {
        switch (code) {
        case "name-empty": return qsTr("The name must not be empty.")
        case "name-too-long": return qsTr("The name is too long: use at most 60 characters.")
        }
        return qsTr("The challenge could not be saved.")
    }

    function create() {
        var code = timeAttackBridge.createChallenge(nameField.text)
        if (code === "")
            pageStack.pop()
        else
            error = errorText(code)
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader { title: qsTr("New challenge") }

            TextField {
                id: nameField
                width: parent.width
                label: qsTr("Name")
                placeholderText: qsTr("Name")
                focus: true
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: page.create()
                onTextChanged: page.error = ""
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                visible: page.error !== ""
                text: page.error
            }

            BigButton {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Create")
                onClicked: page.create()
            }
        }
    }
}

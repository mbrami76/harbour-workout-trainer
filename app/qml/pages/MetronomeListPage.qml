// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// The Metronome tile opens here (R-MET-1, R-MET-7): the library of rhythms. "New rhythm" comes
// first, then the saved rhythms. A tap opens a rhythm in its editor, which can also try it; a press
// and hold opens a menu to try, edit, duplicate or delete it.
Page {
    id: page

    allowedOrientations: Orientation.All

    // Shown under the list after a duplicate or a delete.
    property string notice

    function open(id, creating, tryFirst) {
        notice = ""
        pageStack.push(Qt.resolvedUrl("MetronomePage.qml"),
                       { mode: "rhythm", savedId: id, creating: creating, tryFirst: tryFirst === true })
    }

    function confirmDelete(m) {
        var users = metronomeLibrary.exercisesUsing(m.id)
        var dialog = pageStack.push(Qt.resolvedUrl("DeleteMetronomeDialog.qml"), { metronomeName: m.name, users: users })
        dialog.accepted.connect(function () {
            var r = metronomeLibrary.remove(m.id)
            if (r.status !== "ok")
                page.notice = qsTr("The rhythm could not be deleted.")
            else
                page.notice = r.cleared.length > 0
                    ? qsTr("Deleted \"%1\". %n exercise(s) lost their rhythm.", "", r.cleared.length).arg(m.name)
                    : qsTr("Deleted \"%1\".").arg(m.name)
        })
    }

    function duplicate(m) {
        var r = metronomeLibrary.duplicate(m.id)
        page.notice = r.status === "ok" ? qsTr("Created \"%1\".").arg(r.name) : qsTr("The rhythm could not be copied.")
    }

    Component.onCompleted: metronomeLibrary.refresh()
    onStatusChanged: {
        if (status === PageStatus.Active)
            metronomeLibrary.refresh()
    }

    SilicaListView {
        id: list
        anchors.fill: parent
        model: metronomeLibrary.metronomes

        header: Column {
            width: list.width

            PageHeader { title: qsTr("Metronome") }

            BackgroundItem {
                width: parent.width
                height: Math.max(Theme.itemSizeLarge, Metrics.mm(9))
                onClicked: page.open(0, true)
                Accessible.role: Accessible.Button
                Accessible.name: qsTr("New rhythm")

                Column {
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    Label {
                        width: parent.width
                        text: qsTr("New rhythm")
                        font.pixelSize: Theme.fontSizeLarge
                        color: highlighted ? Theme.highlightColor : Theme.primaryColor
                    }
                    Label {
                        width: parent.width
                        text: qsTr("Make a rhythm of phases with a name")
                        font.pixelSize: Theme.fontSizeSmall
                        color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                        truncationMode: TruncationMode.Fade
                    }
                }
            }

            SectionHeader { text: qsTr("Saved rhythms") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: metronomeLibrary.metronomes.length === 0
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
                text: qsTr("No saved rhythms yet. Use New rhythm to make one.")
            }
        }

        footer: Label {
            x: Theme.horizontalPageMargin
            width: list.width - 2 * Theme.horizontalPageMargin
            height: visible ? implicitHeight + 2 * Theme.paddingLarge : 0
            visible: page.notice !== ""
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            color: Theme.highlightColor
            text: page.notice
        }

        delegate: ListItem {
            id: item
            width: ListView.view.width
            contentHeight: Math.max(Theme.itemSizeExtraLarge, row.height + 2 * Theme.paddingMedium)
            onClicked: page.open(modelData.id, false)

            Accessible.role: Accessible.Button
            Accessible.name: modelData.name

            Row {
                id: row
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                spacing: Theme.paddingLarge

                Column {
                    width: parent.width
                    anchors.verticalCenter: parent.verticalCenter

                    Label {
                        width: parent.width
                        text: modelData.name
                        font.pixelSize: Theme.fontSizeLarge
                        color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                        truncationMode: TruncationMode.Fade
                    }
                    // The strip of one period on a line of its own, the summary under it,
                    // wrapping when it is long, so that nothing overlaps at any font size.
                    Item {
                        width: parent.width
                        height: Metrics.mm(5) + Theme.paddingSmall

                        PaceStrip {
                            width: parent.width
                            anchors.verticalCenter: parent.verticalCenter
                            units: modelData.phaseUnits
                        }
                    }
                    Label {
                        width: parent.width
                        text: MetronomeText.summary(modelData)
                        wrapMode: Text.Wrap
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    }
                    Label {
                        width: parent.width
                        visible: modelData.exerciseCount > 0
                        text: qsTr("Used by %n exercise(s)", "", modelData.exerciseCount)
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                        truncationMode: TruncationMode.Fade
                    }
                }
            }

            menu: Component {
                ContextMenu {
                    MenuItem {
                        text: qsTr("Try")
                        onClicked: page.open(modelData.id, false, true)
                    }
                    MenuItem {
                        text: qsTr("Edit")
                        onClicked: page.open(modelData.id, false)
                    }
                    MenuItem {
                        text: qsTr("Duplicate")
                        onClicked: page.duplicate(modelData)
                    }
                    MenuItem {
                        text: qsTr("Delete")
                        onClicked: page.confirmDelete(modelData)
                    }
                }
            }
        }

        VerticalScrollDecorator { }
    }
}

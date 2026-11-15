// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0

// The licences of the app and of what it ships. The components, voices and assets come from the
// notices file installed with the app, and the texts from the files installed beside it; they are read
// when the page opens, and what is not installed is not shown. The licence texts are in English.
Page {
    id: page
    allowedOrientations: Orientation.All

    readonly property var notices: licences.notices()
    readonly property var texts: licences.texts()
    readonly property var appTexts: texts.filter(function (t) { return t.group === "app" })
    readonly property var assetTexts: texts.filter(function (t) { return t.group === "asset" })
    readonly property var componentTexts: texts.filter(function (t) { return t.group === "text" })

    // One entry that opens an installed text.
    Component {
        id: textRow
        BackgroundItem {
            width: column.width
            onClicked: pageStack.push(Qt.resolvedUrl("LicenceTextPage.qml"),
                                      { textId: modelData.id, title: modelData.label })

            Label {
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                truncationMode: TruncationMode.Fade
                color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                text: modelData.label
            }
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader { title: qsTr("Licences") }

            SectionHeader { text: qsTr("Workout Trainer") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                text: qsTr("Free software, licensed under the GNU General Public License, version 3 or later (GPL-3.0-or-later).")
            }

            Repeater {
                model: page.appTexts
                delegate: textRow
            }

            SectionHeader {
                visible: page.notices.components.length > 0
                text: qsTr("Components")
            }

            Repeater {
                model: page.notices.components
                delegate: Column {
                    width: column.width

                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        wrapMode: Text.Wrap
                        text: modelData.name
                    }
                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        wrapMode: Text.Wrap
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                        text: modelData.version + ", " + modelData.licence
                    }
                    Item { width: 1; height: Theme.paddingMedium }
                }
            }

            SectionHeader {
                visible: page.notices.voices.length > 0
                text: qsTr("Voices")
            }

            Repeater {
                model: page.notices.voices
                delegate: Label {
                    x: Theme.horizontalPageMargin
                    width: column.width - 2 * Theme.horizontalPageMargin
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.secondaryColor
                    bottomPadding: Theme.paddingMedium
                    text: modelData
                }
            }

            SectionHeader {
                visible: page.notices.assets.length > 0 || page.assetTexts.length > 0
                text: qsTr("Bundled assets")
            }

            Repeater {
                model: page.notices.assets
                delegate: Label {
                    x: Theme.horizontalPageMargin
                    width: column.width - 2 * Theme.horizontalPageMargin
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.secondaryColor
                    bottomPadding: Theme.paddingMedium
                    text: modelData
                }
            }

            Repeater {
                model: page.assetTexts
                delegate: textRow
            }

            SectionHeader {
                visible: page.componentTexts.length > 0
                text: qsTr("Licence texts")
            }

            Repeater {
                model: page.componentTexts
                delegate: textRow
            }
        }

        VerticalScrollDecorator { }
    }
}

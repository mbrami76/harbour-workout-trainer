// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "."

// The choice of an icon and a colour for a record that has both (R-WRK-12). The icons and the
// colours to offer come from the core; the empty key is the default of each and is always offered
// first. The page keeps the chosen keys and learns of a choice from the signals.
Column {
    id: picker

    property var icons: []
    property var colours: []
    property string icon
    property string iconSet: "exercise"
    property string colour

    signal iconPicked(string key)
    signal colourPicked(string key)

    width: parent ? parent.width : Screen.width
    spacing: Theme.paddingMedium

    Label {
        x: Theme.horizontalPageMargin
        text: qsTr("Icon")
        font.pixelSize: Theme.fontSizeSmall
        color: Theme.secondaryHighlightColor
    }

    Flow {
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * Theme.horizontalPageMargin
        spacing: Theme.paddingMedium

        Repeater {
            model: picker.icons
            delegate: MouseArea {
                readonly property bool chosen: picker.icon === modelData
                width: Math.max(Metrics.mm(9), Theme.itemSizeSmall)
                height: width
                onClicked: picker.iconPicked(modelData)
                Accessible.role: Accessible.Button
                Accessible.name: modelData === "" ? qsTr("Default icon") : qsTr("Icon %1").arg(picker.iconSet === "workout" ? WorkoutText.iconName(modelData) : modelData)
                Accessible.checked: chosen

                Rectangle {
                    anchors.fill: parent
                    radius: Theme.paddingSmall
                    color: Theme.rgba(Theme.highlightColor, chosen ? 0.35 : 0.1)
                    border.width: chosen ? Math.max(2, Metrics.mm(0.5)) : 1
                    border.color: chosen ? Theme.highlightColor : Theme.secondaryColor
                }

                IconBadge {
                    anchors.centerIn: parent
                    width: parent.width * 0.8
                    iconKey: modelData
                    iconSet: picker.iconSet
                    colourKey: picker.colour
                }
            }
        }
    }

    Label {
        x: Theme.horizontalPageMargin
        text: qsTr("Colour: %1").arg(Swatches.label(picker.colour))
        font.pixelSize: Theme.fontSizeSmall
        color: Theme.secondaryHighlightColor
    }

    Flow {
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * Theme.horizontalPageMargin
        spacing: Theme.paddingMedium

        Repeater {
            model: [""].concat(picker.colours)
            delegate: MouseArea {
                readonly property bool chosen: picker.colour === modelData
                width: Math.max(Metrics.mm(9), Theme.itemSizeSmall)
                height: width
                onClicked: picker.colourPicked(modelData)
                Accessible.role: Accessible.Button
                Accessible.name: Swatches.label(modelData)
                Accessible.checked: chosen

                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    // The default colour is an empty ring, so it is not taken for a palette colour.
                    color: modelData === "" ? "transparent" : Swatches.color(modelData)
                    border.width: chosen ? Math.max(3, Metrics.mm(0.7)) : 1
                    border.color: Theme.primaryColor
                }

                // A mark on the chosen colour, so that the choice is not shown by colour alone.
                Glyph {
                    anchors.centerIn: parent
                    width: parent.width * 0.5
                    height: width
                    visible: chosen
                    kind: "check"
                    color: modelData === "" ? Theme.primaryColor : BeatStyle.inkOn(Swatches.color(modelData))
                }
            }
        }
    }
}

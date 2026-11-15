// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.6
import Sailfish.Silica 1.0
import "."

// The large icons of a page of modes (R-MOD-1, R-MOD-6) with a label under each. The grid rests on
// the bottom of the page, where the thumb reaches first (R-UI-13), with the rows given by `rowList`
// (the first row on top, the last one lowest). The size comes from the screen (R-UI-12, R-NFR-4): the
// grid is complete at once inside 95 % of the page height and width, never larger than a comfortable
// size, in both orientations. `modes` is a list of {title, icon}; `columns` is the widest row.
Item {
    id: grid

    property var modes: []
    // Indices into `modes`, one list per row, the lowest row last.
    property var rowList: []
    property int columns: 3
    // The height of the page header above the grid, which the grid must not cover.
    property real headerHeight: 0

    signal activated(var mode)

    readonly property real gap: Metrics.mm(2)
    readonly property real availableHeight: height * 0.95 - headerHeight
    readonly property real availableWidth: width * 0.95
    // Two lines of label sit under every icon, 0.3 of the icon size high.
    // The widest row (`columns` tiles with a gap between them) fits inside 95 % of the width.
    readonly property real cell: Math.max(1, Math.min((availableWidth - (columns - 1) * gap) / columns,
                                                      (availableHeight - (rowList.length - 1) * gap) / rowList.length / 1.3,
                                                      Metrics.mm(22)))
    readonly property real iconSize: cell
    readonly property real labelHeight: cell * 0.3
    readonly property real labelSize: Math.max(Metrics.labelSize, cell * 0.105)

    Column {
        id: rows
        anchors.horizontalCenter: parent.horizontalCenter
        // The bottom of the grid rests on the 95 % line, where the thumb reaches it.
        y: grid.height * 0.95 - height
        spacing: grid.gap

        Repeater {
            model: grid.rowList
            delegate: Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: grid.gap

                Repeater {
                    model: modelData
                    delegate: BackgroundItem {
                        id: tile
                        readonly property var mode: grid.modes[modelData]
                        width: grid.cell
                        height: grid.cell + grid.labelHeight
                        onClicked: grid.activated(mode)

                        Accessible.role: Accessible.Button
                        Accessible.name: mode.title

                        Image {
                            id: icon
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: grid.iconSize
                            height: grid.iconSize
                            sourceSize.width: 384
                            sourceSize.height: 384
                            smooth: true
                            mipmap: true
                            source: Qt.resolvedUrl("../../mode-icons/" + mode.icon + ".png")
                        }

                        Label {
                            anchors {
                                top: icon.bottom
                                left: parent.left
                                right: parent.right
                            }
                            height: grid.labelHeight
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignTop
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                            font.pixelSize: grid.labelSize
                            color: tile.highlighted ? Theme.highlightColor : Theme.primaryColor
                            text: mode.title
                        }
                    }
                }
            }
        }
    }
}

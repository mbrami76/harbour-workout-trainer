// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.6
import Sailfish.Silica 1.0
import "."

// The icon of a saved record on a disc of its colour. The icon set is "exercise" (R-EXR-8) or
// "workout" (R-WRK-12); an icon key without a picture in its set shows the default icon. The
// pictures are one dark colour on transparent and sit on a white disc inside the disc of the
// colour, so they stay visible in the light and in the dark ambience. The badge carries no meaning
// alone: the name is always shown beside it.
Rectangle {
    id: badge

    property string iconKey
    property string colourKey
    property string iconSet: "exercise"

    readonly property string defaultSource: Qt.resolvedUrl("../../exercise-icons/default.png")

    width: Metrics.mm(11)
    height: width
    radius: width / 2
    color: Swatches.color(colourKey)
    border.width: Math.max(1, Metrics.mm(0.3))
    border.color: Theme.primaryColor

    Rectangle {
        anchors.centerIn: parent
        width: parent.width * 0.78
        height: width
        radius: width / 2
        color: "white"
    }

    Image {
        anchors.centerIn: parent
        width: parent.width * 0.7
        height: width
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        sourceSize.width: 128
        sourceSize.height: 128
        source: badge.iconKey === "" ? badge.defaultSource
                : Qt.resolvedUrl("../../" + badge.iconSet + "-icons/" + badge.iconKey + ".png")
        onStatusChanged: {
            if (status === Image.Error && source !== badge.defaultSource)
                source = badge.defaultSource
        }
    }
}

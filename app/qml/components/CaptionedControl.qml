// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "."

// A control of the session page with a short caption under it (R-UI-5): the control is the one
// child, a BigButton or a HoldButton, which fills the width and the height above the caption.
Item {
    id: cell

    property string caption
    property real buttonHeight: Metrics.buttonSize
    property real captionHeight: Metrics.labelSize * 1.3
    default property alias content: slot.data

    implicitHeight: buttonHeight + captionHeight

    Item {
        id: slot
        width: parent.width
        height: cell.buttonHeight
    }

    Label {
        y: cell.buttonHeight
        width: parent.width
        height: cell.captionHeight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: Metrics.labelSize
        fontSizeMode: Text.Fit
        minimumPixelSize: Metrics.labelSize * 0.8
        color: cell.enabled ? Theme.secondaryHighlightColor : Theme.secondaryColor
        text: cell.caption
    }
}

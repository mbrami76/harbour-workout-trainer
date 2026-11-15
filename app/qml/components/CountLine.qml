// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0

// One line of a summary that is shown only when its number is not zero.
Label {
    property int n

    x: Theme.horizontalPageMargin
    width: parent.width - 2 * Theme.horizontalPageMargin
    wrapMode: Text.Wrap
    visible: n > 0
    height: visible ? implicitHeight : 0
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.6
import Sailfish.Silica 1.0
import "../components"

// The dashboard (R-DSH-1 to R-DSH-3, R-DSH-5): the exercise time of the last 7 days and of the four
// weeks before, of the current month and of the six months before, each with its average per day,
// then the five muscle groups with the most and the least time over the last 7 days. It is a page
// to read, not a run page: it uses the theme sizes, so the system font size applies, and it scrolls.
Page {
    id: page

    allowedOrientations: Orientation.All

    property var figures: ({ ok: true, sessionsInWeek: 0, weeks: [], months: [], most: [], least: [] })

    readonly property var locale: Qt.locale()
    readonly property real barHeight: Metrics.mm(2.5)

    function reload() {
        figures = dashboard.overview()
    }

    Component.onCompleted: reload()
    onStatusChanged: {
        if (status === PageStatus.Active)
            reload()
    }

    function number(value, decimals) {
        return Number(value).toLocaleString(locale, "f", decimals)
    }

    // "1 h 05 min", "35 min" or "40 s", from whole seconds.
    function duration(sec) {
        if (sec < 60)
            return qsTr("%1 s").arg(number(sec, 0))
        var minutes = Math.round(sec / 60)
        var hours = Math.floor(minutes / 60)
        if (hours === 0)
            return qsTr("%1 min").arg(number(minutes, 0))
        return qsTr("%1 h %2 min").arg(number(hours, 0)).arg(number(minutes % 60, 0))
    }

    function day(iso) {
        return new Date(iso + "T12:00:00")
    }

    function weekName(period, index) {
        if (index === 0)
            return qsTr("Last 7 days")
        return qsTr("From %1").arg(Qt.formatDate(day(period.first), locale.dateFormat(Locale.ShortFormat)))
    }

    function monthName(period, index) {
        var d = day(period.first)
        var name = locale.monthName(d.getMonth(), Locale.LongFormat) + " " + d.getFullYear()
        return index === 0 ? qsTr("%1, so far").arg(name) : name
    }

    function largest(periods) {
        var max = 0
        for (var i = 0; i < periods.length; ++i)
            max = Math.max(max, periods[i].totalSec)
        return max
    }

    // One period: its name and total on a line, the bar under it, the average under that.
    Component {
        id: periodRow
        Column {
            id: row
            property string name
            property var period
            property real maxSec
            width: ListView.view ? ListView.view.width : parent.width

            Item {
                width: parent.width
                height: Math.max(nameLabel.implicitHeight, totalLabel.implicitHeight)

                Label {
                    id: totalLabel
                    anchors.right: parent.right
                    text: row.period.totalSec > 0 ? page.duration(row.period.totalSec) : qsTr("No session")
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.highlightColor
                }
                Label {
                    id: nameLabel
                    anchors {
                        left: parent.left
                        right: totalLabel.left
                        rightMargin: Theme.paddingMedium
                    }
                    text: row.name
                    font.pixelSize: Theme.fontSizeSmall
                    truncationMode: TruncationMode.Fade
                }
            }
            Rectangle {
                width: parent.width
                height: page.barHeight
                radius: height / 2
                color: Theme.rgba(Theme.secondaryColor, 0.2)

                Rectangle {
                    width: row.maxSec > 0 ? Math.max(row.period.totalSec > 0 ? height : 0,
                                                     parent.width * row.period.totalSec / row.maxSec) : 0
                    height: parent.height
                    radius: height / 2
                    color: Theme.highlightColor
                }
            }
            Label {
                width: parent.width
                visible: row.period.totalSec > 0
                text: qsTr("%1 min per day on average").arg(page.number(row.period.averageMinutes, 1))
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                truncationMode: TruncationMode.Fade
            }
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height + Theme.paddingLarge

        Column {
            id: content
            width: parent.width

            PageHeader { title: qsTr("Dashboard") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: !page.figures.ok
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                text: qsTr("The session log cannot be read.")
            }

            SectionHeader { text: qsTr("Exercise time per week") }
            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingMedium

                Repeater {
                    model: page.figures.weeks
                    delegate: Loader {
                        width: parent.width
                        sourceComponent: periodRow
                        onLoaded: {
                            item.name = page.weekName(modelData, index)
                            item.period = modelData
                            item.maxSec = page.largest(page.figures.weeks)
                        }
                    }
                }
            }

            SectionHeader { text: qsTr("Exercise time per month") }
            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingMedium

                Repeater {
                    model: page.figures.months
                    delegate: Loader {
                        width: parent.width
                        sourceComponent: periodRow
                        onLoaded: {
                            item.name = page.monthName(modelData, index)
                            item.period = modelData
                            item.maxSec = page.largest(page.figures.months)
                        }
                    }
                }
            }

            SectionHeader { text: qsTr("Most trained, last 7 days") }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.figures.most.length === 0
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
                text: qsTr("No exercise time in the last 7 days.")
            }
            Repeater {
                model: page.figures.most
                delegate: groupRow
            }

            SectionHeader { text: qsTr("Least trained, last 7 days") }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.figures.sessionsInWeek === 0
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
                text: qsTr("No session in the last 7 days, so every group counts as untrained.")
            }
            Repeater {
                model: page.figures.least
                delegate: groupRow
            }
        }

        VerticalScrollDecorator { }
    }

    // A group with its time: the name on the left, the time on the right.
    Component {
        id: groupRow
        Item {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            height: Math.max(groupName.implicitHeight, groupTime.implicitHeight) + Theme.paddingSmall

            Label {
                id: groupTime
                anchors.right: parent.right
                text: modelData.seconds > 0 ? page.duration(modelData.seconds) : qsTr("None")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.highlightColor
            }
            Label {
                id: groupName
                anchors {
                    left: parent.left
                    right: groupTime.left
                    rightMargin: Theme.paddingMedium
                }
                text: MuscleText.label(modelData.key)
                font.pixelSize: Theme.fontSizeSmall
                truncationMode: TruncationMode.Fade
            }
        }
    }
}

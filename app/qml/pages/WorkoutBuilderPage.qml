// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// Creates or edits a workout (R-WRK-1 to R-WRK-6, R-WRK-8 to R-WRK-13): name, icon and colour, the
// rest between blocks, and the blocks with their steps. A tap on a block or on a step opens its
// page; the page of a step reaches the exercise picker. The draft lives in workoutBuilder, not in
// this page, so everything the pages above this one do leaves what is entered intact. The total
// planned time is shown live; Save is possible only when the core finds nothing to refuse, and
// the first finding is shown beside it. Leaving with the back gesture drops unsaved changes
// without asking.
Page {
    id: page

    // The workout to edit; 0 for a new one.
    property int workoutId
    readonly property bool creating: workoutId === 0
    property bool loaded: false
    // Set when the store failed on Save for a reason other than a finding.
    property bool saveError: false

    readonly property var limits: workoutBuilder.limits
    readonly property var total: workoutBuilder.total
    readonly property string reason: WorkoutText.firstProblem(workoutBuilder.problems, limits)

    allowedOrientations: Orientation.All

    function totalLine() {
        var text = WorkoutText.totalText(total)
        if (total.uncounted > 0)
            text += " (" + qsTr("%n step(s) not counted", "", total.uncounted) + ")"
        return text
    }

    function openStep(block, step) {
        pageStack.push(Qt.resolvedUrl("WorkoutStepPage.qml"), { block: block, step: step })
    }

    function addStep(block) {
        var p = workoutBuilder.addStep(block)
        if (p.block !== undefined)
            openStep(p.block, p.step)
    }

    function addBlock() {
        var p = workoutBuilder.addBlock()
        if (p.block !== undefined)
            pageStack.push(Qt.resolvedUrl("WorkoutBlockPage.qml"), { block: p.block })
    }

    function save() {
        if (!workoutBuilder.canSave)
            return
        var r = workoutBuilder.save()
        if (r.status === "ok")
            pageStack.pop()
        else if (r.status !== "invalid")
            saveError = true
    }

    Component.onCompleted: loaded = workoutBuilder.begin(workoutId)
    onStatusChanged: {
        if (status === PageStatus.Active && loaded)
            workoutBuilder.refreshLookups()
    }

    SilicaListView {
        id: list
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: bar.top
        }
        clip: true
        model: page.loaded ? workoutBuilder.rows : null
        cacheBuffer: height * 2

        header: Column {
            width: list.width
            spacing: Theme.paddingMedium

            PageHeader { title: page.creating ? qsTr("New workout") : qsTr("Edit workout") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: !page.loaded
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
                text: qsTr("This workout could not be opened.")
            }

            TextField {
                width: parent.width
                visible: page.loaded
                label: qsTr("Name")
                placeholderText: qsTr("Name")
                text: workoutBuilder.header.name
                color: workoutBuilder.problems.some(function (p) { return p.code === "name-too-long" || p.code === "identity" })
                       ? Theme.errorColor : Theme.primaryColor
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
                onTextChanged: {
                    if (activeFocus)
                        workoutBuilder.setName(text)
                }
            }

            IconColourPicker {
                visible: page.loaded
                icons: workoutBuilder.icons
                iconSet: "workout"
                colours: workoutBuilder.colours
                icon: workoutBuilder.header.icon
                colour: workoutBuilder.header.colour
                onIconPicked: workoutBuilder.setIcon(key)
                onColourPicked: workoutBuilder.setColour(key)
            }

            NumberStepper {
                id: blockRest
                visible: page.loaded
                label: qsTr("Rest between blocks")
                minimum: page.limits.minRestSec
                maximum: page.limits.maxRestSec
                valueText: WorkoutText.duration(value)
                onEdited: workoutBuilder.setRestBetweenBlocks(value)
            }

            // The stepper assigns its own value when a button is pressed; this keeps it following the draft.
            Binding {
                target: blockRest
                property: "value"
                value: workoutBuilder.header.restBetweenBlocksSec
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.loaded && page.total.uncounted > 0
                wrapMode: Text.Wrap
                textFormat: Text.PlainText
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.highlightColor
                text: WorkoutText.uncountedText(page.total)
            }

            SectionHeader {
                visible: page.loaded
                text: qsTr("Blocks and steps")
            }

            // Arranging shows every block as one line with buttons that move it, and the steps of a
            // block that is opened with buttons that move them, also into the neighbouring block (R-WRK-6).
            TextSwitch {
                width: parent.width
                height: Math.max(implicitHeight, Metrics.mm(9))
                visible: page.loaded
                text: qsTr("Arrange blocks and steps")
                description: qsTr("Blocks are shown as one line each; open a block to move its steps.")
                checked: workoutBuilder.arranging
                automaticCheck: false
                onClicked: workoutBuilder.setArranging(!checked)
            }
        }

        footer: Column {
            width: list.width
            visible: page.loaded

            Item {
                width: 1
                height: Theme.paddingLarge
            }

            BigButton {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Add block")
                onActivated: page.addBlock()
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: visible ? implicitHeight + Theme.paddingLarge : 0
                visible: page.saveError
                wrapMode: Text.Wrap
                color: Theme.errorColor
                text: qsTr("The workout could not be saved.")
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }
        }

        delegate: Loader {
            width: ListView.view.width
            sourceComponent: model.row.kind === "block" ? blockRow
                           : (model.row.kind === "step" ? stepRow : addRow)
            property var row: model.row
        }

        VerticalScrollDecorator { }
    }

    Component {
        id: blockRow
        BackgroundItem {
            property var row: parent ? parent.row : ({})
            height: Math.max(Metrics.mm(9) + Theme.paddingMedium, blockColumn.height + 2 * Theme.paddingMedium)
            onClicked: pageStack.push(Qt.resolvedUrl("WorkoutBlockPage.qml"), { block: row.block })
            Accessible.role: Accessible.Button
            Accessible.name: WorkoutText.blockTitle(row) + ", " + WorkoutText.blockDetails(row)

            // Collapses or expands the block; only a view (R-WRK-6).
            IconButton {
                id: fold
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin - Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                width: Math.max(Metrics.mm(9), Theme.itemSizeSmall)
                height: width
                icon.source: row.collapsed ? "image://theme/icon-m-right" : "image://theme/icon-m-down"
                onClicked: workoutBuilder.toggleBlockCollapsed(row.block)
                Accessible.name: row.collapsed ? qsTr("Expand block %1").arg(row.block + 1) : qsTr("Collapse block %1").arg(row.block + 1)
            }

            Column {
                id: blockColumn
                anchors {
                    left: fold.right
                    leftMargin: Theme.paddingSmall
                    right: moveBlock.left
                    rightMargin: workoutBuilder.arranging ? Theme.paddingSmall : Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                Label {
                    width: parent.width
                    font.pixelSize: Theme.fontSizeMedium
                    color: highlighted ? Theme.highlightColor : Theme.primaryColor
                    truncationMode: TruncationMode.Fade
                    text: WorkoutText.blockTitle(row)
                }
                Label {
                    width: parent.width
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    wrapMode: Text.Wrap
                    maximumLineCount: row.collapsed ? 1 : 3
                    elide: Text.ElideRight
                    text: row.collapsed ? WorkoutText.blockLine(row) : WorkoutText.blockDetails(row)
                }
                Label {
                    width: parent.width
                    visible: row.problems.length > 0
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.errorColor
                    wrapMode: Text.Wrap
                    text: row.problems.length > 0 ? WorkoutText.problem(row.problems[0], page.limits) : ""
                }
            }

            // Moves the block up or down while arranging.
            Row {
                id: moveBlock
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin - Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                visible: workoutBuilder.arranging
                width: visible ? implicitWidth : 0
                IconButton {
                    width: Math.max(Metrics.mm(9), Theme.itemSizeSmall)
                    height: width
                    enabled: row.block > 0
                    icon.source: "image://theme/icon-m-up"
                    onClicked: workoutBuilder.moveBlock(row.block, -1)
                    Accessible.name: qsTr("Move block %1 up").arg(row.block + 1)
                }
                IconButton {
                    width: Math.max(Metrics.mm(9), Theme.itemSizeSmall)
                    height: width
                    enabled: row.block < workoutBuilder.header.blockCount - 1
                    icon.source: "image://theme/icon-m-down"
                    onClicked: workoutBuilder.moveBlock(row.block, 1)
                    Accessible.name: qsTr("Move block %1 down").arg(row.block + 1)
                }
            }
        }
    }

    Component {
        id: stepRow
        BackgroundItem {
            property var row: parent ? parent.row : ({})
            height: Math.max(Metrics.mm(9) + Theme.paddingMedium, stepColumn.height + 2 * Theme.paddingSmall)
            onClicked: page.openStep(row.block, row.step)
            Accessible.role: Accessible.Button
            Accessible.name: WorkoutText.stepTitle(row, StepStyle.label(row.type)) + ", " + WorkoutText.stepDetails(row)

            Rectangle {
                id: typeBar
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(2, Metrics.mm(1))
                height: parent.height - 2 * Theme.paddingSmall
                radius: width / 2
                color: StepStyle.color(row.type)
            }

            Column {
                id: stepColumn
                anchors {
                    left: typeBar.right
                    leftMargin: Theme.paddingMedium
                    right: moveStep.left
                    rightMargin: workoutBuilder.arranging ? Theme.paddingSmall : Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                Label {
                    width: parent.width
                    textFormat: Text.PlainText
                    font.pixelSize: Theme.fontSizeSmall
                    color: highlighted ? Theme.highlightColor : Theme.primaryColor
                    wrapMode: Text.Wrap
                    text: WorkoutText.stepTitle(row, StepStyle.label(row.type))
                }
                Label {
                    width: parent.width
                    textFormat: Text.PlainText
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    wrapMode: Text.Wrap
                    text: WorkoutText.stepDetails(row)
                }
                Label {
                    width: parent.width
                    visible: row.problems.length > 0
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.errorColor
                    wrapMode: Text.Wrap
                    text: row.problems.length > 0 ? WorkoutText.problem(row.problems[0], page.limits) : ""
                }
            }

            // Moves the step up or down while arranging; at the end of a block it goes into the next
            // one (R-WRK-6).
            Row {
                id: moveStep
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin - Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                visible: workoutBuilder.arranging
                width: visible ? implicitWidth : 0
                IconButton {
                    width: Math.max(Metrics.mm(9), Theme.itemSizeSmall)
                    height: width
                    enabled: row.canMoveUp
                    icon.source: "image://theme/icon-m-up"
                    onClicked: workoutBuilder.moveStepAcross(row.block, row.step, -1)
                    Accessible.name: qsTr("Move step %1 up").arg(row.step + 1)
                }
                IconButton {
                    width: Math.max(Metrics.mm(9), Theme.itemSizeSmall)
                    height: width
                    enabled: row.canMoveDown
                    icon.source: "image://theme/icon-m-down"
                    onClicked: workoutBuilder.moveStepAcross(row.block, row.step, 1)
                    Accessible.name: qsTr("Move step %1 down").arg(row.step + 1)
                }
            }
        }
    }

    Component {
        id: addRow
        BackgroundItem {
            property var row: parent ? parent.row : ({})
            height: Math.max(Metrics.mm(9) + Theme.paddingMedium, Theme.itemSizeSmall)
            onClicked: page.addStep(row.block)
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Add step to block %1").arg(row.block + 1)

            Label {
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin + Theme.paddingLarge
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                font.pixelSize: Theme.fontSizeSmall
                color: highlighted ? Theme.highlightColor : Theme.secondaryHighlightColor
                text: qsTr("Add step to block %1").arg(row.block + 1)
            }
        }
    }

    // The total planned time, the first reason Save is off and Save itself rest at the bottom
    // (R-UI-13).
    Item {
        id: bar
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: Math.max(texts.height, saveButton.height) + 2 * Theme.paddingMedium
        visible: page.loaded

        Column {
            id: texts
            anchors {
                left: parent.left
                leftMargin: Theme.horizontalPageMargin
                right: saveButton.left
                rightMargin: Theme.paddingMedium
                verticalCenter: parent.verticalCenter
            }
            Label {
                width: parent.width
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.highlightColor
                wrapMode: Text.Wrap
                maximumLineCount: 2
                text: page.totalLine()
            }
            Label {
                width: parent.width
                visible: page.reason !== ""
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.errorColor
                wrapMode: Text.Wrap
                maximumLineCount: 3
                textFormat: Text.PlainText
                text: page.reason
            }
        }

        BigButton {
            id: saveButton
            anchors {
                right: parent.right
                rightMargin: Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }
            width: Math.max(implicitWidth, parent.width * 0.3)
            text: qsTr("Save")
            selected: workoutBuilder.canSave
            enabled: workoutBuilder.canSave
            opacity: enabled ? 1 : 0.4
            onActivated: page.save()
        }
    }
}

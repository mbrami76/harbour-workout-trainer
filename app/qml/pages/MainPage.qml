// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import "../components"

// The first page (R-MOD-1): five large icons with a label under each. Quick start and Dashboard (the
// normal colour) rest in the lowest row, where the thumb reaches first; above them are Workouts,
// Exercises and Metronome, the advanced entries (another tile shape and colour). The size comes from the screen
// (R-UI-12, R-NFR-4) and the grid is complete at once inside 95 % of the page height.
Page {
    id: page

    allowedOrientations: Orientation.All

    readonly property var modes: [
        { title: qsTr("Quick start"), icon: "quick-start", page: "QuickStartPage.qml" },
        { title: qsTr("Workouts"), icon: "workouts", page: "WorkoutListPage.qml" },
        { title: qsTr("Exercises"), icon: "exercises", page: "ExerciseListPage.qml" },
        { title: qsTr("Metronome"), icon: "metronome", page: "MetronomeListPage.qml" },
        { title: qsTr("Dashboard"), icon: "dashboard", page: "DashboardPage.qml" }
    ]
    // The advanced entries in the upper row, Quick start and Dashboard in the lowest one.
    readonly property var rowList: [[1, 2, 3], [0, 4]]

    // The file chosen for an import. The preview opens when this page is on top again, because the
    // picker closes itself after a choice.
    property string pendingImport

    function exportAll() {
        pageStack.push(Qt.resolvedUrl("ExportResultPage.qml"), { result: exchangeBridge.exportAll() })
    }

    function openImportPreview(path) {
        pageStack.push(Qt.resolvedUrl("ImportPreviewPage.qml"), { preview: exchangeBridge.previewFile(path) })
    }

    onStatusChanged: {
        if (status === PageStatus.Active && pendingImport !== "") {
            var path = pendingImport
            pendingImport = ""
            openImportPreview(path)
        }
    }

    Component {
        id: importPicker
        FilePickerPage {
            nameFilters: ["*.json"]
            onSelectedContentPropertiesChanged: {
                if (selectedContentProperties && selectedContentProperties.filePath)
                    page.pendingImport = selectedContentProperties.filePath
            }
        }
    }

    function open(mode) {
        if (mode.page !== "")
            pageStack.push(Qt.resolvedUrl(mode.page))
        else
            pageStack.push(Qt.resolvedUrl("PlaceholderPage.qml"), { title: mode.title })
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: height

        PullDownMenu {
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
            MenuItem {
                text: qsTr("About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }
            MenuItem {
                text: qsTr("Export all workouts")
                onClicked: page.exportAll()
            }
            MenuItem {
                text: qsTr("Import workouts")
                onClicked: pageStack.push(importPicker)
            }
        }

        PageHeader {
            id: header
            title: qsTr("Workout Trainer")
        }
    }

    // The grid sits on the page, not in the flickable, so that it takes the size of the screen.
    ModeGrid {
        anchors.fill: parent
        modes: page.modes
        rowList: page.rowList
        columns: 3
        headerHeight: header.height
        onActivated: page.open(mode)
    }
}

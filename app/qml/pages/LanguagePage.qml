// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

// Shown at the first start, before the first page (R-I18N-5). The text is shown in every shipped
// language at once, because the interface language is not chosen yet. The page cannot be left
// without a choice; the system language is preselected when the app ships it.
Page {
    id: page

    property string chosen: languageBridge.systemLanguage

    allowedOrientations: Orientation.All
    backNavigation: false
    forwardNavigation: false
    showNavigationIndicator: false

    function confirm() {
        Settings.setLanguage(chosen)
        languageBridge.apply(chosen)
        pageStack.replace(Qt.resolvedUrl(Settings.safetyNoticeRead ? "MainPage.qml" : "SafetyNoticePage.qml"))
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            Item { width: 1; height: Theme.paddingLarge }

            Repeater {
                model: languageBridge.languages
                Label {
                    x: Theme.horizontalPageMargin
                    width: column.width - 2 * Theme.horizontalPageMargin
                    wrapMode: Text.WordWrap
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeLarge
                    text: languageBridge.translated(modelData, "LanguagePage",
                                                    QT_TRANSLATE_NOOP("LanguagePage", "Choose your language"))
                }
            }

            Repeater {
                model: languageBridge.languages
                BackgroundItem {
                    id: row
                    readonly property bool selected: page.chosen === modelData
                    width: column.width
                    highlighted: down || selected
                    onClicked: page.chosen = modelData

                    Label {
                        anchors {
                            left: parent.left
                            leftMargin: Theme.horizontalPageMargin
                            right: mark.left
                            rightMargin: Theme.paddingMedium
                            verticalCenter: parent.verticalCenter
                        }
                        text: LanguageNames.name(modelData)
                        wrapMode: Text.WordWrap
                        color: row.highlighted ? Theme.highlightColor : Theme.primaryColor
                    }

                    // The mark on the chosen row, so that the choice is not shown by colour alone.
                    Glyph {
                        id: mark
                        anchors {
                            right: parent.right
                            rightMargin: Theme.horizontalPageMargin
                            verticalCenter: parent.verticalCenter
                        }
                        width: Theme.iconSizeSmall
                        height: width
                        visible: row.selected
                        kind: "check"
                        color: Theme.highlightColor
                    }
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: languageBridge.translated(page.chosen, "LanguagePage", QT_TRANSLATE_NOOP("LanguagePage", "Continue"))
                onClicked: page.confirm()
            }
        }

        VerticalScrollDecorator { }
    }
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

import QtQuick 2.0
import Sailfish.Silica 1.0
import "pages"
import "components"

ApplicationWindow {
    // The shell opens the main page; when the data cannot be used it shows the error page and
    // touches nothing else. Before the first choice of the language the language page comes
    // first (R-I18N-5), then the safety notice until it is confirmed (R-GEN-5).
    initialPage: {
        var first = languageBridge.firstPage(services.ready, Settings.language, Settings.safetyNoticeRead)
        if (first === "error")
            return errorPage
        if (first === "language")
            return languagePage
        // At the start the translation is put in place before the first page is created. When the
        // interface is loaded again after a change, the language is already in use and the stored
        // value may not have been read back yet.
        if (languageBridge.language === "")
            languageBridge.apply(Settings.language)
        return first === "notice" ? noticePage : mainPage
    }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations

    // The playback service reads the switches of the settings through this link.
    Component.onCompleted: {
        Sounds.settings = Settings
        // After a change of language the interface was loaded again: the settings page comes back.
        if (languageBridge.takeReopenSettings())
            reopen.start()
    }

    // The switch of the settings reaches the speech of the app.
    Binding { target: speech; property: "enabled"; value: Settings.spokenAnnouncements }
    Binding { target: speech; property: "cues"; value: Settings.spokenCues }
    Binding { target: speech; property: "phaseNames"; value: Settings.spokenPhaseNames }

    Timer {
        id: reopen
        interval: 1
        onTriggered: pageStack.push(Qt.resolvedUrl("pages/SettingsPage.qml"), {}, PageStackAction.Immediate)
    }

    Component { id: mainPage; MainPage { } }
    Component { id: noticePage; SafetyNoticePage { } }
    Component { id: languagePage; LanguagePage { } }
    Component { id: errorPage; ErrorPage { code: services.error } }
}

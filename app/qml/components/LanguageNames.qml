// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

pragma Singleton
import QtQuick 2.0

// The name of each shipped language written in that language, so that a reader finds their own
// whatever the interface shows now (R-I18N-5). English is the source language of the texts.
QtObject {
    readonly property var sources: ({
        "en": QT_TRANSLATE_NOOP("LanguageNames", "English"),
        "it": QT_TRANSLATE_NOOP("LanguageNames", "Italian")
    })

    function name(tag) {
        return sources[tag] === undefined ? tag : languageBridge.translated(tag, "LanguageNames", sources[tag])
    }
}

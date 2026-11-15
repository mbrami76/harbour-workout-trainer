// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef LANGUAGEBRIDGE_H
#define LANGUAGEBRIDGE_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>

class QQuickView;
class QTranslator;
class Services;

// The interface language (R-I18N-1, R-I18N-5). It installs the translation of the chosen language,
// offers the starter library of that language and hands the language to the services for the tag
// of new exercises. English is the source language; its translator answers with the source text.
// The Qt of the platform cannot translate a running QML scene again, so a change of language made
// in the settings loads the interface again from its main file, at once and without a restart of
// the app, and the settings page is shown again. Registered next to "services" as the context
// property "languageBridge".
class LanguageBridge : public QObject
{
    Q_OBJECT
    // The language in use, empty before the first choice.
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)
    // The shipped language tags, English first.
    Q_PROPERTY(QStringList languages READ languages CONSTANT)
    // The language of the system when the app ships it, otherwise English.
    Q_PROPERTY(QString systemLanguage READ systemLanguage CONSTANT)

public:
    // translationDirectory holds the compiled translations (harbour-workout-trainer-<tag>.qm); the view
    // shows the interface that starts from mainQml.
    LanguageBridge(Services *services, QQuickView *view, const QUrl &mainQml, const QString &translationDirectory,
                   QObject *parent = nullptr);

    QString language() const { return language_; }
    QStringList languages() const;
    QString systemLanguage() const;

    // Makes the tag the interface language for the pages created from now on and offers its starter
    // library once the event loop is idle. A tag that is not shipped is ignored. Used at the start
    // and after the first choice, when no page holds a text of another language.
    Q_INVOKABLE void apply(const QString &language);
    // Same for a change of language while the interface is shown: the interface is loaded again.
    Q_INVOKABLE void switchTo(const QString &language);
    // True once after a change made by switchTo(): the interface shows the settings page again.
    Q_INVOKABLE bool takeReopenSettings();
    // The page to show first: "error", "language", "notice" or "main" (R-I18N-5, R-GEN-5). The
    // stored language and the stored confirmation of the notice are passed as read.
    Q_INVOKABLE QString firstPage(bool dataReady, const QString &storedLanguage, const QVariant &noticeConfirmed) const;
    // The shipped tag a stored value stands for, or an empty string.
    Q_INVOKABLE QString shippedFrom(const QString &value) const;
    // A text of the given context in another language, without changing the interface: the
    // language page shows its text in every language at once. The source text is returned when the
    // language has no translation of it.
    Q_INVOKABLE QString translated(const QString &language, const QString &context, const QString &text);
    // The translator of a language, loaded for this purpose and not installed, or nullptr when the
    // language has no translation. The spoken sentences are looked up in it (R-ALR-6).
    QTranslator *translatorOf(const QString &language) { return translatorFor(language); }

signals:
    void languageChanged();

private:
    QTranslator *translatorFor(const QString &language);

    Services *services_;
    QQuickView *view_;
    QUrl mainQml_;
    bool reopenSettings_ = false;
    QString directory_;
    QString language_;
    bool applied_ = false;
    // Translators loaded so far, one per language; a language without a file maps to null.
    QHash<QString, QTranslator *> loaded_;
    // The translator installed in the application.
    QTranslator *installed_ = nullptr;
};

#endif

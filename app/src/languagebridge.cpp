// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "languagebridge.h"

#include <QCoreApplication>
#include <QLocale>
#include <QQmlEngine>
#include <QQuickView>
#include <QTimer>
#include <QTranslator>

#include "language.h"
#include "startflow.h"
#include "services.h"

namespace {

// English is the source language: this translator, installed last, answers every text with the
// source text, so that the translator the platform installs for the system language (a language
// the user did not choose) is never reached.
class SourceTextTranslator : public QTranslator
{
public:
    using QTranslator::QTranslator;
    QString translate(const char *, const char *sourceText, const char *, int) const override
    {
        return QString::fromUtf8(sourceText);
    }
    bool isEmpty() const override { return false; }
};

}

LanguageBridge::LanguageBridge(Services *services, QQuickView *view, const QUrl &mainQml,
                               const QString &translationDirectory, QObject *parent)
    : QObject(parent), services_(services), view_(view), mainQml_(mainQml), directory_(translationDirectory)
{
}

QStringList LanguageBridge::languages() const
{
    return Language::shipped();
}

QString LanguageBridge::systemLanguage() const
{
    return Language::preselected(QLocale::system().name());
}

QString LanguageBridge::shippedFrom(const QString &value) const
{
    return Language::shippedFrom(value);
}

QTranslator *LanguageBridge::translatorFor(const QString &language)
{
    if (!Language::isShipped(language))
        return nullptr;
    if (loaded_.contains(language))
        return loaded_.value(language);
    if (language == QLatin1String("en")) {
        QTranslator *source = new SourceTextTranslator(this);
        loaded_.insert(language, source);
        return source;
    }
    QTranslator *translator = new QTranslator(this);
    if (!translator->load(QStringLiteral("harbour-workout-trainer-%1").arg(language), directory_)) {
        qWarning("Translation of %s could not be loaded", qPrintable(language));
        delete translator;
        translator = nullptr;
    }
    loaded_.insert(language, translator);
    return translator;
}

QString LanguageBridge::firstPage(bool dataReady, const QString &storedLanguage, const QVariant &noticeConfirmed) const
{
    return StartFlow::pageName(StartFlow::firstPage(dataReady, storedLanguage, StartFlow::noticeConfirmed(noticeConfirmed)));
}

QString LanguageBridge::translated(const QString &language, const QString &context, const QString &text)
{
    QTranslator *translator = translatorFor(language);
    if (!translator)
        return text;
    const QString result = translator->translate(context.toUtf8().constData(), text.toUtf8().constData());
    return result.isEmpty() ? text : result;
}

void LanguageBridge::apply(const QString &language)
{
    if (!Language::isShipped(language))
        return;
    if (!applied_ || language != language_) {
        applied_ = true;
        if (installed_)
            QCoreApplication::removeTranslator(installed_);
        installed_ = translatorFor(language);
        if (installed_)
            QCoreApplication::installTranslator(installed_);
    }
    services_->setLanguage(language);
    if (language != language_) {
        language_ = language;
        emit languageChanged();
    }
    // The starter library is offered when the loop is idle, so that the first page is not delayed.
    QTimer::singleShot(0, services_, [this, language]() { services_->loadStarters(language); });
}

void LanguageBridge::switchTo(const QString &language)
{
    if (!Language::isShipped(language) || language == language_)
        return;
    apply(language);
    reopenSettings_ = true;
    // The scene is replaced after the handler that asked for it has returned.
    QTimer::singleShot(0, this, [this]() {
        view_->setSource(QUrl());
        view_->engine()->clearComponentCache();
        view_->setSource(mainQml_);
    });
}

bool LanguageBridge::takeReopenSettings()
{
    const bool pending = reopenSettings_;
    reopenSettings_ = false;
    return pending;
}

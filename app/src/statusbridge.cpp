// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include "statusbridge.h"

#include <QDateTime>
#include <QGuiApplication>
#include <QLocale>

#include "languagebridge.h"
#include "statusline.h"

StatusBridge::StatusBridge(LanguageBridge *language, QObject *parent)
    : QObject(parent), language_(language)
{
    timer_.setSingleShot(true);
    connect(&timer_, &QTimer::timeout, this, &StatusBridge::refresh);
    connect(qApp, &QGuiApplication::applicationStateChanged, this, &StatusBridge::applicationStateChanged);
    if (language_)
        connect(language_, &LanguageBridge::languageChanged, this, &StatusBridge::refresh);
    refresh();
}

QLocale StatusBridge::locale() const
{
    const QString tag = language_ ? language_->language() : QString();
    return tag.isEmpty() ? QLocale::system() : QLocale(tag);
}

// Reads the time and the battery, tells QML only if one of them changed, and sets the timer to the
// start of the next minute. A timer that is not running while the app is in the background is set
// again when the app is active.
void StatusBridge::refresh()
{
    const QDateTime now = QDateTime::currentDateTime();
    const QString time = StatusLine::timeText(now, locale());
    const QString date = StatusLine::dateText(now, locale());
    const int battery = StatusLine::readBattery(QString::fromLatin1(StatusLine::batteryFile));
    if (time != time_ || date != date_ || battery != battery_) {
        time_ = time;
        date_ = date;
        battery_ = battery;
        emit changed();
    }
    if (QGuiApplication::applicationState() == Qt::ApplicationActive)
        timer_.start(int(StatusLine::msToNextMinute(now)));
}

void StatusBridge::applicationStateChanged(Qt::ApplicationState state)
{
    if (state == Qt::ApplicationActive)
        refresh();
    else
        timer_.stop();
}

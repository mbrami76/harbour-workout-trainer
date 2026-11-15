// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef STATUSBRIDGE_H
#define STATUSBRIDGE_H

#include <QLocale>
#include <QObject>
#include <QString>
#include <QTimer>

class LanguageBridge;

// The status line of the session screen (R-UI-15): the time of day, the date and the battery
// charge, for QML as the context property "statusLine". The time and the date are redrawn at each
// change of minute, from one single-shot timer set to the next minute, and when the app becomes
// active again; the battery is read at those moments and announced only when it changed. There is
// no faster timer. The battery is read from the system file of the charge; without a readable
// value `batteryPercent` is -1 and the screen shows no battery. Nothing is shown for the network.
class StatusBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString time READ time NOTIFY changed)
    Q_PROPERTY(QString date READ date NOTIFY changed)
    Q_PROPERTY(int batteryPercent READ batteryPercent NOTIFY changed)

public:
    explicit StatusBridge(LanguageBridge *language, QObject *parent = nullptr);

    QString time() const { return time_; }
    QString date() const { return date_; }
    int batteryPercent() const { return battery_; }

signals:
    void changed();

private slots:
    void refresh();
    void applicationStateChanged(Qt::ApplicationState state);

private:
    QLocale locale() const;

    LanguageBridge *language_;
    QTimer timer_;
    QString time_;
    QString date_;
    int battery_ = -1;
};

#endif

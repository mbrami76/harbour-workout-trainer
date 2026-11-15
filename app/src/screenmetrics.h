// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#ifndef SCREENMETRICS_H
#define SCREENMETRICS_H

#include <QGuiApplication>
#include <QObject>
#include <QScreen>

// The physical size of a pixel, so that a control can be given a size in millimetres (R-UI-5).
// Exposed to QML as the context property "screenMetrics".
class ScreenMetrics : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal pixelsPerMm READ pixelsPerMm CONSTANT)
    // True when the graphics stack renders in software, where a canvas is cheaper in an image than
    // in a framebuffer object.
    Q_PROPERTY(bool softwareRendering READ softwareRendering NOTIFY softwareRenderingChanged)

public:
    explicit ScreenMetrics(QObject *parent = nullptr) : QObject(parent) {}

    // Pixels in one millimetre as the platform reports it; 0 when the platform gives no usable
    // value, in which case QML falls back to the sizes of the theme.
    qreal pixelsPerMm() const
    {
        const QScreen *screen = QGuiApplication::primaryScreen();
        const qreal dpi = screen ? screen->physicalDotsPerInch() : 0;
        return (dpi > 50 && dpi < 1000) ? dpi / 25.4 : 0;
    }

    bool softwareRendering() const { return software_; }

public slots:
    void setSoftwareRendering(bool software)
    {
        if (software == software_)
            return;
        software_ = software;
        emit softwareRenderingChanged();
    }

signals:
    void softwareRenderingChanged();

private:
    bool software_ = false;
};

#endif

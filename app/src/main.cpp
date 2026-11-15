// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <sailfishapp.h>

#include <QGuiApplication>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QQmlContext>
#include <QQuickView>
#include <QQuickWindow>
#include <QScopedPointer>
#include <QStandardPaths>

#include "countdownbridge.h"
#include "dashboardbridge.h"
#include "exchangebridge.h"
#include "exerciselibrarybridge.h"
#include "intervalbridge.h"
#include "languagebridge.h"
#include "licencesbridge.h"
#include "metronomebridge.h"
#include "metronomelibrarybridge.h"
#include "screenmetrics.h"
#include "sessionbridge.h"
#include "statusbridge.h"
#include "services.h"
#include "speechbridge.h"
#include "stopwatchbridge.h"
#include "timeattackbridge.h"
#include "timeformatter.h"
#include "workoutbuilderbridge.h"
#include "workoutlibrarybridge.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setApplicationVersion(QStringLiteral(APP_VERSION));
    QScopedPointer<QQuickView> view(SailfishApp::createView());

    // The services exist before the QML engine runs anything that could ask for them.
    Services services(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
                       SailfishApp::pathTo(QStringLiteral("starter")).toLocalFile());
    view->rootContext()->setContextProperty(QStringLiteral("services"), &services);

    // The bridges and helpers of the modes, each next to "services". They take the Services
    // for the clock and the stores they need and must not be used when it is not ready.
    TimeFormatter timeFormatter;
    ScreenMetrics screenMetrics;
    IntervalBridge intervalBridge(&services);
    CountdownBridge countdownBridge(&services);
    StopwatchBridge stopwatchBridge(&services);
    TimeAttackBridge timeAttackBridge(&services);
    MetronomeBridge metronomeBridge(&services);
    MetronomeLibraryBridge metronomeLibrary(&services);
    ExerciseLibraryBridge exerciseLibrary(&services);
    WorkoutLibraryBridge workoutLibrary(&services);
    WorkoutBuilderBridge workoutBuilder(&services);
    SessionBridge sessionBridge(&services);
    ExchangeBridge exchangeBridge(&services);
    DashboardBridge dashboard(&services);
    LanguageBridge languageBridge(&services, view.data(), SailfishApp::pathToMainQml(),
                                  SailfishApp::pathTo(QStringLiteral("translations")).toLocalFile());
    SpeechBridge speechBridge(&languageBridge, SailfishApp::pathTo(QString()).toLocalFile());
    sessionBridge.setSpeech(&speechBridge);
    StatusBridge statusLine(&languageBridge);
    LicencesBridge licences(SailfishApp::pathTo(QString()).toLocalFile());
    view->rootContext()->setContextProperty(QStringLiteral("speech"), &speechBridge);
    view->rootContext()->setContextProperty(QStringLiteral("statusLine"), &statusLine);
    view->rootContext()->setContextProperty(QStringLiteral("licences"), &licences);
    view->rootContext()->setContextProperty(QStringLiteral("timeFormatter"), &timeFormatter);
    view->rootContext()->setContextProperty(QStringLiteral("screenMetrics"), &screenMetrics);
    view->rootContext()->setContextProperty(QStringLiteral("intervalBridge"), &intervalBridge);
    view->rootContext()->setContextProperty(QStringLiteral("countdownBridge"), &countdownBridge);
    view->rootContext()->setContextProperty(QStringLiteral("stopwatchBridge"), &stopwatchBridge);
    view->rootContext()->setContextProperty(QStringLiteral("timeAttackBridge"), &timeAttackBridge);

    view->rootContext()->setContextProperty(QStringLiteral("metronomeBridge"), &metronomeBridge);
    view->rootContext()->setContextProperty(QStringLiteral("metronomeLibrary"), &metronomeLibrary);
    view->rootContext()->setContextProperty(QStringLiteral("exerciseLibrary"), &exerciseLibrary);
    view->rootContext()->setContextProperty(QStringLiteral("workoutLibrary"), &workoutLibrary);
    view->rootContext()->setContextProperty(QStringLiteral("workoutBuilder"), &workoutBuilder);
    view->rootContext()->setContextProperty(QStringLiteral("sessionBridge"), &sessionBridge);
    view->rootContext()->setContextProperty(QStringLiteral("exchangeBridge"), &exchangeBridge);
    view->rootContext()->setContextProperty(QStringLiteral("dashboard"), &dashboard);
    view->rootContext()->setContextProperty(QStringLiteral("languageBridge"), &languageBridge);

    // The graphics stack is known once the scene graph exists. A software renderer is told by its
    // name; the signal comes on the render thread with the context current.
    QObject::connect(view.data(), &QQuickWindow::sceneGraphInitialized, &screenMetrics, [&screenMetrics]() {
        QOpenGLContext *context = QOpenGLContext::currentContext();
        if (!context)
            return;
        const QByteArray renderer(reinterpret_cast<const char *>(context->functions()->glGetString(GL_RENDERER)));
        const bool software = renderer.contains("llvmpipe") || renderer.contains("softpipe")
                              || renderer.contains("Software") || renderer.contains("swrast");
        QMetaObject::invokeMethod(&screenMetrics, "setSoftwareRendering", Qt::QueuedConnection,
                                  Q_ARG(bool, software));
    }, Qt::DirectConnection);

    view->setSource(SailfishApp::pathToMainQml());
    view->show();
    // The starter data is offered by the language bridge once the language is known, after the first
    // page is on its way.
    return app->exec();
}

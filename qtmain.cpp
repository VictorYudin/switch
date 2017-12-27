#include "window.h"

#include <QGuiApplication>

int main(int argc, char **argv)
{
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);

    // Request OpenGL 3.3 compatibility or OpenGL ES 3.0.
    if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL)
    {
        qDebug("Requesting 3.3 compatibility context");
        fmt.setVersion(3, 3);
        fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    }
    else
    {
        qDebug("Requesting 3.0 context");
        fmt.setVersion(3, 0);
    }

    QSurfaceFormat::setDefaultFormat(fmt);

    QGuiApplication app(argc, argv);

    OpenGLWindow glWindow;
    glWindow.showMaximized();

    return app.exec();
}

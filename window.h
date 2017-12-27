#ifndef __WINDOWS_H_
#define __WINDOWS_H_

#include "model.h"

#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWindow>
#include <QSharedPointer>
#include <QTime>

class OpenGLWindow : public QOpenGLWindow
{
    Q_OBJECT

public:
    OpenGLWindow();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mouseReleaseEvent(QMouseEvent* ev) override;

private:
    int loadModel(
        const char* iFileName,
        QSharedPointer<QOpenGLVertexArrayObject> oVAO,
        QSharedPointer<QOpenGLBuffer> oSwitchAngles) const;

    int getObjectID(int x, int y) const;

    QSharedPointer<QOpenGLShaderProgram> mProgram;
    QSharedPointer<QOpenGLVertexArrayObject> mSwitchVAO;
    QSharedPointer<QOpenGLBuffer> mSwitchAnglesBuffer;
    int mSwitchNPoints;

    float mSwitchAngles[16];
    float mSwitchAnglesAspire[16];
    bool mWin;

    QMatrix4x4 mProj;

    int mMVPLoc;
    int mLightPosLoc;

    QTime mTime;

    QSharedPointer<QOpenGLFramebufferObject> mFrameBuffer;
};

#endif

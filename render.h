#ifndef __RENDER_H_
#define __RENDER_H_

#include <QOpenGLBuffer>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWindow>
#include <QSharedPointer>
#include <QTime>
#include <QtQuick/QQuickFramebufferObject>
#include "object.h"
#include "hdrloader.h"

class SwitchRender : public QQuickFramebufferObject::Renderer
{
public:
    SwitchRender();
    ~SwitchRender();

    void init();

    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size)
        Q_DECL_OVERRIDE;

    void render() Q_DECL_OVERRIDE;

protected:
    void synchronize(QQuickFramebufferObject* item) Q_DECL_OVERRIDE;

private:
    int getObjectID(int x, int y);

    // The current angle of the switches in the animation.
    std::vector<float> mSwitchAngles;
    // The state of the switches.
    std::vector<int> mSwitchAnglesAspire;
    bool mWin;

    QMatrix4x4 mProj;

    QTime mTime;

    int mSize;

    Object mSwitches;
    Object mBoard;
    HDRLoaderResult mHDRI;
    GLuint mEnvironment;
};

class Switch : public QQuickFramebufferObject
{
    Q_OBJECT

public:
    Switch(QQuickItem* parent = Q_NULLPTR);
    Renderer* createRenderer() const;

protected:
    virtual void mousePressEvent(QMouseEvent* ev) Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent* ev) Q_DECL_OVERRIDE;

signals:
    void winGame();

public slots:
    void newGame();

private:
    friend class SwitchRender;

    int mLastClickX;
    int mLastClickY;
    bool mNewGamePressed;
};

#endif

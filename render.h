#ifndef __RENDER_H_
#define __RENDER_H_

#include "hdrloader.h"
#include "object.h"
#include <QOpenGLBuffer>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWindow>
#include <QSharedPointer>
#include <QTime>
#include <QtQuick/QQuickFramebufferObject>

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

    qint64 mElapsed[10];
    size_t mCurrentElapsed;

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

    // Elapsed time to draw one frime
    Q_PROPERTY(int elapsed READ elapsed WRITE setElapsed NOTIFY elapsedChanged)

public:
    Switch(QQuickItem* parent = Q_NULLPTR);
    Renderer* createRenderer() const;

    void setElapsed(float elapsed)
    {
        if (elapsed != mElapsed)
        {
            mElapsed = elapsed;
            emit elapsedChanged();
        }
    }
    float elapsed() const { return mElapsed; }

protected:
    virtual void mousePressEvent(QMouseEvent* ev) Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent* ev) Q_DECL_OVERRIDE;

signals:
    void winGame();
    void elapsedChanged();

public slots:
    void newGame();

private:
    friend class SwitchRender;

    int mLastClickX;
    int mLastClickY;
    bool mNewGamePressed;

    float mElapsed;
};

#endif

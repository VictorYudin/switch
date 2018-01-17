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

    // signals:
    //     void winGame();

private:
    void initialize();

    int loadModel(
        const char* iFileName,
        QSharedPointer<QOpenGLVertexArrayObject> oVAO,
        QSharedPointer<QOpenGLBuffer> oSwitchAngles);

    int getObjectID(int x, int y);

    QSharedPointer<QOpenGLShaderProgram> mProgram;
    QSharedPointer<QOpenGLVertexArrayObject> mSwitchVAO;
    QSharedPointer<QOpenGLBuffer> mSwitchAnglesBuffer;
    int mSwitchNPoints;

    float mSwitchAngles[16];
    int mSwitchAnglesAspire[16];
    bool mWin;

    QMatrix4x4 mProj;

    int mMVPLoc;
    int mCamLoc;
    int mLightPosLoc;

    QTime mTime;

    int mSize;
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

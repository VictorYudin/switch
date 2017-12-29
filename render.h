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
#include <QtQuick/QQuickItem>

class SwitchRender : public QObject, protected QOpenGLExtraFunctions
{
    Q_OBJECT
public:
    SwitchRender();
    ~SwitchRender();

    void init();

    void setViewportSize(const QSize& size);
    void setWindow(QQuickWindow* window) { m_window = window; }
    void mouseReleaseEvent(QMouseEvent* ev);

signals:
    void winGame();

public slots:
    void paint();

private:
    QQuickWindow* m_window;

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
    float mSwitchAnglesAspire[16];
    bool mWin;

    QMatrix4x4 mProj;

    int mMVPLoc;
    int mLightPosLoc;

    QTime mTime;

    QSharedPointer<QOpenGLFramebufferObject> mFrameBuffer;

    int mSize;
};

class Switch : public QQuickItem
{
    Q_OBJECT

public:
    Switch(QQuickItem* parent = Q_NULLPTR);

protected:
    virtual void mousePressEvent(QMouseEvent* ev) override;
    virtual void mouseReleaseEvent(QMouseEvent* ev) override;

signals:
    void winGame();

public slots:
    void sync();
    void cleanup();
    void newGame();

private slots:
    void handleWindowChanged(QQuickWindow* win);

private:
    QSharedPointer<SwitchRender> mRender;
};

#endif

// Copyright 2018 Victor Yudin. All rights reserved.

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

/** @brief The main OpenGL object. OpenGL starts here. */
class SwitchRender : public QQuickFramebufferObject::Renderer
{
public:
    SwitchRender();
    ~SwitchRender();

    /** @brief Fills mSwitchAngles with random data. Is used to start a new
     * game. */
    void init();

    /**
     * @brief Is called when a new QQuickFramebufferObject is needed. This
     * happens on the initial frame. It is called again every time the
     * dimensions of the item changes.
     *
     * @param size The new dimension.
     *
     * @return Create framebuffer object.
     */
    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size)
        Q_DECL_OVERRIDE;

    /** @brief This function is called when this object should be rendered. */
    void render() Q_DECL_OVERRIDE;

protected:
    /**
     * @brief This function is called as a result of
     * QQuickFramebufferObject::update().
     *
     * @param item The parent item.
     */
    void synchronize(QQuickFramebufferObject* item) Q_DECL_OVERRIDE;

private:
    /**
     * @brief Read the object ID buffer and get the object id under the specific
     * coordinate.
     *
     * @param x screen coordinate
     * @param y screen coordinate
     *
     * @return The ID of the object.
     */
    int getObjectID(int x, int y);

    /**
     * @brief Initializes multiple framebuffers for the prepass. We need this
     * because QOpenGLFramebufferObject doesn't support float attachments.
     * According to the docs it should support but see the source of
     * QOpenGLFramebufferObjectPrivate::initTexture for details.
     *
     * @param width Viewport width.
     * @param height Viewport height.
     */
    void initPrepassFramebufferObject(int iWidth, int iHeight);

    /**
     * @brief Add a float color textur buffer to the current bound framebuffer.
     *
     * @param width Viewport width.
     * @param height Viewport height.
     *
     * @return OpenGL ID of the created texture.
     */
    GLuint addFloatColorAttachment(int iWidth, int iHeight);

    /** @brief Initializes the OpenGL buffers to draw a quad that covers all the
     * viewport. */
    void initPlane();

    /**
     * @brief Render the first pass that bakes the data.
     *
     * @param iCameraLocation The position of the camera.
     * @param iMVP The Movel View Projection matrix.
     */
    void renderBakePass(
        const QVector3D& iCameraLocation,
        const QMatrix4x4& iMVP) const;

    /**
     * @brief Render the second pass that generates the beauty image.
     *
     * @param iCameraLocation The position of the camera.
     * @param iLightLocation The position of the light.
     * @param iMVP The Movel View Projection matrix.
     */
    void renderBeautyPass(
        const QVector3D& iCameraLocation,
        const QVector3D& iLightLocation,
        const QMatrix4x4& iMVP) const;

    // Timer stuff. We keep 10 last times to compute average time.
    qint64 mElapsed[10];
    size_t mCurrentElapsed;

    // The current angles of the switches in the animation.
    std::vector<float> mSwitchAngles;
    // The state of the switches.
    std::vector<int> mSwitchAnglesAspire;
    // The flag that keeps the info if the user already won the game.
    bool mWin;

    // The camera projection matrix.
    QMatrix4x4 mProj;

    // The main timer for the animation.
    QTime mTime;

    // The number of rows and columns in the game. Usually it's 4.
    int mSize;

    // The object that represents all the switches.
    Object mSwitches;
    // The object that represents the board.
    Object mBoard;
    // HDRI map.
    HDRLoaderResult mHDRI;
    // OpenGL texture ID with the environment.
    GLuint mEnvironment;

    // Framebuffer and AOVs of the first pass.
    GLuint mPrepassFBO;
    GLuint mPrepassTexID;
    GLuint mPrepassTexP;
    GLuint mPrepassTexN;
    GLuint mPrepassTexC;
    GLuint mPrepassDepth;

    // The shader program and uniform locations of the first pass, that mixes
    // all the AOVs together.
    QSharedPointer<QOpenGLShaderProgram> mBeautyProgram;
    int mShaderLightPos;
    int mShaderEPos;
    int mShaderPPos;
    int mShaderNPos;
    int mShaderCPos;
    int mShaderEnvPos;
    int mShaderMVPPos;

    // OpenGL gometry buffers.
    GLuint mVertexArray;
    GLuint mPointBuffer;
    GLuint mUVBuffer;
};

class Switch : public QQuickFramebufferObject
{
    Q_OBJECT

    // Elapsed time to draw one frime
    Q_PROPERTY(int elapsed READ elapsed WRITE setElapsed NOTIFY elapsedChanged)

public:
    Switch(QQuickItem* parent = Q_NULLPTR);
    Renderer* createRenderer() const;

    /**
     * @brief Methods to set/get the property that contains the time elapsed to
     * draw one frame.
     */
    void setElapsed(float elapsed);
    float elapsed() const { return mElapsed; }

protected:
    virtual void mousePressEvent(QMouseEvent* ev) Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent* ev) Q_DECL_OVERRIDE;

signals:
    /** @brief Called when the user wins the game. */
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

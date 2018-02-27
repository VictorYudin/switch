
#include "render.h"

#include "model.h"

#include <GLES3/gl3.h>
#include <QElapsedTimer>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QtQuick/qquickwindow.h>
#include <assert.h>

// Qt doesn't have it
#define GL_COLOR 0x1800

SwitchRender::SwitchRender() :
        mSize(4),
        mElapsed{},
        mCurrentElapsed(0),
        mPrepassFBO(0)
{
    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    srand(time(NULL));

    // The current angles of the switches in the animation.
    mSwitchAngles.resize(mSize * mSize);
    // The state of the switches.
    mSwitchAnglesAspire.resize(mSize * mSize);

    // Fill the states with random data.
    init();

    // Load HDR environment map as a texture.
    HDRLoader::load(":/environment.hdr", mHDRI);

    // Init environment texture.
    f->glGenTextures(1, &mEnvironment);
    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, mEnvironment);

    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    f->glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB32F,
        mHDRI.width,
        mHDRI.height,
        0,
        GL_RGB,
        GL_FLOAT,
        mHDRI.cols);

    // Init the OpenGL buffers to draw one polygon that covers the screen and is
    // used to draw the second pass.
    initPlane();

    // Initialize the shader program that mixes all the AOVs togeter on the
    // second pass.
    QFile vertexShader(":/beauty.vert");
    QFile fragmantShader(":/beauty.frag");
    if (!vertexShader.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug("Can't read vertex shader");
    }
    if (!fragmantShader.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug("Can't read fragment shader");
    }

    mBeautyProgram.reset(new QOpenGLShaderProgram);
    mBeautyProgram->addShaderFromSourceCode(
        QOpenGLShader::Vertex, versionShaderCode(vertexShader.readAll()));
    mBeautyProgram->addShaderFromSourceCode(
        QOpenGLShader::Fragment, versionShaderCode(fragmantShader.readAll()));
    mBeautyProgram->link();

    // Get locations of the unform data of the initialized shader.
    mShaderLightPos = mBeautyProgram->uniformLocation("gLightPos");
    mShaderEPos = mBeautyProgram->uniformLocation("gE");
    mShaderPPos = mBeautyProgram->uniformLocation("gSamplerP");
    mShaderNPos = mBeautyProgram->uniformLocation("gSamplerN");
    mShaderCPos = mBeautyProgram->uniformLocation("gSamplerC");
    mShaderEnvPos = mBeautyProgram->uniformLocation("gSamplerEnv");
    mShaderMVPPos = mBeautyProgram->uniformLocation("gMVP");
}

SwitchRender::~SwitchRender()
{
    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    f->glDeleteTextures(1, &mEnvironment);
    f->glDeleteVertexArrays(1, &mVertexArray);
    f->glDeleteBuffers(1, &mPointBuffer);
    f->glDeleteBuffers(1, &mUVBuffer);

    if (mPrepassFBO)
    {
        f->glDeleteTextures(1, &mPrepassTexID);
        f->glDeleteTextures(1, &mPrepassTexP);
        f->glDeleteTextures(1, &mPrepassTexN);
        f->glDeleteTextures(1, &mPrepassTexC);
        f->glDeleteTextures(1, &mPrepassDepth);
        f->glDeleteFramebuffers(1, &mPrepassFBO);
    }
}

void SwitchRender::init()
{
    mWin = false;

    for (int i = 0; i < mSize * mSize; i++)
    {
        int v = rand() % 2;
        mSwitchAngles[i] = 0.0f;
        mSwitchAnglesAspire[i] = v;
    }

    mTime.restart();
}

void SwitchRender::initPrepassFramebufferObject(int iWidth, int iHeight)
{
    // TODO: Don't call it in any function.
    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    f->glGenFramebuffers(1, &mPrepassFBO);
    f->glBindFramebuffer(GL_FRAMEBUFFER, mPrepassFBO);

    mPrepassTexID = addFloatColorAttachment(iWidth, iHeight);
    mPrepassTexP = addFloatColorAttachment(iWidth, iHeight);
    mPrepassTexN = addFloatColorAttachment(iWidth, iHeight);
    mPrepassTexC = addFloatColorAttachment(iWidth, iHeight);

    // depth texture instead of a renderbuffer.
    f->glGenTextures(1, &mPrepassDepth);
    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, mPrepassDepth);
    f->glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT32F,
        iWidth,
        iHeight,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        NULL);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach depth texture to framebuffer
    f->glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mPrepassDepth, 0);

    // Attach textures to the framebuffer.
    f->glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mPrepassTexP, 0);
    f->glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, mPrepassTexN, 0);
    f->glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, mPrepassTexID, 0);
    f->glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, mPrepassTexC, 0);

    // We don't use location 0 because if when we use it, it goes to the beauty
    // pass when swapping the beauty buffers.
    GLenum drawBufs[] = {GL_NONE,
                         GL_COLOR_ATTACHMENT1,
                         GL_COLOR_ATTACHMENT2,
                         GL_COLOR_ATTACHMENT3,
                         GL_COLOR_ATTACHMENT4};
    f->glDrawBuffers(sizeof(drawBufs) / sizeof(drawBufs[0]), drawBufs);

    // Validate mPrepassFBO and return false on error.
    GLenum status = f->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (GL_FRAMEBUFFER_COMPLETE != status)
    {
        qDebug("ERROR: incomplete framebuffer");
    }
}

GLuint SwitchRender::addFloatColorAttachment(int iWidth, int iHeight)
{
    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    GLuint textureID;

    // Generate output texture.
    f->glGenTextures(1, &textureID);
    f->glBindTexture(GL_TEXTURE_2D, textureID);
    f->glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA32F,
        iWidth,
        iHeight,
        0,
        GL_RGBA,
        GL_FLOAT,
        NULL);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return textureID;
}

void SwitchRender::initPlane()
{
    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    // Geometry to use. These are 4 xyz points to make a quad.
    static const GLfloat points[] = {
        -1.f, -1.f, 0.f, 1.f, -1.f, 0.f, 1.f, 1.f, 0.f, -1., 1., 0.};
    // These are 4 UVs.
    static const GLfloat uvs[] = {0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f};

    // Create a vertex buffer object. It stores an array of data on the graphics
    // adapter's memory. The vertex points in our case.
    f->glGenBuffers(1, &mPointBuffer);
    f->glBindBuffer(GL_ARRAY_BUFFER, mPointBuffer);
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

    // The same for UVs.
    f->glGenBuffers(1, &mUVBuffer);
    f->glBindBuffer(GL_ARRAY_BUFFER, mUVBuffer);
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);

    // The vertex array object is a descriptor that defines which data from
    // vertex buffer objects should be used as input variables to vertex
    // shaders.
    f->glGenVertexArrays(1, &mVertexArray);
    f->glBindVertexArray(mVertexArray);
}

void SwitchRender::renderBakePass(
    const QVector3D& iCameraLocation,
    const QMatrix4x4& iMVP) const
{
    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    f->glEnable(GL_DEPTH_TEST);
    f->glFrontFace(GL_CCW);
    f->glCullFace(GL_BACK);
    f->glDisable(GL_BLEND);
    f->glDepthMask(GL_TRUE);

    // Clear AOVs.
    static const float sBlack[] = {0.0f, 0.0f, 0.0f, 0.0f};
    f->glClearBufferfv(GL_COLOR, 1, sBlack);
    f->glClearBufferfv(GL_COLOR, 2, sBlack);
    f->glClearBufferfv(GL_COLOR, 3, sBlack);
    f->glClearBufferfv(GL_COLOR, 4, sBlack);
    // Clear the depth buffer.
    f->glClear(GL_DEPTH_BUFFER_BIT);

    // Setup texture.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mEnvironment);

    // Render the geometry.
    mSwitches.render(iMVP, mSwitchAngles.data());
    mBoard.render(iMVP, nullptr);
}

void SwitchRender::renderBeautyPass(
    const QVector3D& iCameraLocation,
    const QVector3D& iLightLocation,
    const QMatrix4x4& iMVP) const
{
    assert(mVertexArray);
    assert(mPointBuffer);
    assert(mUVBuffer);
    assert(mBeautyProgram);

    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    f->glDisable(GL_BLEND);
    f->glDisable(GL_DEPTH_TEST);
    f->glDepthMask(GL_FALSE);

    mBeautyProgram->bind();

    // Setup uniform data.
    mBeautyProgram->setUniformValue(mShaderNPos, 0);
    mBeautyProgram->setUniformValue(mShaderPPos, 1);
    mBeautyProgram->setUniformValue(mShaderEnvPos, 2);
    mBeautyProgram->setUniformValue(mShaderCPos, 3);
    // Setup camera position.
    mBeautyProgram->setUniformValue(mShaderEPos, iCameraLocation);
    mBeautyProgram->setUniformValue(mShaderMVPPos, iMVP);
    // Setup light position.
    mBeautyProgram->setUniformValue(mShaderLightPos, iLightLocation);

    f->glBindVertexArray(mVertexArray);

    f->glBindBuffer(GL_ARRAY_BUFFER, mPointBuffer);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    f->glBindBuffer(GL_ARRAY_BUFFER, mUVBuffer);
    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    f->glEnableVertexAttribArray(0);
    f->glEnableVertexAttribArray(1);

    // Get color and depts buffers as texture. It's important to have this
    // order. Otherwise it's blinking.
    f->glActiveTexture(GL_TEXTURE3);
    f->glBindTexture(GL_TEXTURE_2D, mPrepassTexC);
    f->glActiveTexture(GL_TEXTURE2);
    f->glBindTexture(GL_TEXTURE_2D, mEnvironment);
    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, mPrepassTexP);
    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, mPrepassTexN);

    // Draw points 0-4 from the currently bound VAO with current in-use shader.
    f->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

QOpenGLFramebufferObject* SwitchRender::createFramebufferObject(
    const QSize& size)
{
    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    // Create the framebuffer for prepass.
    initPrepassFramebufferObject(size.width(), size.height());

    // Create the beauty framebuffer.
    QOpenGLFramebufferObject* frameBuffer = new QOpenGLFramebufferObject(
        size, QOpenGLFramebufferObject::CombinedDepthStencil);
    static GLenum drawBufs[] = {GL_COLOR_ATTACHMENT0};
    f->glDrawBuffers(sizeof(drawBufs) / sizeof(drawBufs[0]), drawBufs);

    mProj.setToIdentity();
    mProj.perspective(
        45.0f, GLfloat(size.width()) / size.height(), 10.0f, 1000.0f);

    return frameBuffer;
}

void SwitchRender::render()
{
    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    // Bake prepass
    GLuint prevFbo = 0;
    f->glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&prevFbo);
    f->glBindFramebuffer(GL_FRAMEBUFFER, mPrepassFBO);

    static QVector3D sCameraLocation(0.0f, 500.0f, 250.0f);
    static QVector3D sCameraLookAt(0.0f, 0.0f, 30.0f);
    static QVector3D sUp(0.0f, 1.0f, 0.0f);
    static QVector3D sLightLocation(-300.0f, 300.0f, 0.0f);

    if (!mSwitches.valid())
    {
        mSwitches.init(":/switch.usda", 1, mSize);
    }

    if (!mBoard.valid())
    {
        mBoard.init(":/board.usda", 2, 1);
        mTime.start();
    }

    QElapsedTimer timer;
    timer.start();

    float delta = float(mTime.restart()) * 0.003f;
    bool needUpdate = false;

    for (int i = 0; i < mSize * mSize; i++)
    {
        if (mSwitchAnglesAspire[i] > mSwitchAngles[i])
        {
            float animated = mSwitchAngles[i] + delta;
            mSwitchAngles[i] =
                std::min<float>(animated, mSwitchAnglesAspire[i]);
            needUpdate = true;
        }
    }

    // Setup camera.
    QMatrix4x4 camera;
    camera.setToIdentity();
    camera.lookAt(sCameraLocation, sCameraLookAt, sUp);

    QMatrix4x4 mvp = mProj * camera;

    // Prepass. It will render point positions, normals, IDs to the AOVs.
    renderBakePass(sCameraLocation, mvp);

    // Beauty pass
    f->glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
    renderBeautyPass(sCameraLocation, sLightLocation, mvp);

    if (needUpdate)
    {
        // We need to call this function once again.
        update();
    }

    mElapsed[mCurrentElapsed] = timer.elapsed();
    mCurrentElapsed = (mCurrentElapsed + 1) % 10;
}

void SwitchRender::synchronize(QQuickFramebufferObject* item)
{
    reinterpret_cast<Switch*>(item)->setElapsed(
        std::accumulate(mElapsed, mElapsed + 10, 0) / 10);

    if (mWin)
    {
        // The user already won. Just lock the game.
        return;
    }

    Switch* sw = reinterpret_cast<Switch*>(item);

    // Check if the user clicked.
    if (sw->mLastClickX > 0 && sw->mLastClickY > 0)
    {
        // Check if the user clicked by one of the switches.
        int ID = getObjectID(sw->mLastClickX, sw->mLastClickY);

        sw->mLastClickX = -1;
        sw->mLastClickY = -1;

        if (ID >= 0)
        {
            int x = ID % mSize;
            int y = ID / mSize;

            int wrongPlaced = 0;

            // Move the switches to the new position.
            for (int i = 0; i < mSize; i++)
            {
                for (int j = 0; j < mSize; j++)
                {
                    int& current = mSwitchAnglesAspire[j * mSize + i];

                    if (i == x || j == y)
                    {
                        current += 1;
                    }

                    wrongPlaced += 1 - current % 2;
                }
            }

            // If everything placed good, then the user wins.
            if (wrongPlaced == 0)
            {
                emit sw->winGame();
            }
        }

        mTime.restart();
    }

    if (sw->mNewGamePressed)
    {
        sw->mNewGamePressed = false;
        init();
    }
}

int SwitchRender::getObjectID(int x, int y)
{
    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    GLuint prevFbo = 0;
    f->glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&prevFbo);
    f->glBindFramebuffer(GL_FRAMEBUFFER, mPrepassFBO);

    f->glReadBuffer(GL_COLOR_ATTACHMENT3);

    float d[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    f->glReadPixels(
        x,
        framebufferObject()->size().height() - y,
        1,
        1,
        GL_RGBA,
        GL_FLOAT,
        &d);

    f->glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);

    if (roundf(d[0] * 10) == 1)
    {
        return static_cast<int>(roundf(d[1] * 10) + roundf(d[2] * 10) * 10);
    }

    return -1;
}

Switch::Switch(QQuickItem* parent) :
        QQuickFramebufferObject(parent),
        mLastClickX(-1),
        mLastClickY(-1),
        mNewGamePressed(false)
{
    // This call is crucial to even get any clicks at all
    setAcceptedMouseButtons(Qt::LeftButton);
}

QQuickFramebufferObject::Renderer* Switch::createRenderer() const
{
#if QT_CONFIG(opengl)
    qDebug("Create renderer");
    return new SwitchRender;
#else
    qDebug("No OpenGL, no renderer");
    return nullptr;
#endif
}

void Switch::setElapsed(float elapsed)
{
    if (elapsed != mElapsed)
    {
        mElapsed = elapsed;
        emit elapsedChanged();
    }
}

void Switch::mousePressEvent(QMouseEvent* ev)
{
    ev->accept();
}

void Switch::mouseReleaseEvent(QMouseEvent* ev)
{
    mLastClickX = ev->x();
    mLastClickY = ev->y();
    ev->accept();

    update();
}

void Switch::newGame()
{
    mNewGamePressed = true;

    update();
}

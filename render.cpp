
#include "render.h"

#include "model.h"

#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QtQuick/qquickwindow.h>
#include <GLES3/gl3.h >

#define GL_COLOR 0x1800

SwitchRender::SwitchRender() : mSize(4)
{
    srand(time(NULL));

    mSwitchAngles.resize(mSize * mSize);
    mSwitchAnglesAspire.resize(mSize * mSize);

    init();

    // Load HDR environment map as a texture.
    HDRLoader::load(":/environment.hdr", mHDRI);

    glGenTextures(1, &mEnvironment);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mEnvironment);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB32F,
        mHDRI.width,
        mHDRI.height,
        0,
        GL_RGB,
        GL_FLOAT,
        mHDRI.cols);
}

SwitchRender::~SwitchRender()
{}

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

QOpenGLFramebufferObject* SwitchRender::createFramebufferObject(
    const QSize& size)
{
    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    QOpenGLFramebufferObject* frameBuffer = new QOpenGLFramebufferObject(
        size.width(),
        size.height(),
        QOpenGLFramebufferObject::CombinedDepthStencil);
    frameBuffer->addColorAttachment(size.width(), size.height());

    static GLenum drawBufs[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};

    f->glDrawBuffers(2, drawBufs);

    mProj.setToIdentity();
    mProj.perspective(
        45.0f, GLfloat(size.width()) / size.height(), 10.0f, 1000.0f);

    return frameBuffer;
}

void SwitchRender::render()
{
    static QVector3D sCameraLocation(0.0f, 500.0f, 250.0f);
    static QVector3D sCameraLookAt(0.0f, 0.0f, 30.0f);
    static QVector3D sUp(0.0f, 1.0f, 0.0f);
    static QVector3D sLightLocation(-300.0f, 300.0f, 0.0f);

    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    if (!mSwitches.valid())
    {
        mSwitches.init(":/switch.usda", 1, mSize);
    }

    if (!mBoard.valid())
    {
        mBoard.init(":/board.usda", 2, 1);
        mTime.start();
    }

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

    f->glEnable(GL_DEPTH_TEST);
    f->glEnable(GL_CULL_FACE);
    f->glDepthMask(GL_TRUE);
    f->glDepthFunc(GL_LESS);
    f->glFrontFace(GL_CCW);
    f->glCullFace(GL_BACK);

    // Clear the first AOV.
    static const float background[] = {0.322f, 0.38f, 0.424f, 1.0f};
    f->glClearBufferfv(GL_COLOR, 0, background);
    // Clear the second AOV.
    static const float black[] = {0.0f, 0.0f, 0.0f, 0.0f};
    f->glClearBufferfv(GL_COLOR, 1, black);
    // Clear the depth buffer.
    f->glClear(GL_DEPTH_BUFFER_BIT);

    // Setup camera.
    QMatrix4x4 camera;
    camera.setToIdentity();
    camera.lookAt(sCameraLocation, sCameraLookAt, sUp);

    // Setup texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mEnvironment);

    mSwitches.render(
        mProj * camera, sCameraLocation, sLightLocation, mSwitchAngles.data());

    mBoard.render(mProj * camera, sCameraLocation, sLightLocation, nullptr);

    if (needUpdate)
    {
        // We need to call this function once again.
        update();
    }
}

void SwitchRender::synchronize(QQuickFramebufferObject* item)
{
    if (mWin)
    {
        return;
    }

    Switch* sw = reinterpret_cast<Switch*>(item);

    if (sw->mLastClickX > 0 && sw->mLastClickY > 0)
    {
        int ID = getObjectID(sw->mLastClickX, sw->mLastClickY);

        sw->mLastClickX = -1;
        sw->mLastClickY = -1;

        if (ID >= 0)
        {
            int x = ID % mSize;
            int y = ID / mSize;

            int wrongPlaced = 0;

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

    framebufferObject()->bind();

    f->glReadBuffer(GL_COLOR_ATTACHMENT1);

    float d[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    f->glReadPixels(x, y, 1, 1, GL_RGBA, GL_FLOAT, &d);

    framebufferObject()->release();

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

#include "window.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <QMouseEvent>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>

#define GL_COLOR 0x1800
#define GL_DEPTH 0x1801
#define GL_CLAMP_READ_COLOR 0x891C
#define GL_CLAMP_VERTEX_COLOR 0x891A
#define GL_CLAMP_FRAGMENT_COLOR 0x891B
#define GL_COLOR_ATTACHMENT1 (GL_COLOR_ATTACHMENT0 + 1)

static const char* vertexShaderSource =
    "layout(location = 0) in vec3 vertex;\n"
    "layout(location = 1) in vec3 normal;\n"
    "layout(location = 2) in float angle;\n"
    "uniform highp mat4 mvp;\n"
    "out vec3 vert;\n"
    "out vec3 vertNormal;\n"
    "out vec3 color;\n"
    "out vec3 vertObjectID;\n"
    "mat4 rotationMatrix(float a)\n"
    "{\n"
    "    float s = sin(3.1415926 * a / 2.0);\n"
    "    float c = cos(3.1415926 * a / 2.0);\n"
    "    return mat4(  c, 0.0,   s, 0.0,\n"
    "                0.0, 1.0, 0.0, 0.0,\n"
    "                - s, 0.0,   c, 0.0,\n"
    "                0.0, 0.0, 0.0, 1.0);\n"
    "}\n"
    "void main() {\n"
    "   ivec2 index = ivec2(gl_InstanceID % 4, gl_InstanceID / 4);\n"
    "   vec2 offset = vec2("
    "      (-1.5 + float(index.x)) * 105.0, "
    "      (-1.5 + float(index.y)) * 105.0);\n"
    "   mat4 world = mat4("
    "      1.0, 0.0, 0.0, 0.0,"
    "      0.0, 1.0, 0.0, 0.0,"
    "      0.0, 0.0, 1.0, 0.0,"
    "      offset.x, 0.0f, offset.y, 1.0) * rotationMatrix(angle);\n"
    "   color = vec3(0.4, 1.0, 0.0);\n"
    "   vert = vec3(world * vec4(vertex, 1.0f));\n"
    "   vertNormal = mat3(world) * normal;\n"
    "   float floatID = float(gl_InstanceID + 1);\n"
    "   vertObjectID = vec3("
    "      mod(floatID, 10.0) * 0.1,"
    "      floor(floatID / 10.0) * 0.1,"
    "      0.0f);\n"
    "   gl_Position = mvp * world * vec4(vertex, 1.0f);\n"
    "}\n";

static const char* fragmentShaderSource =
    "uniform highp vec3 lightPos;\n"
    "in highp vec3 vert;\n"
    "in highp vec3 vertNormal;\n"
    "in highp vec3 color;\n"
    "in highp vec3 vertObjectID;\n"
    "layout(location = 0) out highp vec4 fragColor;\n"
    "layout(location = 1) out highp vec4 objectID;\n"
    "void main() {\n"
    "   highp vec3 L = lightPos - vert;\n"
    "   highp float NL = max(dot(normalize(vertNormal), normalize(L)), 0.0);\n"
    "   fragColor = vec4(color, 1.0) * NL;\n"
    "   objectID = vec4(vertObjectID, 1.0f);\n"
    "}\n";

OpenGLWindow::OpenGLWindow() : mWin(false)
{
    srand(time(NULL));

    for (int i = 0; i < 16; i++)
    {
        int v = rand() % 2;
        mSwitchAngles[i] = v;
        mSwitchAnglesAspire[i] = v;
    }
}

QByteArray versionedShaderCode(const char* src)
{
    QByteArray versionedSrc;

    if (QOpenGLContext::currentContext()->isOpenGLES())
        versionedSrc.append(QByteArrayLiteral("#version 300 es\n"));
    else
        versionedSrc.append(QByteArrayLiteral("#version 330\n"));

    versionedSrc.append(src);
    return versionedSrc;
}

void OpenGLWindow::initializeGL()
{
    qDebug("initializeGL");

    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    mProgram.reset(new QOpenGLShaderProgram);
    mProgram->addShaderFromSourceCode(
        QOpenGLShader::Vertex, versionedShaderCode(vertexShaderSource));
    mProgram->addShaderFromSourceCode(
        QOpenGLShader::Fragment, versionedShaderCode(fragmentShaderSource));
    mProgram->link();

    mMVPLoc = mProgram->uniformLocation("mvp");
    mLightPosLoc = mProgram->uniformLocation("lightPos");

    mSwitchVAO.reset(new QOpenGLVertexArrayObject());
    mSwitchAnglesBuffer.reset(new QOpenGLBuffer());
    mSwitchNPoints = loadModel("switch.usda", mSwitchVAO, mSwitchAnglesBuffer);

    f->glEnable(GL_DEPTH_TEST);
    f->glEnable(GL_CULL_FACE);

    mTime.start();
}

void OpenGLWindow::paintGL()
{
    float delta = float(mTime.restart()) * 0.005f;
    bool needUpdate = false;

    for (int i = 0; i < 16; i++)
    {
        if (mSwitchAnglesAspire[i] > mSwitchAngles[i])
        {
            float animated = mSwitchAngles[i] + delta;
            mSwitchAngles[i] = std::min(animated, mSwitchAnglesAspire[i]);
            needUpdate = true;
        }
    }

    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    mFrameBuffer->bind();

    static const float background[] = {0.1f, 0.2f, 0.3f, 0.0f};
    f->glClearBufferfv(GL_COLOR, 0, background);
    static const float black[] = {0.0f, 0.0f, 0.0f, 0.0f};
    f->glClearBufferfv(GL_COLOR, 1, black);
    f->glClear(GL_DEPTH_BUFFER_BIT);

    mProgram->bind();

    QMatrix4x4 camera;
    camera.setToIdentity();
    camera.lookAt(
        QVector3D(0.0f, 500.0f, 300.0f),
        QVector3D(0.0f, 0.0f, 0.0f),
        QVector3D(0.0f, 1.0f, 0.0f));
    mProgram->setUniformValue(mMVPLoc, mProj * camera);
    mProgram->setUniformValue(mLightPosLoc, QVector3D(0.0f, 300.0f, 0.0f));

    mSwitchAnglesBuffer->bind();
    mSwitchAnglesBuffer->write(0, mSwitchAngles, sizeof(mSwitchAngles));
    mSwitchAnglesBuffer->release();

    mSwitchVAO->bind();
    f->glDrawArraysInstanced(GL_TRIANGLES, 0, mSwitchNPoints, 4 * 4);
    mSwitchVAO->release();

    mProgram->release();

    mFrameBuffer->release();

    QOpenGLFramebufferObject::blitFramebuffer(nullptr, mFrameBuffer.data());

    if (needUpdate)
    {
        update();
    }
}

void OpenGLWindow::resizeGL(int w, int h)
{
    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    mFrameBuffer.reset(new QOpenGLFramebufferObject(
        w, h, QOpenGLFramebufferObject::CombinedDepthStencil));
    mFrameBuffer->addColorAttachment(w, h);

    static GLenum drawBufs[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};

    f->glDrawBuffers(2, drawBufs);

    mProj.setToIdentity();
    mProj.perspective(45.0f, GLfloat(w) / h, 10.0f, 1000.0f);
}

void OpenGLWindow::mouseReleaseEvent(QMouseEvent* ev)
{
    if (mWin)
    {
        return;
    }

    int ID = getObjectID(ev->x(), mFrameBuffer->size().height() - ev->y());

    if (ID < 0)
    {
        return;
    }

    int x = ID % 4;
    int y = ID / 4;

    int wrongPlaced = 0;

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            float& current = mSwitchAnglesAspire[j * 4 + i];

            if (i == x || j == y)
            {
                current += 1.0f;
            }

            wrongPlaced += 1 - static_cast<int>(current) % 2;
        }
    }

    if (wrongPlaced == 0)
    {
        mWin = true;
    }

    mTime.restart();
    update();
}

int OpenGLWindow::loadModel(
    const char* iFileName,
    QSharedPointer<QOpenGLVertexArrayObject> oVAO,
    QSharedPointer<QOpenGLBuffer> oSwitchAngles) const
{
    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    Model model(iFileName);

    oVAO->create();
    oVAO->bind();

    QOpenGLBuffer vbo;
    vbo.create();
    vbo.bind();
    vbo.allocate(model.data(), sizeof(GLfloat) * model.points() * 6);
    f->glEnableVertexAttribArray(0);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);
    f->glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(GLfloat),
        reinterpret_cast<void*>(3 * sizeof(GLfloat)));
    vbo.release();

    if (oSwitchAngles)
    {
        oSwitchAngles->create();
        oSwitchAngles->bind();
        oSwitchAngles->allocate(mSwitchAngles, sizeof(mSwitchAngles));
        f->glEnableVertexAttribArray(2);
        f->glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, 0);
        f->glVertexAttribDivisor(2, 1);
        oSwitchAngles->release();
    }

    oVAO->release();

    return model.points();
}

int OpenGLWindow::getObjectID(int x, int y) const
{
    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    mFrameBuffer->bind();

    f->glReadBuffer(GL_COLOR_ATTACHMENT1);

    float data[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    f->glReadPixels(x, y, 1, 1, GL_RGBA, GL_FLOAT, &data);

    mFrameBuffer->release();

    return static_cast<int>(roundf(data[0] * 10) + roundf(data[1] * 10) * 10) -
        1;
}

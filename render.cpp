
#include "render.h"

#include "model.h"

#include <QtQuick/qquickwindow.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLShaderProgram>

#define GL_COLOR 0x1800
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
    "   mat4 verticalFlip = mat4("
    "      1.0, 0.0, 0.0, 0.0,"
    "      0.0, -1.0, 0.0, 0.0,"
    "      0.0, 0.0, 1.0, 0.0,"
    "      0.0, 0.0f, 0.0, 1.0);\n"
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
    "   gl_Position = verticalFlip * mvp * world * vec4(vertex, 1.0f);\n"
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
    "   fragColor = vec4(color * NL, 1.0);\n"
    "   objectID = vec4(vertObjectID, 1.0f);\n"
    "}\n";

QByteArray versionShaderCode(const char* src)
{
    QByteArray versionedSrc;

    if (QOpenGLContext::currentContext()->isOpenGLES())
        versionedSrc.append(QByteArrayLiteral("#version 300 es\n"));
    else
        versionedSrc.append(QByteArrayLiteral("#version 330\n"));

    versionedSrc.append(src);
    return versionedSrc;
}

SwitchRender::SwitchRender() : mSize(4)
{
    srand(time(NULL));

    init();
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
    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    if (!mProgram)
    {
        initialize();
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

    mProgram->bind();

    f->glEnable(GL_DEPTH_TEST);
    f->glEnable(GL_CULL_FACE);
    f->glDepthMask(GL_TRUE);
    f->glDepthFunc(GL_LESS);
    f->glFrontFace(GL_CCW);
    f->glCullFace(GL_BACK);

    // Clear the first AOV.
    static const float background[] = {0.1f, 0.2f, 0.3f, 1.0f};
    f->glClearBufferfv(GL_COLOR, 0, background);
    // Clear the second AOV.
    static const float black[] = {0.0f, 0.0f, 0.0f, 0.0f};
    f->glClearBufferfv(GL_COLOR, 1, black);
    // Clear the depth buffer.
    f->glClear(GL_DEPTH_BUFFER_BIT);

    // Setup camera.
    QMatrix4x4 camera;
    camera.setToIdentity();
    camera.lookAt(
        QVector3D(0.0f, 500.0f, 250.0f),
        QVector3D(0.0f, 0.0f, 30.0f),
        QVector3D(0.0f, 1.0f, 0.0f));
    mProgram->setUniformValue(mMVPLoc, mProj * camera);

    // Setup light position.
    mProgram->setUniformValue(mLightPosLoc, QVector3D(0.0f, 300.0f, 0.0f));

    mSwitchAnglesBuffer->bind();
    mSwitchAnglesBuffer->write(0, mSwitchAngles, sizeof(mSwitchAngles));
    mSwitchAnglesBuffer->release();

    mSwitchVAO->bind();
    f->glDrawArraysInstanced(GL_TRIANGLES, 0, mSwitchNPoints, mSize * mSize);
    mSwitchVAO->release();

    mProgram->release();

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

void SwitchRender::initialize()
{
    qDebug("Initializing OpenGL");

    mProgram.reset(new QOpenGLShaderProgram);
    mProgram->addShaderFromSourceCode(
        QOpenGLShader::Vertex, versionShaderCode(vertexShaderSource));
    mProgram->addShaderFromSourceCode(
        QOpenGLShader::Fragment, versionShaderCode(fragmentShaderSource));
    mProgram->link();

    mMVPLoc = mProgram->uniformLocation("mvp");
    mLightPosLoc = mProgram->uniformLocation("lightPos");

    mSwitchVAO.reset(new QOpenGLVertexArrayObject());
    mSwitchAnglesBuffer.reset(new QOpenGLBuffer());
    mSwitchNPoints = loadModel("switch.usda", mSwitchVAO, mSwitchAnglesBuffer);

    mTime.start();
}

int SwitchRender::loadModel(
    const char* iFileName,
    QSharedPointer<QOpenGLVertexArrayObject> oVAO,
    QSharedPointer<QOpenGLBuffer> oSwitchAngles)
{
    qDebug("Loading...");

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

int SwitchRender::getObjectID(int x, int y)
{
    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    framebufferObject()->bind();

    f->glReadBuffer(GL_COLOR_ATTACHMENT1);

    float d[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    f->glReadPixels(x, y, 1, 1, GL_RGBA, GL_FLOAT, &d);

    framebufferObject()->release();

    return static_cast<int>(roundf(d[0] * 10) + roundf(d[1] * 10) * 10) - 1;
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

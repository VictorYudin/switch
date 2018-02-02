
#include <object.h>

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

#define GL_COLOR 0x1800
#define GL_COLOR_ATTACHMENT1 (GL_COLOR_ATTACHMENT0 + 1)

static const char* vertexShaderSource =
    "layout(location = 0) in vec3 vertex;\n"
    "layout(location = 1) in vec3 normal;\n"
    "layout(location = 2) in float angle;\n"
    "uniform highp mat4 mvp;\n"
    "uniform highp vec3 camera;\n"
    "uniform highp int nrows;\n"
    "out vec3 vert;\n"
    "out vec3 vertNormal;\n"
    "out vec3 color;\n"
    "out vec3 vertObjectID;\n"
    "out vec4 V;\n"
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
    "   ivec2 index = ivec2(gl_InstanceID % nrows, gl_InstanceID / nrows);\n"
    "   float step = 3.0 / float(nrows - 1);\n"
    "   vec2 offset = vec2("
    "      (-1.5 + float(index.x) * step) * 105.0, "
    "      (-1.5 + float(index.y) * step) * 105.0);\n"
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
    "   V = vec4(camera, 1.0f) - world * vec4(vertex, 1.0f);\n"
    "}\n";

static const char* fragmentShaderSource =
    "uniform highp vec3 lightPos;\n"
    "in highp vec3 vert;\n"
    "in highp vec3 vertNormal;\n"
    "in highp vec3 color;\n"
    "in highp vec3 vertObjectID;\n"
    "in highp vec4 V;\n"
    "layout(location = 0) out highp vec4 fragColor;\n"
    "layout(location = 1) out highp vec4 objectID;\n"
    "highp float ggx (highp vec3 N, highp vec3 V, highp vec3 L, highp float "
    "roughness, highp float F0) {\n"
    "    highp float alpha = roughness*roughness;\n"
    "    highp vec3 H = normalize(L + V);\n"
    "    highp float dotLH = max(0.0, dot(L,H));\n"
    "    highp float dotNH = max(0.0, dot(N,H));\n"
    "    highp float dotNL = max(0.0, dot(N,L));\n"
    "    highp float alphaSqr = alpha * alpha;\n"
    "    highp float denom = dotNH * dotNH * (alphaSqr - 1.0) + 1.0;\n"
    "    highp float D = alphaSqr / (3.141592653589793 * denom * denom);\n"
    "    highp float F = F0 + (1.0 - F0) * pow(1.0 - dotLH, 5.0);\n"
    "    highp float k = 0.5 * alpha;\n"
    "    highp float k2 = k * k;\n"
    "    return dotNL * D * F / (dotLH*dotLH*(1.0-k2)+k2);\n"
    "}\n"
    "void main() {\n"
    "   highp vec3 L = lightPos - vert;\n"
    "   highp vec3 nL = normalize(L);\n"
    "   highp vec3 nN = normalize(vertNormal);\n"
    "   highp float NL = max(dot(nN, nL), 0.0);\n"
    "   highp float g = ggx(nN, V.xyz, nL, 0.5, 0.1);\n"
    "   fragColor = vec4(color * NL + vec3(g, g, g), 1.0);\n"
    "   objectID = vec4(vertObjectID, 1.0f);\n"
    "}\n";

QByteArray versionCode(const char* src)
{
    QByteArray versionedSrc;

    if (QOpenGLContext::currentContext()->isOpenGLES())
        versionedSrc.append(QByteArrayLiteral("#version 300 es\n"));
    else
        versionedSrc.append(QByteArrayLiteral("#version 330\n"));

    versionedSrc.append(src);
    return versionedSrc;
}

Object::Object()
{}

void Object::init(const char* iFileName, int iRows)
{
    qDebug("Initializing OpenGL");

    mNRows = iRows;

    mProgram.reset(new QOpenGLShaderProgram);
    mProgram->addShaderFromSourceCode(
        QOpenGLShader::Vertex, versionCode(vertexShaderSource));
    mProgram->addShaderFromSourceCode(
        QOpenGLShader::Fragment, versionCode(fragmentShaderSource));
    mProgram->link();

    mMVPLoc = mProgram->uniformLocation("mvp");
    mCamLoc = mProgram->uniformLocation("camera");
    mLightPosLoc = mProgram->uniformLocation("lightPos");
    mNRowsLoc = mProgram->uniformLocation("nrows");

    mVAO.reset(new QOpenGLVertexArrayObject());
    mAnglesBuffer.reset(new QOpenGLBuffer());
    mNPoints = loadModel(iFileName, mVAO, mAnglesBuffer);
}

void Object::render(
    const QMatrix4x4& iMVP,
    const QVector3D& iCameraLocation,
    const QVector3D& iLightLocation,
    const float* iAngles)
{
    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    mProgram->bind();

    // Set transform.
    mProgram->setUniformValue(mMVPLoc, iMVP);

    // Cat camera position.
    mProgram->setUniformValue(mCamLoc, iCameraLocation);

    // Setup light position.
    mProgram->setUniformValue(mLightPosLoc, iLightLocation);

    mProgram->setUniformValue(mNRowsLoc, mNRows);

    if (iAngles)
    {
        mAnglesBuffer->bind();
        mAnglesBuffer->write(0, iAngles, sizeof(*iAngles) * mNRows * mNRows);
        mAnglesBuffer->release();
    }

    mVAO->bind();
    f->glDrawElementsInstanced(
        GL_TRIANGLES, mNPoints, GL_UNSIGNED_INT, nullptr, mNRows * mNRows);
    mVAO->release();

    mProgram->release();
}

int Object::loadModel(
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

    QOpenGLBuffer ibo(QOpenGLBuffer::IndexBuffer);
    ibo.create();
    ibo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ibo.bind();
    ibo.allocate(model.indexData(), model.indexes() * sizeof(int));

    if (oSwitchAngles)
    {
        std::vector<GLfloat> angles(mNRows * mNRows, 0.0f);

        oSwitchAngles->create();
        oSwitchAngles->bind();
        oSwitchAngles->allocate(
            angles.data(),
            sizeof(decltype(angles)::value_type) * angles.size());
        f->glEnableVertexAttribArray(2);
        f->glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, 0);
        f->glVertexAttribDivisor(2, 1);
        oSwitchAngles->release();
    }

    oVAO->release();
    vbo.release();
    ibo.release();

    return model.indexes();
}

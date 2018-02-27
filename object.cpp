// Copyright 2018 Victor Yudin. All rights reserved.

#include "object.h"

#include <QFile>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

QByteArray versionShaderCode(const QByteArray& src)
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

void Object::init(const char* iFileName, int iID, int iRows)
{
    qDebug(iFileName);

    QFile vertexShader(":/default.vert");
    QFile fragmantShader(":/default.frag");
    if (!vertexShader.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug("Can't read vertex shader");
    }
    if (!fragmantShader.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug("Can't read fragment shader");
    }

    mNRows = iRows;
    mID = iID;

    mProgram.reset(new QOpenGLShaderProgram);
    mProgram->addShaderFromSourceCode(
        QOpenGLShader::Vertex, versionShaderCode(vertexShader.readAll()));
    mProgram->addShaderFromSourceCode(
        QOpenGLShader::Fragment, versionShaderCode(fragmantShader.readAll()));
    mProgram->link();

    mMVPLoc = mProgram->uniformLocation("mvp");
    mNRowsLoc = mProgram->uniformLocation("nrows");
    mIDLoc = mProgram->uniformLocation("id");

    mVAO.reset(new QOpenGLVertexArrayObject());
    mAnglesBuffer.reset(new QOpenGLBuffer());
    mNPoints = loadModel(iFileName, mVAO, mAnglesBuffer);
}

void Object::render(const QMatrix4x4& iMVP, const float* iAngles) const
{
    QOpenGLExtraFunctions* f =
        QOpenGLContext::currentContext()->extraFunctions();

    mProgram->bind();

    // Set transform.
    mProgram->setUniformValue(mMVPLoc, iMVP);
    mProgram->setUniformValue(mNRowsLoc, mNRows);
    mProgram->setUniformValue(mIDLoc, mID);

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
    vbo.allocate(model.data(), sizeof(GLfloat) * model.points() * 3 * 3);
    for (int i = 0; i < 3; i++)
    {
        f->glEnableVertexAttribArray(i);
        f->glVertexAttribPointer(
            i,
            3,
            GL_FLOAT,
            GL_FALSE,
            9 * sizeof(GLfloat),
            reinterpret_cast<void*>(i * 3 * sizeof(GLfloat)));
    }

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
        f->glEnableVertexAttribArray(3);
        f->glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, 0);
        f->glVertexAttribDivisor(3, 1);
        oSwitchAngles->release();
    }

    oVAO->release();
    vbo.release();
    ibo.release();

    return model.indexes();
}

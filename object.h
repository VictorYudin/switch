// Copyright 2018 Victor Yudin. All rights reserved.

#ifndef __OBJECT_H_
#define __OBJECT_H_

#include <model.h>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QSharedPointer>

/**
 * @brief Adds the OpenGL version specification string like "#version 300 es" to
 * the shader source.
 *
 * @param src The shader source.
 *
 * @return Full shader source with the version specification string.
 */
QByteArray versionShaderCode(const QByteArray& src);

/** @brief Keeps the OpenGL buffers to draw an object in the scene. */
class Object
{
public:
    Object();

    /**
     * @brief Initializes object, OpenGL buffers and programs from the external
     * file. It's not in the initializer becuse it can be possible that the
     * initialization should be in the OpenGL context.
     *
     * It can use OpenGL instancing to draw NxN shapes.
     *
     * @param iFileName The file with the vertex data.
     * @param iID ID of the object. Is used for the selection.
     * @param iRows Allows to draw NxN instances. If it's >1, then there will be
     *        iRows * iRows objects.
     */
    void init(const char* iFileName, int iID, int iRows);

    /**
     * @brief Checks if the object was initialized successfully.
     *
     * @return True if it's a valid object.
     */
    bool valid() const { return !mVAO.isNull(); }

    /**
     * @brief Renders the object with OpenGL.
     *
     * @param iMVP The model view projection matrix.
     * @param iAngles Specifies the angle of rotation of each instance.
     */
    void render(const QMatrix4x4& iMVP, const float* iAngles) const;

private:
    /**
     * @brief Reads the file with a model and returns Qt-OpenGL buffers.
     *
     * @param iFileName The file with the vertex data.
     * @param oVAO The vertex buffer.
     * @param oSwitchAngles The buffer with the angles for each instance.
     *
     * @return Number of points of the initialized mesh.
     */
    int loadModel(
        const char* iFileName,
        QSharedPointer<QOpenGLVertexArrayObject> oVAO,
        QSharedPointer<QOpenGLBuffer> oSwitchAngles) const;

    // The main shader that is used to draw it.
    QSharedPointer<QOpenGLShaderProgram> mProgram;
    // The vertex buffer with the mesh.
    QSharedPointer<QOpenGLVertexArrayObject> mVAO;
    // The buffer with angles for each instance.
    QSharedPointer<QOpenGLBuffer> mAnglesBuffer;
    // The number of points in the mesh.
    int mNPoints;

    // The uniform location of the MVP matrix in the program.
    int mMVPLoc;
    // The uniform location of the number of rows in the program.
    int mNRowsLoc;
    // The uniform location of the object ID in the program.
    int mIDLoc;

    // The number of rows. The total number of instances is mNRows * mNRows;
    int mNRows;
    // The object ID.
    int mID;
};

#endif

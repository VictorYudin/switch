
#ifndef __OBJECT_H_
#define __OBJECT_H_

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QSharedPointer>
#include <model.h>

QByteArray versionShaderCode(const QByteArray& src);

class Object
{
public:
    Object();

    void init(const char* iFileName, int iID, int iRows);

    bool valid() const { return !mVAO.isNull(); }

    void render(const QMatrix4x4& iMVP, const float* iAngles) const;

private:
    int loadModel(
        const char* iFileName,
        QSharedPointer<QOpenGLVertexArrayObject> oVAO,
        QSharedPointer<QOpenGLBuffer> oSwitchAngles);

    QSharedPointer<QOpenGLShaderProgram> mProgram;
    QSharedPointer<QOpenGLVertexArrayObject> mVAO;
    QSharedPointer<QOpenGLBuffer> mAnglesBuffer;
    int mNPoints;

    int mMVPLoc;
    int mNRowsLoc;
    int mIDLoc;

    int mNRows;
    int mID;
};

#endif

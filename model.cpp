#include "model.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/js/json.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <QCoreApplication>
#include <QDir>

PXR_NAMESPACE_USING_DIRECTIVE

extern "C" bool _DefaultReadPlugInfoObject(
    const std::string& pathname,
    JsObject* result);
extern "C" bool GoodReadPlugInfoObject(
    const std::string& pathname,
    JsObject* result)
{
    QString plugInfoPath =
        QDir(QCoreApplication::applicationDirPath()).filePath("plugInfo.json");
    return _DefaultReadPlugInfoObject(plugInfoPath.toStdString(), result);
}

extern "C" bool (*_ReadPlugInfoObject)(
    const std::string& pathname,
    JsObject* result) = GoodReadPlugInfoObject;

Model::Model(const char* iFile)
{
    QString switchPath =
        QDir(QCoreApplication::applicationDirPath()).filePath(iFile);

    UsdStageRefPtr stage = UsdStage::Open(switchPath.toStdString().c_str());
    if (!TF_VERIFY(stage, "Can't open %s", switchPath.toStdString().c_str()))
    {
        return;
    }

    qDebug(switchPath.toStdString().c_str());

    UsdPrimRange range(stage->GetPseudoRoot());
    for (const UsdPrim& prim : range)
    {
        if (!prim.IsA<UsdGeomMesh>())
        {
            continue;
        }

        UsdGeomMesh mesh(prim);

        VtIntArray faceVertexCounts;
        mesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts);

        VtIntArray faceVertexIndices;
        mesh.GetFaceVertexIndicesAttr().Get(&faceVertexIndices);

        VtArray<GfVec3f> points;
        mesh.GetPointsAttr().Get(&points);

        int numTriangles = 0;
        for (int n : faceVertexCounts)
        {
            if (n < 3)
            {
                continue;
            }

            numTriangles += n - 3;
        }

        GfMatrix4f matrix(mesh.ComputeLocalToWorldTransform(0.0));

        mData.reserve(mData.size() + numTriangles * 3 * 3 * 2);

        int counter = 0;
        for (int n : faceVertexCounts)
        {
            for (int i = 2; i < n; i++)
            {
                const GfVec3f trianglePoints[] = {
                    matrix.Transform(points[faceVertexIndices[counter]]),
                    matrix.Transform(
                        points[faceVertexIndices[counter + i - 1]]),
                    matrix.Transform(points[faceVertexIndices[counter + i]])};

                const GfVec3f u = trianglePoints[1] - trianglePoints[0];
                const GfVec3f v = trianglePoints[2] - trianglePoints[0];
                const GfVec3f normal = GfCross(u, v).GetNormalized();

                for (int p = 2; p >= 0; p--)
                {
                    mData.push_back(trianglePoints[p][0]);
                    mData.push_back(trianglePoints[p][1]);
                    mData.push_back(trianglePoints[p][2]);

                    mData.push_back(normal[0]);
                    mData.push_back(normal[1]);
                    mData.push_back(normal[2]);
                }
            }

            counter += n;
        }

        TF_STATUS("%s: %i", prim.GetPath().GetText(), numTriangles);
    }

    return;
}

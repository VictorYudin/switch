#include "model.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <opensubdiv/far/primvarRefiner.h>
#include <opensubdiv/far/topologyDescriptor.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/js/json.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

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

struct Vertex
{
    // Minimal required interface ----------------------
    Vertex() {}

    Vertex(Vertex const& src)
    {
        _position[0] = src._position[0];
        _position[1] = src._position[1];
        _position[2] = src._position[2];
    }

    void Clear(void* = 0) { _position[0] = _position[1] = _position[2] = 0.0f; }

    void AddWithWeight(Vertex const& src, float weight)
    {
        _position[0] += weight * src._position[0];
        _position[1] += weight * src._position[1];
        _position[2] += weight * src._position[2];
    }

    // Public interface ------------------------------------
    void SetPosition(float x, float y, float z)
    {
        _position[0] = x;
        _position[1] = y;
        _position[2] = z;
    }

    const float* GetPosition() const { return _position; }

private:
    float _position[3];
};

// Returns the cross product of \p v1 and \p v2.
void cross(float const* v1, float const* v2, float* vOut)
{
    vOut[0] = v1[1] * v2[2] - v1[2] * v2[1];
    vOut[1] = v1[2] * v2[0] - v1[0] * v2[2];
    vOut[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

void subdivide(
    int iNverts,
    int iNfaces,
    float* iVerts,
    int* iVertsperface,
    int* iVertIndices,
    int maxlevel,
    VtIntArray& oFaceVertexCounts,
    VtIntArray& oFaceVertexIndices,
    VtArray<GfVec3f>& oPoints,
    VtArray<GfVec3f>& oNormals)
{
    // Populate a topology descriptor with our raw data
    typedef OpenSubdiv::Far::TopologyDescriptor Descriptor;

    OpenSubdiv::Sdc::SchemeType type = OpenSubdiv::Sdc::SCHEME_CATMARK;

    OpenSubdiv::Sdc::Options sdcOptions;
    sdcOptions.SetVtxBoundaryInterpolation(
        OpenSubdiv::Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);

    Descriptor desc;
    desc.numVertices = iNverts;
    desc.numFaces = iNfaces;
    desc.numVertsPerFace = iVertsperface;
    desc.vertIndicesPerFace = iVertIndices;

    // Instantiate a FarTopologyRefiner from the descriptor
    OpenSubdiv::Far::TopologyRefiner* refiner =
        OpenSubdiv::Far::TopologyRefinerFactory<Descriptor>::Create(
            desc,
            OpenSubdiv::Far::TopologyRefinerFactory<Descriptor>::Options(
                type, sdcOptions));

    // Uniformly refine the topolgy up to 'maxlevel'
    OpenSubdiv::Far::TopologyRefiner::UniformOptions options(maxlevel);

    // Required by Limit()
    options.fullTopologyInLastLevel = true;

    // Uniformly refine the topology up to 'maxlevel'
    refiner->RefineUniform(options);

    // Allocate a buffer for vertex primvar data. The buffer length is set to
    // be the sum of all children vertices up to the highest level of
    // refinement.
    std::vector<Vertex> vbuffer(refiner->GetNumVerticesTotal());
    Vertex* verts = &vbuffer[0];

    // Initialize coarse mesh positions
    int nCoarseVerts = iNverts;
    for (int i = 0; i < nCoarseVerts; ++i)
    {
        verts[i].SetPosition(
            iVerts[i * 3 + 0], iVerts[i * 3 + 1], iVerts[i * 3 + 2]);
    }

    // Interpolate vertex primvar data
    OpenSubdiv::Far::PrimvarRefiner primvarRefiner(*refiner);

    Vertex* src = verts;
    for (int level = 1; level <= maxlevel; ++level)
    {
        Vertex* dst = src + refiner->GetLevel(level - 1).GetNumVertices();
        primvarRefiner.Interpolate(level, src, dst);
        src = dst;
    }

    // Limit position, derivatives and normals
    int nFineVerts = refiner->GetLevel(maxlevel).GetNumVertices();
    std::vector<Vertex> fineLimitPos(nFineVerts);
    std::vector<Vertex> fineDu(nFineVerts);
    std::vector<Vertex> fineDv(nFineVerts);

    primvarRefiner.Limit(src, fineLimitPos, fineDu, fineDv);

    {
        OpenSubdiv::Far::TopologyLevel const& refLastLevel =
            refiner->GetLevel(maxlevel);

        int nverts = refLastLevel.GetNumVertices();
        int nfaces = refLastLevel.GetNumFaces();

        // Print vertex positions
        int firstOfLastVerts = refiner->GetNumVerticesTotal() - nverts;

        oPoints.resize(nverts);
        oNormals.resize(nverts);

        for (int vert = 0; vert < nverts; ++vert)
        {
            float const* pos = verts[firstOfLastVerts + vert].GetPosition();
            oPoints[vert] = GfVec3f(pos[0], pos[1], pos[2]);

            float norm[3];
            float const* du = fineDu[vert].GetPosition();
            float const* dv = fineDv[vert].GetPosition();

            cross(du, dv, norm);
            oNormals[vert] = GfVec3f(norm[0], norm[1], norm[2]);
        }

        oFaceVertexCounts.resize(nfaces);

        int nIndices = 0;
        for (int face = 0; face < nfaces; ++face)
        {
            OpenSubdiv::Far::ConstIndexArray fverts =
                refLastLevel.GetFaceVertices(face);

            // All refined Catmark faces should be quads
            assert(fverts.size() == 4);
            oFaceVertexCounts[face] = fverts.size();
            nIndices += fverts.size();
        }

        oFaceVertexIndices.resize(nIndices);

        int counter = 0;
        for (int face = 0; face < nfaces; ++face)
        {
            OpenSubdiv::Far::ConstIndexArray fverts =
                refLastLevel.GetFaceVertices(face);

            for (int vert = 0; vert < fverts.size(); ++vert)
            {
                oFaceVertexIndices[counter++] = fverts[vert];
            }
        }
    }
}

static float g_verts[8][3] = {{-50.0f, -50.0f, 50.0f},
                              {50.0f, -50.0f, 50.0f},
                              {-50.0f, 50.0f, 50.0f},
                              {50.0f, 50.0f, 50.0f},
                              {-50.0f, 50.0f, -50.0f},
                              {50.0f, 50.0f, -50.0f},
                              {-50.0f, -50.0f, -50.0f},
                              {50.0f, -50.0f, -50.0f}};

static int g_nverts = 8;
static int g_nfaces = 6;

static int g_vertsperface[6] = {4, 4, 4, 4, 4, 4};

static int g_vertIndices[24] = {0, 1, 3, 2, 2, 3, 5, 4, 4, 5, 7, 6,
                                6, 7, 1, 0, 1, 7, 5, 3, 6, 0, 2, 4};

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

        // Get the mesh.
        UsdGeomMesh mesh(prim);

        // Get the transform.
        GfMatrix4f matrix(mesh.ComputeLocalToWorldTransform(0.0));
        int pointsStartFrom = mData.size() / 6;

        // Get the mesh data.
        VtIntArray faceVertexCounts;
        mesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts);

        VtIntArray faceVertexIndices;
        mesh.GetFaceVertexIndicesAttr().Get(&faceVertexIndices);

        VtArray<GfVec3f> points;
        mesh.GetPointsAttr().Get(&points);

        VtArray<GfVec3f> normals;

        // Subdivide it.
#if 1
        subdivide(
            points.size(),
            faceVertexCounts.size(),
            reinterpret_cast<float*>(points.data()),
            faceVertexCounts.data(),
            faceVertexIndices.data(),
            1,
            faceVertexCounts,
            faceVertexIndices,
            points,
            normals);
#else
        subdivide(
            g_nverts,
            g_nfaces,
            &(g_verts[0][0]),
            g_vertsperface,
            g_vertIndices,
            4,
            faceVertexCounts,
            faceVertexIndices,
            points,
            normals);
#endif

        mData.reserve(mData.size() + points.size() * 3 * 2);
        for (int i = 0; i < points.size(); i++)
        {
            const GfVec3f pt = matrix.Transform(points[i]);
            const GfVec3f nt = matrix.Transform(normals[i]);
            mData.push_back(pt[0]);
            mData.push_back(pt[1]);
            mData.push_back(pt[2]);
            mData.push_back(nt[0]);
            mData.push_back(nt[1]);
            mData.push_back(nt[2]);
        }

        int numTriangles = 0;
        for (int n : faceVertexCounts)
        {
            if (n < 3)
            {
                continue;
            }

            numTriangles += n - 3;
        }

        mIndexData.reserve(mIndexData.size() + numTriangles * 3);

        int counter = 0;
        for (int n : faceVertexCounts)
        {
            for (int i = 2; i < n; i++)
            {
#if 1
                mIndexData.push_back(
                    pointsStartFrom + faceVertexIndices[counter + i]);
                mIndexData.push_back(
                    pointsStartFrom + faceVertexIndices[counter + i - 1]);
                mIndexData.push_back(
                    pointsStartFrom + faceVertexIndices[counter]);
#else
                mIndexData.push_back(
                    pointsStartFrom + faceVertexIndices[counter]);
                mIndexData.push_back(
                    pointsStartFrom + faceVertexIndices[counter + i - 1]);
                mIndexData.push_back(
                    pointsStartFrom + faceVertexIndices[counter + i]);
#endif
            }

            counter += n;
        }
    }

    return;
}

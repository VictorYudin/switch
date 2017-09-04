#include "scene.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/predef/os.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>
#include <string>

#if (BOOST_OS_WINDOWS)
#  include <stdlib.h>
#elif (BOOST_OS_LINUX)
#  include <unistd.h>
#  include <limits.h>
#elif (BOOST_OS_MACOS)
#  include <mach-o/dyld.h>
#endif

// Returns the full path to the currently running executable.
boost::filesystem::path currentPath()
{
#if (BOOST_OS_WINDOWS)
    char *exePath;
    if (_get_pgmptr(&exePath) != 0)
    {
        exePath = "";
    }
#elif (BOOST_OS_LINUX)
    char exePath[PATH_MAX];
    ssize_t len = ::readlink("/proc/self/exe", exePath, sizeof(exePath));
    if (len == -1 || len == sizeof(exePath))
    {
        len = 0;
    }
    exePath[len] = '\0';
#elif (BOOST_OS_MACOS)
    char exePath[PATH_MAX];
    uint32_t len = sizeof(exePath);
    if (_NSGetExecutablePath(exePath, &len) != 0)
    {
        exePath[0] = '\0'; // buffer too small (!)
    }
    else
    {
        // resolve symlinks, ., .. if possible
        char *canonicalPath = realpath(exePath, NULL);
        if (canonicalPath != NULL)
        {
            strncpy(exePath, canonicalPath, len);
            free(canonicalPath);
        }
    }
#endif

    boost::system::error_code ec;
    boost::filesystem::path path(
        boost::filesystem::canonical(
            exePath, boost::filesystem::current_path(), ec));
    return path.parent_path();
}


Scene::Scene() :
    mWidth(0),
    mHeight(0),
    mWon(false)
{
    boost::filesystem::path current = currentPath();
    boost::filesystem::path boardPath = current / "board.usda";
    boost::filesystem::path switchPath = current / "switch.usda";

    mStage = UsdStage::Open(boardPath.string());
    UsdStageRefPtr switchStage = UsdStage::Open(switchPath.string());

    mBoard = mStage->GetPrimAtPath(SdfPath("/board1"));
    auto cameraPrim = mStage->GetPrimAtPath(SdfPath("/camera1"));
    mCamera = UsdGeomCamera(cameraPrim).GetCamera(UsdTimeCode::Default());

    // Static USD produces warning that the visibility attribute doesn't exist.
    {
        UsdGeomImageable imBoard(mBoard);
        imBoard.CreateVisibilityAttr();
    }
    UsdPrimRange range(switchStage->GetPrimAtPath(SdfPath::AbsoluteRootPath()));
    // Iterate everything except root.
    for (auto it = ++range.begin(); it != range.end(); it++)
    {
        const auto& prim = *it;
        UsdGeomImageable imageable(prim);
        imageable.CreateVisibilityAttr();
    }

    srand(time(NULL));

    char name[32];
    for (int i=0; i<4; i++)
    {
        for (int j=0; j<4; j++)
        {
            sprintf(name, "/board1/xf%ix%i", i, j);
            auto xfPrim = mStage->DefinePrim(SdfPath(name), TfToken("Xform"));

            // Static USD produces warning that the visibility attribute doesn't
            // exist.
            UsdGeomImageable imageable(xfPrim);
            imageable.CreateVisibilityAttr();

            UsdGeomXformCommonAPI(xfPrim).SetTranslate(
                    GfVec3f(105.0f * i, 0.0f, -105.0f * j));

            // Set attributes
            // Number of turns
            auto turnsAttr = xfPrim.CreateAttribute(
                    TfToken("turns"),
                    SdfValueTypeNames->Int,
                    true);
            turnsAttr.Set(rand() % 2);

            // Index
            auto indexIAttr = xfPrim.CreateAttribute(
                    TfToken("indexI"),
                    SdfValueTypeNames->Int,
                    true);
            indexIAttr.Set(i);
            auto indexJAttr = xfPrim.CreateAttribute(
                    TfToken("indexJ"),
                    SdfValueTypeNames->Int,
                    true);
            indexJAttr.Set(j);

            // Add an object
            sprintf(name, "%s/switch", name);
            auto instPrim = mStage->DefinePrim(SdfPath(name));
            instPrim.GetReferences().AddReference(
                    switchStage->GetRootLayer()->GetIdentifier(),
                    SdfPath("/switch1"));
        }
    }

    GLint major = 0;
    GLint minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    printf("OpenGL version is %i.%i\n", major, minor);

    printf(
        "Hydra is %s\n",
        UsdImagingGL::IsEnabledHydra() ? "enabled" : "disabled");

    mParams.frame = 1.0;
    mParams.complexity = 1.1f;
}

void Scene::prepare(float seconds)
{
    bool rotated = false;

    for(const auto& prim : mBoard.GetChildren())
    {
        auto turnsAttr = prim.GetAttribute(TfToken("turns"));
        if (!turnsAttr.IsValid())
        {
            continue;
        }

        UsdGeomXformCommonAPI xf(prim);

        GfVec3d translation;
        GfVec3f rotation;
        GfVec3f scale;
        GfVec3f pivot;
        UsdGeomXformCommonAPI::RotationOrder rotOrder;

        xf.GetXformVectors(
                &translation, 
                &rotation,
                &scale,
                &pivot,
                &rotOrder,
                UsdTimeCode::Default());

        int turns;
        turnsAttr.Get(&turns);

        float target = std::min(90.0f * turns, rotation[1] + 300.0f * seconds);
        if (target > rotation[1])
        {
            rotation[1] = target;
            xf.SetRotate(rotation);
            rotated = true;
        }

        if (mCurrent.HasPrefix(prim.GetPath()))
        {
            float s = std::min(scale[0]+seconds, 1.1f);
            if (scale[0] < s)
            {
                xf.SetScale(GfVec3f(s, s, s));
            }
        }
        else
        {
            float s = std::max(scale[0]-seconds, 1.0f);
            if (scale[0] > s)
            {
                xf.SetScale(GfVec3f(s, s, s));
            }
        }

        if (mWon > 1)
        {
            double t = std::max(translation[1]-100.0*seconds, -200.0);
            if (translation[1] > t)
            {
                translation[1] = t;
                xf.SetTranslate(translation);
            }
        }
    }

    if (mWon && !rotated)
    {
        mWon = 2;
    }
}

void Scene::draw(int width, int height)
{
    mWidth = width;
    mHeight = height;

    auto frustum = mCamera.GetFrustum();

    mRenderer.SetCameraState(
            frustum.ComputeViewMatrix(),
            frustum.ComputeProjectionMatrix(),
            GfVec4d(0, 0, mWidth, mHeight));

    // USD render.
    mRenderer.Render(mBoard, mParams);

    // Clear OpenGL errors. Because UsdImagingGL::TestIntersection prints them.
    while (glGetError() != GL_NO_ERROR) {}
}

void Scene::click()
{
    if (mCurrent.IsEmpty())
    {
        return;
    }

    auto clickedPrim = mStage->GetPrimAtPath(mCurrent);
    while (clickedPrim.IsValid() && clickedPrim.GetTypeName() != "Xform")
    {
        clickedPrim = clickedPrim.GetParent();
    }

    if (!clickedPrim.IsValid())
    {
        return;
    }

    // Get index
    int clickedI;
    int clickedJ;
    auto indexIAttr = clickedPrim.GetAttribute(TfToken("indexI"));
    auto indexJAttr = clickedPrim.GetAttribute(TfToken("indexJ"));
    indexIAttr.Get(&clickedI);
    indexJAttr.Get(&clickedJ);

    mWon = 1;

    // Turn the switches of the same row and column
    for(const auto& prim : mBoard.GetChildren())
    {
        auto turnsAttr = prim.GetAttribute(TfToken("turns"));
        if (!turnsAttr.IsValid())
        {
            continue;
        }

        indexIAttr = prim.GetAttribute(TfToken("indexI"));
        indexJAttr = prim.GetAttribute(TfToken("indexJ"));
        int i;
        int j;
        indexIAttr.Get(&i);
        indexJAttr.Get(&j);

        int turns;
        turnsAttr.Get(&turns);

        if (clickedI == i || clickedJ == j)
        {
            turnsAttr.Set(++turns);
        }

        mWon = mWon & (turns%2);
    }
}

void Scene::cursor(float x, float y)
{
    GfVec2d size(1.0 / mWidth, 1.0 / mHeight);

    // Compute pick frustum.
    auto cameraFrustum = mCamera.GetFrustum();
    auto frustum = cameraFrustum.ComputeNarrowedFrustum(
            GfVec2d(2.0 * x - 1.0, 2.0 * (1.0-y) - 1.0),
            size);

    GfVec3d outHitPoint;
    SdfPath outHitPrimPath;

    if (mRenderer.TestIntersection(
            frustum.ComputeViewMatrix(),
            frustum.ComputeProjectionMatrix(),
            GfMatrix4d(1.0),
            mBoard,
            mParams,
            &outHitPoint,
            &outHitPrimPath))
    {
        if (outHitPrimPath != mCurrent)
        {
            mCurrent = outHitPrimPath;
        }
    }
    else
    {
        mCurrent = SdfPath();
    }
}

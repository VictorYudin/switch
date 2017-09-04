
#ifndef __SCENE_H__
#define __SCENE_H__

#include <pxr/imaging/glf/glew.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#ifdef _WIN32
  #include <windows.h>
#endif
#include <GL/gl.h>
#endif

#include <GLFW/glfw3.h>

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usdImaging/usdImagingGL/gl.h>
#include <pxr/base/gf/camera.h>

PXR_NAMESPACE_USING_DIRECTIVE

class Scene
{
public:
    Scene();
    void prepare(float seconds);
    void draw(int width, int height);
    void click();
    void cursor(float x, float y);

private:

    void _SaveBindingState();
    void _RestoreBindingState();

    UsdStageRefPtr mStage;
    SdfPathVector mExcludePaths;
    UsdImagingGL mRenderer;
    UsdImagingGLEngine::RenderParams mParams;
    UsdPrim mBoard;
    GfCamera mCamera;
    SdfPath mCurrent;
    int mWidth;
    int mHeight;
    int mWon;
};

#endif


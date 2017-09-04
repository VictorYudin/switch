@echo off
set current_dir=%~dp0

echo Building %current_dir%

set BUILD_CONFIG=Release
set DEFINES=/EHsc /DBOOST_ALL_NO_LIB /DPTEX_STATIC /DBOOST_ALL_STATIC_LINK /DBOOST_PYTHON_STATIC_LIB /DNOMINMAX
set LIB_ROOT=%current_dir%dependencies\lib
set TBB_LIBRARY=%LIB_ROOT%\tbb\lib
set TBB_ROOT_DIR=%LIB_ROOT%\tbb\include

%LIB_ROOT%\cmake\bin\cmake ^
    -G "NMake Makefiles" ^
    -DBOOST_ROOT:PATH=%current_dir%boost ^
    -DBoost_USE_STATIC_LIBS:BOOL=ON ^
    -DCMAKE_BUILD_TYPE=%BUILD_CONFIG% ^
    -DCMAKE_CXX_FLAGS="/MT %DEFINES%" ^
    -DCMAKE_CXX_FLAGS_DEBUG="/MTd %DEFINES%" ^
    -DCMAKE_CXX_FLAGS_RELEASE="/MT %DEFINES%" ^
    -DCMAKE_C_FLAGS="/MT %DEFINES%" ^
    -DCMAKE_C_FLAGS_DEBUG="/MTd %DEFINES%" ^
    -DCMAKE_C_FLAGS_RELEASE="/MT %DEFINES%" ^
    -DCMAKE_INSTALL_PREFIX:PATH=%current_dir%switch ^
    -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON ^
    -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON ^
    -DGLEW_LOCATION:PATH=%LIB_ROOT%\glew ^
    -DGLFW_LOCATION:PATH=%LIB_ROOT%\glfw ^
    -DILMBASE_HOME:PATH=%LIB_ROOT%\ilmbase ^
    -DILMBASE_ROOT:PATH=%LIB_ROOT%\ilmbase ^
    -DJPEG_PATH:PATH=%LIB_ROOT%\jpeg ^
    -DLIB_ROOT:PATH=%LIB_ROOT% ^
    -DOIIO_LOCATION:PATH=%LIB_ROOT%\oiio ^
    -DOPENEXR_HOME:PATH=%LIB_ROOT%\openexr ^
    -DOPENSUBDIV_ROOT_DIR:PATH=%LIB_ROOT%\opensubd ^
    -DPNG_LIBRARY=%LIB_ROOT%\png\lib\libpng16_static.lib ^
    -DPNG_PNG_INCLUDE_DIR:PATH=%LIB_ROOT%\png\include ^
    -DPTEX_LOCATION:PATH=%LIB_ROOT%\ptex ^
    -DPYTHON_EXECUTABLE=%current_dir%python\python.exe ^
    -DTBB_LIBRARIES=%TBB_LIBRARY% ^
    -DTBB_LIBRARY=%TBB_LIBRARY% ^
    -DTBB_ROOT_DIR:PATH=%TBB_ROOT_DIR% ^
    -DTIFF_INCLUDE_DIR:PATH=%LIB_ROOT%\tiff\include ^
    -DTIFF_LIBRARY=%LIB_ROOT%\tiff\lib\libtiff.lib ^
    -DUSD_ROOT:PATH=%LIB_ROOT%\usd ^
    -DUSE_HDF5:BOOL=ON ^
    -DUSE_STATIC_HDF5:BOOL=ON ^
    -DZLIB_ROOT:PATH=%LIB_ROOT%\zlib ^
    %current_dir% && ^
%LIB_ROOT%\cmake\bin\cmake ^
    --build . ^
    --target package ^
    --config %BUILD_CONFIG%

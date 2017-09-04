cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/share)

project(switch)

#variable helpers
# Cmake system specific flags
set(WINDOWS FALSE)
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
    set(WINDOWS TRUE)
endif()

set(DARWIN FALSE)
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
    set(DARWIN TRUE)
    # suppress rpath warning
    if (POLICY CMP0042)
        cmake_policy(SET CMP0042 OLD)
    endif()
endif()

set(LINUX FALSE)
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
    set(LINUX TRUE)
endif()

if (LINUX OR DARWIN)
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
    if (COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    elseif (COMPILER_SUPPORTS_CXX0X)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    endif()
endif()

# Find USD
find_package(USD REQUIRED)
find_package(
    Boost REQUIRED
    COMPONENTS regex system filesystem thread chrono date_time atomic python)
find_package(GLFW REQUIRED)
find_package(IlmBase REQUIRED)
find_package(JPEG REQUIRED)
find_package(OpenEXR REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenImageIO REQUIRED)
find_package(OpenSubdiv REQUIRED)
find_package(PNG REQUIRED)
find_package(PTex REQUIRED)
find_package(PythonLibs 2.7 REQUIRED)
find_package(TBB REQUIRED)
find_package(TIFF REQUIRED)
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)
find_package(Zlib REQUIRED)

# Standard FindGLEW is looking in the system. Fix it.
set(GLEW_INCLUDE_DIR ${GLEW_LOCATION}/include)
file(GLOB GLEW_LIBRARY "${GLEW_LOCATION}/lib/libGLEW*")
find_package(GLEW REQUIRED)

### Switch app
add_executable(switch main.cpp scene.cpp)

# Definitions
target_compile_definitions(
    switch
    PRIVATE
    BUILD_COMPONENT_SRC_PREFIX=\"\"
    BUILD_OPTLEVEL_OPT
    MFB_ALT_PACKAGE_NAME=switch
    TF_NO_GNU_EXT)

# Includes
target_include_directories(
    switch
    PRIVATE
    .
    ${USD_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${GLEW_INCLUDE_DIR}
    ${TBB_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR})

# USD libraries.
set(USD_ALL_LIBS
    ${ALEMBIC_ILMBASE_HALF_LIB}
    ${ALEMBIC_ILMBASE_IEX_LIB}
    ${USD_LIBRARIES}
    ${OIIO_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${GLEW_LIBRARIES}
    ${JPEG_LIBRARY}
    ${OPENEXR_LIBRARIES}
    ${OPENSUBDIV_LIBRARIES}
    ${PNG_LIBRARY}
    ${PTEX_LIBRARY}
    ${PYTHON_LIBRARIES}
    ${TBB_LIBRARIES}
    TIFF::TIFF
    ${Boost_ATOMIC_LIBRARY}
    ${Boost_CHRONO_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_PYTHON_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_REGEX_LIBRARY}
    ${OPENGL_gl_LIBRARY}
    ${OPENGL_glu_LIBRARY}
    ${X11_X11_LIB}
    ${X11_Xcursor_LIB}
    ${X11_Xext_LIB}
    ${X11_Xi_LIB}
    ${X11_Xinerama_LIB}
    ${X11_Xrandr_LIB}
    ${X11_Xxf86vm_LIB}
    ${ZLIB_LIB}
    ${GLFW_LIBRARY}
    Wbemuuid
    Shlwapi
    Dbghelp)

if(MSVC)
    target_link_libraries(switch PRIVATE ${USD_ALL_LIBS})
elseif(CMAKE_COMPILER_IS_GNUCXX)
    target_link_libraries(
        switch -Wl,--whole-archive ${USD_ALL_LIBS} -Wl,--no-whole-archive)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    target_link_libraries(switch -Wl,-force_load ${USD_ALL_LIBS})
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    install(TARGETS switch RUNTIME DESTINATION bin)
else()
    install(TARGETS switch DESTINATION bin)
endif()

install(FILES board.usda switch.usda DESTINATION bin)
install(DIRECTORY ${USD_ROOT}/share/usd DESTINATION share)

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "switch")
include(CPack)

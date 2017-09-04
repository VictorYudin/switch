# Trying to find USD include path.
find_path (
    USD_INCLUDE_DIR
    pxr/pxr.h
    PATHS ${USD_ROOT}/include
    NO_DEFAULT_PATH)

# Trying to get version
file(STRINGS ${USD_INCLUDE_DIR}/pxr/pxr.h TMP REGEX "^#define PXR_VERSION .*$")
string (REGEX MATCHALL "[0-9]+" USD_VERSION ${TMP})

# The list of required libraries for minimal USD.
set (_usd_components
    usdImagingGL
    usdImaging
    usdHydra
    hdx
    hdSt
    hdStream
    hd
    glf
    garch
    pxOsd
    usdRi
    usdUI
    usdShade
    usdGeom
    usd
    usdUtils
    tracelite
    pcp
    sdf
    plug
    js
    ar
    work
    tf
    kind
    arch
    vt
    gf
    hf
    cameraUtil
    usdLux)

set (USD_LIBRARIES "")

# Trying to find all the libraries.
foreach (COMPONENT ${_usd_components})
    string (TOUPPER ${COMPONENT} UPPERCOMPONENT)

    unset (USD_${UPPERCOMPONENT}_LIBRARY CACHE)

    find_library (
        USD_${UPPERCOMPONENT}_LIBRARY
        NAMES ${COMPONENT} ${COMPONENT}${CMAKE_STATIC_LIBRARY_SUFFIX} lib${COMPONENT}${CMAKE_STATIC_LIBRARY_SUFFIX}
        PATHS ${USD_ROOT}/lib
        NO_DEFAULT_PATH)

    # MSVC 15.4 ignores WHOLEARCHIVE flag with direct path and with unix slash.
    file(RELATIVE_PATH USD_${UPPERCOMPONENT}_LIBRARY ${CMAKE_CURRENT_BINARY_DIR} ${USD_${UPPERCOMPONENT}_LIBRARY})
    string(REPLACE "/" "\\\\" USD_${UPPERCOMPONENT}_LIBRARY ${USD_${UPPERCOMPONENT}_LIBRARY})

    list(APPEND USD_LIBRARIES -WHOLEARCHIVE:${USD_${UPPERCOMPONENT}_LIBRARY})
endforeach ()

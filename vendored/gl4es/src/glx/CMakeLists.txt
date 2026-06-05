add_library(glx OBJECT)

if(NOT NOX11)
    find_package(X11)
    find_path(GBM_INCLUDE_DIR gbm.h)
    find_path(DRM_INCLUDE_DIR drm.h PATH_SUFFIXES libdrm)
endif()

target_sources(glx PRIVATE
    hardext.c
    glx.c
    lookup.c
    rpi.c
    gbm.c
    streaming.c
    utils.c
)

# Optional Linux/X11 backend
if(UNIX AND NOT NO_LOADER AND NOT NOX11)
    target_compile_definitions(glx PRIVATE HAVE_GLX)
    if(NOT GBM_INCLUDE_DIR)
        target_compile_definitions(glx PRIVATE NO_GBM)
    endif()
else()
    target_compile_definitions(glx PRIVATE NO_GLX)
    target_compile_definitions(glx PRIVATE NO_GBM)
    target_compile_definitions(glx PRIVATE NOX11)
endif()

target_include_directories(glx PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../gl
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

if(X11_INCLUDE_DIR)
    target_include_directories(glx PRIVATE ${X11_INCLUDE_DIR})
endif()
if(GBM_INCLUDE_DIR)
    target_include_directories(glx PRIVATE ${GBM_INCLUDE_DIR})
endif()
if(DRM_INCLUDE_DIR)
    target_include_directories(glx PRIVATE ${DRM_INCLUDE_DIR})
endif()
if(AMIGAOS4)
    add_subdirectory(src/agl)
    target_sources(GL PRIVATE $<TARGET_OBJECTS:agl>)
endif()

cmake_minimum_required(VERSION 3.10)
project(GL LANGUAGES C)

option(NOX11 "Disable X11 GLX backend" OFF)
option(NO_LOADER "Disable loader" OFF)
option(AMIGAOS4 "Enable AmigaOS4 backend" OFF)
option(EGL_WRAPPER "Build EGL wrapper" OFF)
option(USE_ANDROID_LOG "Android logging" OFF)
option(USE_CLOCK "clock support" OFF)

if(ANDROID)
    set(NOX11 ON)
    set(USE_ANDROID_LOG ON)
    add_definitions(-DDEFAULT_ES=2)
    add_definitions(-DBCMHOST)
    add_definitions(-DNOX11)
    add_definitions(-DNO_GBM)
    add_definitions(-DUSE_ANDROID_LOG)
    add_definitions(-DSTATICLIB)
endif()

# ---------------- CORE GL ----------------
add_library(gl_core OBJECT)

target_sources(gl_core PRIVATE
    src/gl/arbconverter.c
    src/gl/arbgenerator.c
    src/gl/arbhelper.c
    src/gl/arbparser.c
    src/gl/array.c
    src/gl/blit.c
    src/gl/blend.c
    src/gl/buffers.c
    src/gl/build_info.c
    src/gl/debug.c
    src/gl/decompress.c
    src/gl/depth.c
    src/gl/directstate.c
    src/gl/drawing.c
    src/gl/enable.c
    src/gl/envvars.c
    src/gl/eval.c
    src/gl/face.c
    src/gl/fog.c
    src/gl/fpe.c
    src/gl/fpe_cache.c
    src/gl/fpe_shader.c
    src/gl/framebuffers.c
    src/gl/gl_lookup.c
    src/gl/getter.c
    src/gl/gl4es.c
    src/gl/glstate.c
    src/gl/hint.c
    src/gl/init.c
    src/gl/light.c
    src/gl/line.c
    src/gl/list.c
    src/gl/listdraw.c
    src/gl/listrl.c
    src/gl/loader.c
    src/gl/logs.c
    src/gl/matrix.c
    src/gl/matvec.c
    src/gl/oldprogram.c
    src/gl/pixel.c
    src/gl/planes.c
    src/gl/pointsprite.c
    src/gl/preproc.c
    src/gl/program.c
    src/gl/queries.c
    src/gl/raster.c
    src/gl/render.c
    src/gl/samplers.c
    src/gl/shader.c
    src/gl/shaderconv.c
    src/gl/shader_hacks.c
    src/gl/stack.c
    src/gl/stencil.c
    src/gl/string_utils.c
    src/gl/stubs.c
    src/gl/texenv.c
    src/gl/texgen.c
    src/gl/texture.c
    src/gl/texture_compressed.c
    src/gl/texture_params.c
    src/gl/texture_read.c
    src/gl/texture_3d.c
    src/gl/uniform.c
    src/gl/vertexattrib.c
    src/gl/wrap/gl4eswraps.c
    src/gl/wrap/gles.c
    src/gl/wrap/glstub.c
    src/gl/math/matheval.c
)

target_include_directories(gl_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(gl_core PRIVATE c_std_99)

add_subdirectory(src/glx)

if(NOT NOX11)
    find_package(X11)
endif()

add_library(GL STATIC $<TARGET_OBJECTS:gl_core> $<TARGET_OBJECTS:glx>)
target_link_libraries(GL PUBLIC gl_core ${CMAKE_DL_LIBS})
if(X11_FOUND)
    target_link_libraries(GL PUBLIC ${X11_LIBRARIES})
endif()

if(ANDROID)
    target_link_libraries(GL PUBLIC log)
endif()

if(EGL_WRAPPER)
    add_subdirectory(src/egl)
endif()

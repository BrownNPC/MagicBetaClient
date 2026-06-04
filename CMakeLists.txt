cmake_minimum_required(VERSION 3.17)

project(MagicBetaClient)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Debug)


# add sources
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS _build/transpiled/*.c)
if(ANDROID)
  add_library(${PROJECT_NAME} ${SOURCES}) # android needs shared library.
else()
  add_executable(${PROJECT_NAME} ${SOURCES})
endif()

#pretend to be OpenBSD for PSP
if (PSP)
  target_compile_definitions(${PROJECT_NAME} PRIVATE so_build_openbsd) 
endif()

target_include_directories(${PROJECT_NAME} PUBLIC _build/transpiled external/)
# Link with CURL
find_package(CURL REQUIRED)
# Link with SDL3
find_package(SDL3 REQUIRED)
# Link with OpenGL
find_package(OpenGL REQUIRED)
# Link with SDL3_mixer
find_package(SDL3_mixer REQUIRED)
if (NOT PSP)
add_subdirectory(vendored/gl4es)
endif()
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
add_subdirectory(vendored/SDL_mixer EXCLUDE_FROM_ALL)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    SDL3_mixer::SDL3_mixer
    CURL::libcurl
    GL
    m
)


# This is safe to set on all platforms. Otherwise your SDL app will
#  have a terminal window pop up with it on Windows.
set_property(TARGET ${PROJECT_NAME} PROPERTY WIN32_EXECUTABLE TRUE)

if(EMSCRIPTEN)
    set_property(TARGET ${PROJECT_NAME} PROPERTY SUFFIX ".html") # create html web page.
endif()

if (PSP)
  set(PSPDEV $ENV{PSPDEV})
  # make clangd happy
  file(GLOB GCC_INCLUDE_DIR "${PSPDEV}/lib/gcc/psp/*/include")
  target_compile_options(${PROJECT_NAME} PRIVATE
      -isystem ${GCC_INCLUDE_DIR}
  )

  create_pbp_file(
    TARGET ${PROJECT_NAME}
    TITLE change this in CMakeLists.txt
    ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png"
    BUILD_PRX=1
    ENC_PRX=1
  )
  # Copy assets at build time
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
  )
endif()

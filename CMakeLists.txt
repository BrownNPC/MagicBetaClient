cmake_minimum_required(VERSION 3.17)


project(MagicBetaClient LANGUAGES C)


set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Debug)

option(USE_VENDORED_SDL3 "Use vendored SDL3" ON)
option(USE_VENDORED_MIXER "Use vendored SDL_Mixer" ON)


# disable formats we don't use to make the build faster and smaller. Also some of these don't work on all platforms so you'll need to do some experimentation.
set(SDLMIXER_MIDI_NATIVE OFF)     
set(SDLMIXER_GME OFF)
set(SDLMIXER_WAVPACK OFF)     
set(SDLMIXER_MOD OFF)
set(SDLMIXER_MOD_XMP OFF)
set(SDLMIXER_OPUS OFF)
set(SDLMIXER_MP3_MPG123 OFF)
set(SDLMIXER_VORBIS_VORBISFILE OFF)
set(SDLMIXER_FLAC_LIBFLAC OFF)
set(SDLMIXER_VENDORED ON)   # tell SDL_mixer to build its own dependencies
# add sources
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS _build/transpiled/*.c)
if(ANDROID)
  add_library(${PROJECT_NAME} SHARED ${SOURCES}) # android needs shared library.
  set_target_properties(MagicBetaClient PROPERTIES OUTPUT_NAME main)
else()
  add_executable(${PROJECT_NAME} ${SOURCES})
endif()

target_include_directories(${PROJECT_NAME} PUBLIC _build/transpiled src/)

#pretend to be OpenBSD for PSP
if (PSP)
  target_compile_definitions(${PROJECT_NAME} PRIVATE so_build_openbsd) 
endif()

find_package(OpenGL REQUIRED)

if (USE_VENDORED_SDL3)
  add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
else()
  find_package(SDL3 REQUIRED)
endif()

if (USE_VENDORED_MIXER)
  add_subdirectory(vendored/SDL_mixer EXCLUDE_FROM_ALL)
else()
  find_package(SDL3_mixer REQUIRED)
endif()

if(ANDROID)
    find_package(curl REQUIRED CONFIG)
    set(CURL_TARGET curl::curl_static c++_shared) # need this or boringSSL will not compile :D
else()
    find_package(CURL REQUIRED)
    set(CURL_TARGET CURL::libcurl )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    SDL3_mixer::SDL3_mixer
    ${CURL_TARGET}
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

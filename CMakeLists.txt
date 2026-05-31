cmake_minimum_required(VERSION 3.17)

project(MagicBetaClient)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# add sources
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS _build/transpiled/*.c)
add_executable(${PROJECT_NAME} ${SOURCES})
if (PSP)
  # pretend to be openbsd for PSP.
  target_compile_definitions(${PROJECT_NAME} PRIVATE so_build_openbsd)
endif()
target_include_directories(${PROJECT_NAME} PUBLIC _build/transpiled external/)

set(CMAKE_BUILD_TYPE Debug)
# Link with CURL
find_package(CURL REQUIRED)
# Link with SDL3
find_package(SDL3 REQUIRED)
# Link with OpenGL
find_package(OpenGL REQUIRED)


target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    CURL::libcurl
    OpenGL::GL
    m
)
if (!PSP)
# target_compile_options(${PROJECT_NAME} PRIVATE
#     -g
#     -O1
#     -fno-omit-frame-pointer
#     -fsanitize=address
# )

# target_link_options(${PROJECT_NAME} PRIVATE
#     -fsanitize=address
# )
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

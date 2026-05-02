cmake_minimum_required(VERSION 3.17)

project(MagicBetaClient)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# add sources
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.c)
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC src/)

set(CMAKE_BUILD_TYPE Debug)
# Link with SDL3
find_package(SDL3 REQUIRED)
# Link with CURL
find_package(CURL REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    CURL::libcurl
    m
)

if (PSP)
  # PSP SDK
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
endif()

add_library(agl OBJECT)

target_sources(agl PRIVATE
    agl.c
    amigaos.c
    lookup.c
)

target_include_directories(agl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../glx
)

add_library(egl_wrapper STATIC
    egl.c
    lookup.c
)

target_include_directories(egl_wrapper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../gl
)

target_link_libraries(egl_wrapper PRIVATE GL)

find_package(boringssl REQUIRED CONFIG)

if(NOT TARGET curl::curl_static)
add_library(curl::curl_static STATIC IMPORTED)
set_target_properties(curl::curl_static PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/vendored/curl-aar/prefab/modules/curl_static/libs/android.armeabi-v7a/libcurl_static.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendored/curl-aar/prefab/modules/curl_static/include"
    INTERFACE_LINK_LIBRARIES "-lz;curl::nghttp2_static;curl::nghttp3_static;curl::ngtcp2_static;boringssl::ssl_static"
)
endif()

if(NOT TARGET curl::nghttp2_static)
add_library(curl::nghttp2_static STATIC IMPORTED)
set_target_properties(curl::nghttp2_static PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/vendored/curl-aar/prefab/modules/nghttp2_static/libs/android.armeabi-v7a/libnghttp2_static.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendored/curl-aar/prefab/modules/nghttp2_static/include"
    INTERFACE_LINK_LIBRARIES ""
)
endif()

if(NOT TARGET curl::nghttp3_static)
add_library(curl::nghttp3_static STATIC IMPORTED)
set_target_properties(curl::nghttp3_static PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/vendored/curl-aar/prefab/modules/nghttp3_static/libs/android.armeabi-v7a/libnghttp3_static.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendored/curl-aar/prefab/modules/nghttp3_static/include"
    INTERFACE_LINK_LIBRARIES ""
)
endif()

if(NOT TARGET curl::ngtcp2_crypto_static)
add_library(curl::ngtcp2_crypto_static STATIC IMPORTED)
set_target_properties(curl::ngtcp2_crypto_static PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/vendored/curl-aar/prefab/modules/ngtcp2_crypto_static/libs/android.armeabi-v7a/libngtcp2_crypto_static.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendored/curl-aar/prefab/modules/ngtcp2_crypto_static/include"
    INTERFACE_LINK_LIBRARIES "boringssl::ssl_static"
)
endif()

if(NOT TARGET curl::ngtcp2_static)
add_library(curl::ngtcp2_static STATIC IMPORTED)
set_target_properties(curl::ngtcp2_static PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/vendored/curl-aar/prefab/modules/ngtcp2_static/libs/android.armeabi-v7a/libngtcp2_static.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendored/curl-aar/prefab/modules/ngtcp2_static/include"
    INTERFACE_LINK_LIBRARIES "curl::ngtcp2_crypto_static"
)
endif()


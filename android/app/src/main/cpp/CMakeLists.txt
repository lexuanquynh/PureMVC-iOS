cmake_minimum_required(VERSION 3.22)
project(puremvc_jni LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static OpenSSL 3.x built for the NDK by android/openssl/build-openssl.sh
# (httplib 0.21 requires OpenSSL >= 3.0; Google's prefab only ships 1.1.1).
# Run that script once before building.
set(OPENSSL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../openssl/prebuilt/${ANDROID_ABI})
if(NOT EXISTS ${OPENSSL_ROOT}/lib/libcrypto.a)
    message(FATAL_ERROR "OpenSSL for ${ANDROID_ABI} not found at ${OPENSSL_ROOT}. "
        "Run android/openssl/build-openssl.sh first.")
endif()
add_library(openssl_crypto STATIC IMPORTED)
set_target_properties(openssl_crypto PROPERTIES
    IMPORTED_LOCATION ${OPENSSL_ROOT}/lib/libcrypto.a
    INTERFACE_INCLUDE_DIRECTORIES ${OPENSSL_ROOT}/include)
add_library(openssl_ssl STATIC IMPORTED)
set_target_properties(openssl_ssl PROPERTIES
    IMPORTED_LOCATION ${OPENSSL_ROOT}/lib/libssl.a
    INTERFACE_INCLUDE_DIRECTORIES ${OPENSSL_ROOT}/include)

# Build the shared C++ Core WITH the httplib HTTP client, linking static OpenSSL.
set(PUREMVC_CORE_WITH_HTTPLIB ON CACHE BOOL "" FORCE)
set(PUREMVC_CORE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(PUREMVC_OPENSSL_TARGETS openssl_ssl openssl_crypto)

# Core lives outside android/, so add_subdirectory needs an explicit binary dir.
add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Core
    ${CMAKE_BINARY_DIR}/puremvc_core
)

add_library(puremvc_jni SHARED jni_bridge.cpp)
target_link_libraries(puremvc_jni PRIVATE puremvc_core)

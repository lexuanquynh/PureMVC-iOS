cmake_minimum_required(VERSION 3.22)
project(puremvc_jni LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build the shared C++ Core without the httplib/OpenSSL client for this first
# bring-up (Domain + non-network Infrastructure only). Native OpenSSL for the NDK
# is a later slice.
set(PUREMVC_CORE_WITH_HTTPLIB OFF CACHE BOOL "" FORCE)
set(PUREMVC_CORE_BUILD_TESTS OFF CACHE BOOL "" FORCE)

# Core lives outside android/, so add_subdirectory needs an explicit binary dir.
add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Core
    ${CMAKE_BINARY_DIR}/puremvc_core
)

add_library(puremvc_jni SHARED jni_bridge.cpp)
target_link_libraries(puremvc_jni PRIVATE puremvc_core)

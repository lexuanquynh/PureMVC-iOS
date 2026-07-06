cmake_minimum_required(VERSION 3.14)
project(PureMVCCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build the httplib-based HTTP client (requires OpenSSL). Turn OFF on platforms
# that provide HTTP another way or lack native OpenSSL (e.g. first Android
# bring-up); the rest of Core still builds.
option(PUREMVC_CORE_WITH_HTTPLIB "Build the httplib HTTP client (needs OpenSSL)" ON)
option(PUREMVC_CORE_BUILD_TESTS "Build PureMVC core unit tests" ${PROJECT_IS_TOP_LEVEL})

# ----------------------------------------------------------------------------
# Core library — domain + infrastructure. Domain has zero third-party deps;
# infrastructure uses the vendored header-only libs under ThirdParty/ (so no
# network fetch is needed — important for the Android NDK build).
# ----------------------------------------------------------------------------
set(PUREMVC_CORE_SOURCES
    Domain/UseCases/LoginUseCase.cpp
    Infrastructure/Auth/AuthRepository.cpp
    Infrastructure/Security/SecureTokenStore.cpp
)
if(PUREMVC_CORE_WITH_HTTPLIB)
    list(APPEND PUREMVC_CORE_SOURCES Infrastructure/Http/HttplibHttpClient.cpp)
endif()

# NOTE: Infrastructure/Security/KeychainSecureStore.mm is Apple-only (ObjC++ +
# Security.framework) and is built by the iOS bridge target, not here.
add_library(puremvc_core ${PUREMVC_CORE_SOURCES})
target_include_directories(puremvc_core
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty        # nlohmann/json.hpp
)

if(PUREMVC_CORE_WITH_HTTPLIB)
    target_compile_definitions(puremvc_core PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    if(PUREMVC_OPENSSL_TARGETS)
        # Caller provides OpenSSL (e.g. Android via the NDK prefab package). Use
        # the vendored httplib header directly.
        target_include_directories(puremvc_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/httplib)
        target_link_libraries(puremvc_core PRIVATE ${PUREMVC_OPENSSL_TARGETS})
    else()
        # Host: cpp-httplib via FetchContent brings its own OpenSSL detection.
        # We don't need compression; disabling the optional deps keeps configure
        # portable (e.g. Linux CI without zstd/brotli).
        set(HTTPLIB_USE_ZLIB_IF_AVAILABLE   OFF CACHE BOOL "" FORCE)
        set(HTTPLIB_USE_ZSTD_IF_AVAILABLE   OFF CACHE BOOL "" FORCE)
        set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
        include(FetchContent)
        FetchContent_Declare(
            httplib
            GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
            GIT_TAG        v0.21.0
        )
        FetchContent_MakeAvailable(httplib)
        target_link_libraries(puremvc_core PRIVATE httplib::httplib)
    endif()
endif()

# ----------------------------------------------------------------------------
# Tests (host-only). Default ON when this is the top-level project.
# ----------------------------------------------------------------------------
if(PUREMVC_CORE_BUILD_TESTS)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    set(PUREMVC_TEST_SOURCES
        tests/LoginUseCaseTests.cpp
        tests/AuthRepositoryTests.cpp
        tests/ExecutorTests.cpp
        tests/SecureTokenStoreTests.cpp
        tests/Base64Tests.cpp
        tests/CertificatePinnerTests.cpp
        tests/HttpClientConfigTests.cpp
    )
    if(PUREMVC_CORE_WITH_HTTPLIB)
        list(APPEND PUREMVC_TEST_SOURCES tests/HttplibHttpClientTests.cpp)
    endif()

    add_executable(core_tests ${PUREMVC_TEST_SOURCES})
    target_include_directories(core_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}          # for "Domain/..."
        ${CMAKE_CURRENT_SOURCE_DIR}/tests    # for "Mocks/..."
        ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty
    )
    target_link_libraries(core_tests PRIVATE puremvc_core GTest::gtest_main)
    if(PUREMVC_CORE_WITH_HTTPLIB)
        target_link_libraries(core_tests PRIVATE httplib::httplib)
    endif()

    include(GoogleTest)
    gtest_discover_tests(core_tests)
endif()

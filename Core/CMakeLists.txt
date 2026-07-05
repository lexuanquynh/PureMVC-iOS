cmake_minimum_required(VERSION 3.14)
project(PureMVCCore LANGUAGES CXX)

# Keep the core C++11-compatible so the same sources also build into the iOS
# target (which uses gnu++11). Tests may rely on the host toolchain default.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ----------------------------------------------------------------------------
# Core library — pure domain/application code, zero platform dependencies.
# ----------------------------------------------------------------------------
add_library(puremvc_core
    Domain/UseCases/LoginUseCase.cpp
)
target_include_directories(puremvc_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Tests (host-only). Enable with -DPUREMVC_CORE_BUILD_TESTS=ON (default ON when
# this is the top-level project).
# ----------------------------------------------------------------------------
option(PUREMVC_CORE_BUILD_TESTS "Build PureMVC core unit tests" ${PROJECT_IS_TOP_LEVEL})

if(PUREMVC_CORE_BUILD_TESTS)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    # Don't override the parent project's compiler/linker settings on Windows.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(core_tests
        tests/LoginUseCaseTests.cpp
    )
    target_include_directories(core_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}        # for "Domain/..."
        ${CMAKE_CURRENT_SOURCE_DIR}/tests  # for "Mocks/..."
    )
    target_link_libraries(core_tests PRIVATE puremvc_core GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(core_tests)
endif()

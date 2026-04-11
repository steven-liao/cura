cmake_minimum_required(VERSION 3.20)

project(cura_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch Catch2
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(Catch2)

# Test sources
set(TEST_SOURCES
    test_scanner.cpp
    test_hasher.cpp
    test_clusterer.cpp
    test_fileops.cpp
    test_image.cpp
)

# Core library sources (for testing)
set(CURA_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/core/cura_scanner.cpp
    ${CMAKE_SOURCE_DIR}/src/core/cura_hasher.cpp
    ${CMAKE_SOURCE_DIR}/src/core/cura_clusterer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/cura_fileops.cpp
    ${CMAKE_SOURCE_DIR}/src/core/cura_image.cpp
)

set(CURA_THREADING_SOURCES
    ${CMAKE_SOURCE_DIR}/src/threading/cura_threadpool.cpp
)

# Create test executable
add_executable(cura_tests
    ${TEST_SOURCES}
    ${CURA_CORE_SOURCES}
    ${CURA_THREADING_SOURCES}
)

# Link with Catch2
target_link_libraries(cura_tests PRIVATE Catch2::Catch2WithMain)

# Include directories
target_include_directories(cura_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/threading
    ${CMAKE_SOURCE_DIR}/third_party
)

# Discover tests
include(CTest)
include(Catch)
catch_discover_tests(cura_tests)
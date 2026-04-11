cmake_minimum_required(VERSION 3.20)

project(cura VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(CURA_BUILD_TESTS "Build unit tests" ON)

# Try to find Slint first, then fall back to FetchContent
find_package(Slint QUIET)

if(NOT Slint_FOUND)
    # Fetch Slint from source (no installer needed)
    # Disable interpreter feature to avoid Rust dependency
    include(FetchContent)
    FetchContent_Declare(Slint
        GIT_REPOSITORY https://github.com/slint-ui/slint.git
        GIT_TAG v1.5.0
        SOURCE_SUBDIR api/cpp
    )

    # Disable features that require Rust
    set(SLINT_FEATURE_INTERPRETER OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(Slint)
endif()

# Third-party headers
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# Source files
set(CURA_CORE_SOURCES
    src/core/cura_scanner.cpp
    src/core/cura_hasher.cpp
    src/core/cura_clusterer.cpp
    src/core/cura_fileops.cpp
    src/core/cura_image.cpp
)

set(CURA_THREADING_SOURCES
    src/threading/cura_threadpool.cpp
)

set(CURA_UI_SOURCES
    src/ui/cura_app.cpp
)

set(CURA_SOURCES
    src/main.cpp
    ${CURA_CORE_SOURCES}
    ${CURA_THREADING_SOURCES}
    ${CURA_UI_SOURCES}
)

# Slint UI files - only compile the main file, others are imported
set(CURA_SLINT_SOURCES
    ui/cura_main.slint
)

# Main executable
add_executable(cura ${CURA_SOURCES})

# Link Slint
target_link_libraries(cura PRIVATE Slint::Slint)

# Slint compile definitions
slint_target_sources(cura ${CURA_SLINT_SOURCES})

# Platform-specific settings
if(WIN32)
    target_compile_definitions(cura PRIVATE _WIN32_WINNT=0x0A00)
endif()

if(APPLE)
    set_target_properties(cura PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.cura.app"
        MACOSX_BUNDLE_BUNDLE_NAME "Cura"
    )
endif()

# Tests
if(CURA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Install rules
install(TARGETS cura RUNTIME DESTINATION bin)
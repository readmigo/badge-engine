cmake_minimum_required(VERSION 3.22)
project(badge_engine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BADGE_ENGINE_BUILD_TESTS "Build tests" ON)
option(BADGE_ENGINE_BUILD_EXAMPLES "Build examples" OFF)
option(BADGE_ENGINE_USE_FILAMENT "Link with Filament SDK for real rendering" OFF)

# --- Sources ---
set(BADGE_ENGINE_SOURCES
    src/engine.cpp
    src/c_api.cpp
    src/material.cpp
    src/lighting.cpp
    src/interaction.cpp
    src/animator.cpp
    src/ceremony.cpp
    src/config.cpp
    src/asset_loader.cpp
)

# Add renderer/scene only when Filament is available
if(BADGE_ENGINE_USE_FILAMENT)
    list(APPEND BADGE_ENGINE_SOURCES
        src/renderer.cpp
        src/scene.cpp
    )
endif()

# --- nlohmann/json (header-only) ---
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann_json)

# --- Library ---
add_library(badge_engine STATIC ${BADGE_ENGINE_SOURCES})
target_include_directories(badge_engine
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(badge_engine PRIVATE nlohmann_json)

# --- Filament SDK ---
if(BADGE_ENGINE_USE_FILAMENT)
    set(FILAMENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/filament)

    if(NOT EXISTS ${FILAMENT_DIR}/include/filament/Engine.h)
        message(FATAL_ERROR
            "Filament SDK not found at ${FILAMENT_DIR}.\n"
            "Run: ./scripts/setup-filament.sh [platform]\n"
            "Or disable Filament: -DBADGE_ENGINE_USE_FILAMENT=OFF"
        )
    endif()

    # Determine library architecture directory
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(FILAMENT_LIB_DIR ${FILAMENT_DIR}/lib/universal)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(FILAMENT_LIB_DIR ${FILAMENT_DIR}/lib/arm64)
    else()
        set(FILAMENT_LIB_DIR ${FILAMENT_DIR}/lib/x86_64)
    endif()

    # Create imported targets for Filament libraries
    set(FILAMENT_LIBS
        filament
        backend
        filabridge
        filaflat
        utils
        geometry
        gltfio
        gltfio_core
        image
        ktxreader
        stb
        uberzlib
        uberarchive
        smol-v
        dracodec
        meshoptimizer
        basis_transcoder
        ibl
        shaders
        matdbg
        zstd
    )

    # Platform-specific graphics backend
    if(APPLE)
        list(APPEND FILAMENT_LIBS bluegl bluevk)
        find_library(METAL_FRAMEWORK Metal)
        find_library(COREGRAPHICS_FRAMEWORK CoreGraphics)
        find_library(QUARTZ_FRAMEWORK QuartzCore)
        find_library(COCOA_FRAMEWORK Cocoa)
        find_library(IOKIT_FRAMEWORK IOKit)
        find_library(FOUNDATION_FRAMEWORK Foundation)
        set(PLATFORM_LIBS
            ${METAL_FRAMEWORK}
            ${COREGRAPHICS_FRAMEWORK}
            ${QUARTZ_FRAMEWORK}
            ${COCOA_FRAMEWORK}
            ${IOKIT_FRAMEWORK}
            ${FOUNDATION_FRAMEWORK}
        )
    else()
        list(APPEND FILAMENT_LIBS bluegl bluevk)
        set(PLATFORM_LIBS)
    endif()

    # Add Filament include directories
    target_include_directories(badge_engine
        PRIVATE ${FILAMENT_DIR}/include
    )

    # Link Filament static libraries
    foreach(LIB ${FILAMENT_LIBS})
        if(EXISTS ${FILAMENT_LIB_DIR}/lib${LIB}.a)
            target_link_libraries(badge_engine PRIVATE ${FILAMENT_LIB_DIR}/lib${LIB}.a)
        endif()
    endforeach()

    # Link platform frameworks
    if(PLATFORM_LIBS)
        target_link_libraries(badge_engine PRIVATE ${PLATFORM_LIBS})
    endif()

    target_compile_definitions(badge_engine PRIVATE BADGE_ENGINE_HAS_FILAMENT=1)
endif()

# --- Shared library (for JNI / dynamic linking) ---
add_library(badge_engine_shared SHARED ${BADGE_ENGINE_SOURCES})
target_include_directories(badge_engine_shared
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(badge_engine_shared PRIVATE nlohmann_json)

if(BADGE_ENGINE_USE_FILAMENT)
    target_include_directories(badge_engine_shared
        PRIVATE ${FILAMENT_DIR}/include
    )
    foreach(LIB ${FILAMENT_LIBS})
        if(EXISTS ${FILAMENT_LIB_DIR}/lib${LIB}.a)
            target_link_libraries(badge_engine_shared PRIVATE ${FILAMENT_LIB_DIR}/lib${LIB}.a)
        endif()
    endforeach()
    if(PLATFORM_LIBS)
        target_link_libraries(badge_engine_shared PRIVATE ${PLATFORM_LIBS})
    endif()
    target_compile_definitions(badge_engine_shared PRIVATE BADGE_ENGINE_HAS_FILAMENT=1)
endif()

# --- Tests ---
if(BADGE_ENGINE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# --- Examples ---
if(BADGE_ENGINE_BUILD_EXAMPLES AND BADGE_ENGINE_USE_FILAMENT)
    add_subdirectory(examples/hello_badge)
endif()

cmake_minimum_required(VERSION 3.22)
project(badge_engine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BADGE_ENGINE_BUILD_TESTS "Build tests" ON)
option(BADGE_ENGINE_BUILD_EXAMPLES "Build examples" OFF)

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

# --- Shared library (for JNI / dynamic linking) ---
add_library(badge_engine_shared SHARED ${BADGE_ENGINE_SOURCES})
target_include_directories(badge_engine_shared
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(badge_engine_shared PRIVATE nlohmann_json)

# --- Tests ---
if(BADGE_ENGINE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

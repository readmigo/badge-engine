find_package(SDL2 REQUIRED)

add_executable(hello_badge main.cpp)

target_link_libraries(hello_badge PRIVATE
    badge_engine
    SDL2::SDL2
    SDL2::SDL2main
)

target_include_directories(hello_badge PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# On macOS, need to link Cocoa for SDL2 Metal support
if(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa)
    target_link_libraries(hello_badge PRIVATE ${COCOA_FRAMEWORK})
endif()

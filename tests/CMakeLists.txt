find_package(GTest REQUIRED)

# Engine lifecycle test
add_executable(test_engine test_engine.cpp)
target_link_libraries(test_engine PRIVATE badge_engine GTest::gtest_main)
target_include_directories(test_engine PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_engine COMMAND test_engine)

# Material system test
add_executable(test_material test_material.cpp)
target_link_libraries(test_material PRIVATE badge_engine GTest::gtest_main)
target_include_directories(test_material PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_material COMMAND test_material WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Lighting system test
add_executable(test_lighting test_lighting.cpp)
target_link_libraries(test_lighting PRIVATE badge_engine GTest::gtest_main)
target_include_directories(test_lighting PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_lighting COMMAND test_lighting WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Interaction handler test
add_executable(test_interaction test_interaction.cpp)
target_link_libraries(test_interaction PRIVATE badge_engine GTest::gtest_main)
target_include_directories(test_interaction PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_interaction COMMAND test_interaction)

# Animator test
add_executable(test_animator test_animator.cpp)
target_link_libraries(test_animator PRIVATE badge_engine GTest::gtest_main)
target_include_directories(test_animator PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_animator COMMAND test_animator)

# Ceremony controller test
add_executable(test_ceremony test_ceremony.cpp)
target_link_libraries(test_ceremony PRIVATE badge_engine GTest::gtest_main)
target_include_directories(test_ceremony PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_ceremony COMMAND test_ceremony WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Config parser test
add_executable(test_config test_config.cpp)
target_link_libraries(test_config PRIVATE badge_engine GTest::gtest_main)
target_include_directories(test_config PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_config COMMAND test_config WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Asset loader test
add_executable(test_asset_loader test_asset_loader.cpp)
target_link_libraries(test_asset_loader PRIVATE badge_engine GTest::gtest_main)
target_include_directories(test_asset_loader PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_asset_loader COMMAND test_asset_loader WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

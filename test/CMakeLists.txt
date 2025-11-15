cmake_minimum_required(VERSION 3.14)
project(inkplate-dashboard-tests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define UNIT_TEST for conditional compilation
add_compile_definitions(UNIT_TEST)

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Include directories
include_directories(
  ${CMAKE_SOURCE_DIR}/mocks
  ${CMAKE_SOURCE_DIR}/../common/src
)

# Add test executables
add_executable(
  decision_tests
  unit/test_decision_functions.cpp
  ../common/src/modes/decision_logic.cpp  # Real production code!
  ../common/src/config_logic.cpp          # Real production code!
  mocks/config_manager.cpp                # Mock that delegates to config_logic
)

add_executable(
  battery_tests
  unit/test_battery_logic.cpp
  ../common/src/battery_logic.cpp  # Real production code!
)

add_executable(
  sleep_tests
  unit/test_sleep_logic.cpp
  ../common/src/sleep_logic.cpp  # Real production code!
)

add_executable(
  config_tests
  unit/test_config_logic.cpp
  ../common/src/config_logic.cpp  # Real production code!
)

# Link Google Test
target_link_libraries(
  decision_tests
  GTest::gtest_main
)

target_link_libraries(
  battery_tests
  GTest::gtest_main
)

target_link_libraries(
  sleep_tests
  GTest::gtest_main
)

target_link_libraries(
  config_tests
  GTest::gtest_main
)

# Discover tests
include(GoogleTest)
gtest_discover_tests(decision_tests)
gtest_discover_tests(battery_tests)
gtest_discover_tests(sleep_tests)
gtest_discover_tests(config_tests)

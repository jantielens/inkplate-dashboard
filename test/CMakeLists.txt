cmake_minimum_required(VERSION 3.14)
project(inkplate-dashboard-tests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define UNIT_TEST for conditional compilation
add_compile_definitions(UNIT_TEST)

# =============================================================================
# Code Coverage Support
# =============================================================================

option(ENABLE_COVERAGE "Enable code coverage" OFF)

if(ENABLE_COVERAGE)
  if(MSVC)
    # MSVC: Use /PROFILE flag for coverage (requires external tools like OpenCppCoverage)
    message(STATUS "Code coverage enabled (MSVC) - use OpenCppCoverage for reports")
    add_compile_options(/DEBUG:FULL)
  else()
    # GCC/Clang: Use gcov/llvm-cov
    message(STATUS "Code coverage enabled (GCC/Clang)")
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
  endif()
endif()

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
  ${CMAKE_SOURCE_DIR}/integration  # Integration test helpers
)

# =============================================================================
# Unit Test Executables
# =============================================================================

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

# =============================================================================
# Integration Test Executables
# =============================================================================

add_executable(
  integration_tests
  integration/test_normal_mode_scenarios.cpp
  ../common/src/modes/decision_logic.cpp  # Real production code!
  ../common/src/config_logic.cpp          # Real production code!
  ../common/src/sleep_logic.cpp           # Real production code!
  mocks/config_manager.cpp                # Mock that delegates to config_logic
)

# =============================================================================
# Link Google Test to All Executables
# =============================================================================

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

target_link_libraries(
  integration_tests
  GTest::gtest_main
)

# =============================================================================
# Discover Tests
# =============================================================================

# Discover tests
include(GoogleTest)
gtest_discover_tests(decision_tests)
gtest_discover_tests(battery_tests)
gtest_discover_tests(sleep_tests)
gtest_discover_tests(config_tests)
gtest_discover_tests(integration_tests)

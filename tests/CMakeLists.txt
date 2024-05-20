message(STATUS "CppCmd: Building Tests")
include(GoogleTest)

enable_testing()

file(GLOB_RECURSE TEST_SOURCES *.cpp)

message(STATUS "CppCmd: Test files - ${TEST_SOURCES}")

add_executable(cppcmd_tests ${TEST_SOURCES})
target_include_directories(cppcmd_tests PRIVATE include)

target_link_libraries(cppcmd_tests PRIVATE GTest::gtest GTest::gtest_main)
target_link_libraries(cppcmd_tests PRIVATE cppcmd_lib)

gtest_discover_tests(cppcmd_tests TEST_PREFIX cppcmd:)
cmake_minimum_required(VERSION 3.10)
project(cppcmd)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

option(CPPCMD_BUILD_TESTS "Build cppcmd tests" OFF)
option(CPPCMD_TESTS_GTEST_USE_SHARED_CRT "Use shared CRT for testing cppcmd with GoogleTest" ON)
option(CPPCMD_MSVC_USE_DYNAMIC_RUNTIME "Use dynamic runtime library with MSVC" ON)

add_executable(t test.cpp)

target_include_directories(t PRIVATE ${CMAKE_SOURCE_DIR}/deps/reflectcpp/include)

add_library(cppcmd_lib INTERFACE)
target_include_directories(cppcmd_lib INTERFACE include)
target_include_directories(cppcmd_lib INTERFACE deps/reflectcpp/include)

if (CPPCMD_BUILD_TESTS)
    if(CPPCMD_TESTS_GTEST_USE_SHARED_CRT)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    else()
        set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
    endif()

    include(cmake/get_cpm.cmake)
    include(cmake/install_gtest.cmake)

    include(CTest)
    add_subdirectory(tests)

    if(MSVC)
        add_compile_options(/utf-8)

        if(CPPCMD_MSVC_USE_DYNAMIC_RUNTIME)
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "MSVC Runtime Library" FORCE)
        else()
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC Runtime Library" FORCE)
        endif()
    endif()

    if (MINGW)
        target_compile_options(cppcmd_tests PUBLIC "-Wa,-mbig-obj")
    endif()
endif()
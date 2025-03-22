cmake_minimum_required(VERSION 3.15)
project(sslog VERSION 1.0.0 DESCRIPTION "Speedy Structured Logger")

# Options
option(SSLOG_BUILD_TESTSSLOG "Build the C++ example program" ON)
option(SSLOG_BUILD_PYTHON_READER "Build the python reader interface" OFF)

# Policies
cmake_policy(SET CMP0009 NEW) # For GLOB_RECURSE
cmake_policy(SET CMP0072 NEW) # For OpenGL (Linux)

# Use ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif(CCACHE_PROGRAM)

# Add the ASAN target
set(CMAKE_CXX_FLAGS_ASAN "-Wall -fsanitize=address -fno-omit-frame-pointer -g"
    CACHE STRING "Flags used by the C++ compiler during asan builds." FORCE)
set(CMAKE_C_FLAGS_ASAN "-Wall -fsanitize=address -fno-omit-frame-pointer -g"
    CACHE STRING "Flags used by the C compiler during asan builds." FORCE)
set(CMAKE_EXE_LINKER_FLAGS_ASAN "-fsanitize=address"
    CACHE STRING "Flags used for linking binaries during asan builds." FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_ASAN "-fsanitize=address"
    CACHE STRING "Flags used by the shared libraries linker during asan builds." FORCE)
mark_as_advanced(CMAKE_CXX_FLAGS_ASAN CMAKE_C_FLAGS_ASAN CMAKE_EXE_LINKER_FLAGS_ASAN
                 CMAKE_SHARED_LINKER_FLAGS_ASAN)
set(CMAKE_BUILD_TYPE
    "${CMAKE_BUILD_TYPE}"
    CACHE
      STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel Asan Tsan."
      FORCE)

# Add the TSAN target
set(CMAKE_CXX_FLAGS_TSAN "-Wall -fsanitize=thread -fno-omit-frame-pointer -g -O2"
    CACHE STRING "Flags used by the C++ compiler during tsan builds." FORCE)
set(CMAKE_C_FLAGS_TSAN "-Wall -fsanitize=thread -fno-omit-frame-pointer -g -O2"
    CACHE STRING "Flags used by the C compiler during tsan builds." FORCE)
set(CMAKE_EXE_LINKER_FLAGS_TSAN "-fsanitize=thread"
    CACHE STRING "Flags used for linking binaries during tsan builds." FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_TSAN "-fsanitize=thread"
    CACHE STRING "Flags used by the shared libraries linker during tsan builds." FORCE)
mark_as_advanced(CMAKE_CXX_FLAGS_TSAN CMAKE_C_FLAGS_TSAN CMAKE_EXE_LINKER_FLAGS_TSAN
                 CMAKE_SHARED_LINKER_FLAGS_TSAN)
set(CMAKE_BUILD_TYPE
    "${CMAKE_BUILD_TYPE}"
    CACHE
      STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel Asan Tsan."
      FORCE)

# Built type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Type of build" FORCE)
endif()
message(
  "Build type: ${CMAKE_BUILD_TYPE}   (change with -DCMAKE_BUILD_TYPE=<Debug|Release|RelWithDebInfo|MinSizeRel|Asan|Tsan>)"
)
if(CMAKE_BUILD_TYPE STREQUAL "Tsan")
  set(CUSTOM_FLAGS "-DSSLOG_STACKTRACE=0")
endif()

# Store output in an easy location
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(POLICY CMP0094) # https://cmake.org/cmake/help/latest/policy/CMP0094.html
  cmake_policy(SET CMP0094 NEW) # FindPython should return the first matching Python
endif()

# needed on GitHub Actions CI: actions/setup-python does not touch registry/frameworks on
# Windows/macOS this mirrors PythonInterp behavior which did not consult registry/frameworks first
if(NOT DEFINED Python_FIND_REGISTRY)
  set(Python_FIND_REGISTRY "LAST")
endif()
if(NOT DEFINED Python_FIND_FRAMEWORK)
  set(Python_FIND_FRAMEWORK "LAST")
endif()

set(CMAKE_INSTALL_PREFIX "$ENV{HOME}")

# System flags
# ============

# Register our package finders
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

# Multithreading is used (both client and server applications)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# projects
# ========

# Build the logging library
add_library(sslog INTERFACE)
target_include_directories(sslog INTERFACE ./)

# Build the reader library
add_subdirectory(tools/sslogread)

# Build the sscat tool
add_subdirectory(tools/sscat)

# Build the python reader library
if(SSLOG_BUILD_PYTHON_READER)
  add_subdirectory(python)
endif()

# Build the tests
if(SSLOG_BUILD_TESTSSLOG)
  add_subdirectory(test/testsslog)
endif()

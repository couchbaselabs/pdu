cmake_minimum_required(VERSION 3.17)

# Set CMake generator and make program if not already set
if(NOT DEFINED CMAKE_GENERATOR)
    set(CMAKE_GENERATOR "Ninja" CACHE STRING "CMake generator" FORCE)
endif()
if(NOT DEFINED CMAKE_MAKE_PROGRAM)
    set(CMAKE_MAKE_PROGRAM "/usr/bin/ninja" CACHE FILEPATH "CMake make program" FORCE)
endif()

# Set up vcpkg before project()
if(NOT CMAKE_TOOLCHAIN_FILE)
    if (NOT DEFINED "$ENV{VCPKG_ROOT}")
        message(WARNING "VCPKG_ROOT env var not found, install vcpkg and set the env var")
    endif()
    set(vcpkg "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    if (NOT EXISTS "${vcpkg}")
        set(vcpkg "$ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    endif()
    if (EXISTS "${vcpkg}")
        set(CMAKE_TOOLCHAIN_FILE "${vcpkg}" CACHE FILEPATH "CMake toolchain file" FORCE)
        message(STATUS "vcpkg toolchain found: ${CMAKE_TOOLCHAIN_FILE}")
    else()
        message(STATUS "vcpkg toolchain NOT found")
    endif()
endif()

# Set compiler if not already set
if(NOT DEFINED CMAKE_C_COMPILER)
    if(APPLE)
        set(CMAKE_C_COMPILER "/usr/bin/clang" CACHE STRING "C compiler" FORCE)
        set(CMAKE_CXX_COMPILER "/usr/bin/clang++" CACHE STRING "C++ compiler" FORCE)
    else()
        set(CMAKE_C_COMPILER "/usr/bin/gcc" CACHE STRING "C compiler" FORCE)
        set(CMAKE_CXX_COMPILER "/usr/bin/g++" CACHE STRING "C++ compiler" FORCE)
    endif()
endif()

# Set vcpkg overlay ports
set(VCPKG_OVERLAY_PORTS "${CMAKE_SOURCE_DIR}/ports/overlay")

# Set vcpkg triplet if not set
if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "Vcpkg target triplet" FORCE)
endif()

# Configure Boost before project()
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_DEBUG ON)  # Enable debug output for Boost finding

project(prometheus_disk_usage)

# Find Boost components early
find_package(Boost REQUIRED COMPONENTS program_options filesystem iostreams)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found. Please check vcpkg installation.")
endif()

message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(CheckSymbolExists)
INCLUDE(CheckCXXSourceCompiles)
cmake_policy(SET CMP0067 NEW)

# the pre-built fmt dep was compiled with _FORTIFY_SOURCE
# and requires __snprintf_chk, normally provided by glibc.
# If compiling for musl-based linux, e.g., Alpine, this is
# not available.
# In that case, use header-only fmt and accept the slower
# compilation.
check_symbol_exists(__snprintf_chk "" HAVE_SNPRINTF_CHK)

if (NOT HAVE_SNPRINTF_CHK)
    add_definitions(-DFMT_HEADER_ONLY)
endif ()

try_compile(IPO_FORTIFY_SOURCE_WORKING PROJECT musl_feature_test
            SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/musl_feature_test/"
            TARGET foo)
if (NOT IPO_FORTIFY_SOURCE_WORKING)
        message(WARNING "-D_FORTIFY_SOURCE will cause link time failures due to ipo/lto and vsnprintf, disabling")
        add_definitions(-U_FORTIFY_SOURCE)
endif()

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}")

add_subdirectory(src)
add_subdirectory(test)
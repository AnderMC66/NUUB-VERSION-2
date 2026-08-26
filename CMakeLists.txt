cmake_minimum_required(VERSION 3.25)
project(nuub VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Static CRT linkage (no VC++ Redistributable needed on target)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# vcpkg integration
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# Compiler warnings
include(cmake/CompilerWarnings.cmake)

# Find packages
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core imgcodecs imgproc videoio video)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
find_package(SndFile REQUIRED)
find_package(GTest CONFIG QUIET)
find_package(unofficial-argon2 CONFIG REQUIRED)

# Subdirectories
add_subdirectory(src/domain)
add_subdirectory(src/application)
add_subdirectory(src/infrastructure)

# Main executable
add_executable(nuub src/main.cpp)
target_include_directories(nuub PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(nuub PRIVATE
    nuub_domain
    nuub_application
    nuub_infrastructure
    nlohmann_json::nlohmann_json
)

# Tests
if(GTest_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif()

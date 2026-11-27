cmake_minimum_required(VERSION 3.25)

project(k15_gto VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Static CRT-independent builds on Windows use the x64-windows-static-md triplet,
# which still links the CRT dynamically (/MD), matching CMake's default.

option(GTO_BUILD_TESTS "Build unit tests" ON)
option(GTO_ENABLE_AVX2
  "Build for CPUs with AVX2 and FMA (Intel since 2013, AMD since 2015). Roughly a third faster."
  ON)

if(MSVC)
  # Headers included with <> (the standard library, gRPC, …) are not ours to lint.
  add_compile_options(/W4 /permissive- /utf-8 /external:anglebrackets /external:W0)
  add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(GTO_ENABLE_AVX2)
  if(MSVC)
    add_compile_options(/arch:AVX2)
  else()
    add_compile_options(-mavx2 -mfma)
  endif()
  add_compile_definitions(GTO_REQUIRES_AVX2)
endif()

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)

if(GTO_BUILD_TESTS)
  enable_testing()
  find_package(GTest CONFIG REQUIRED)
  include(GoogleTest)
endif()

add_subdirectory(proto)
add_subdirectory(engine)
add_subdirectory(server)
add_subdirectory(cli)

if(GTO_BUILD_TESTS)
  add_subdirectory(engine/tests)
endif()

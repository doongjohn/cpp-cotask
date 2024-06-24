cmake_minimum_required(VERSION 3.27)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

include("cmake/sanitizer.cmake")

project(
  Cotask
  VERSION 0.1.0
  DESCRIPTION ""
  HOMEPAGE_URL ""
)

if (WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "-fdiagnostics-color=always -fansi-escape-codes")
endif()

add_library(cotask "")

set_property(TARGET cotask PROPERTY CXX_STANDARD 20)
use_sanitizer(cotask)

target_sources(
  cotask
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS
      src
    FILES
      src/cotask/impl.hpp
      src/cotask/utils.hpp
      src/cotask/cotask.hpp
      src/cotask/timer.hpp
      src/cotask/file.hpp
      src/cotask/tcp.hpp
)

if (WIN32)
  target_sources(
    cotask
    PRIVATE
      src/cotask/windows/cotask.hpp
      src/cotask/windows/cotask.cpp
      src/cotask/windows/timer.hpp
      src/cotask/windows/timer.cpp
      src/cotask/windows/file.hpp
      src/cotask/windows/file.cpp
      src/cotask/windows/tcp.hpp
      src/cotask/windows/tcp.cpp
  )
  target_link_libraries(
    cotask
    PUBLIC
      ws2_32
      mswsock
  )
elseif (LINUX)
  message(FATAL "cotask not implemented for Linux")
elseif (APPLE)
  message(FATAL "cotask not implemented for MacOS")
else()
  message(FATAL "cotask not implemented for ${CMAKE_SYSTEM_NAME}")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(
    cotask
    PRIVATE
      -Wall
      -Wextra
  )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(
    cotask
    PRIVATE
      /W3
      /sdl
  )
endif()

include("cmake/example-task.cmake")
include("cmake/example-file.cmake")
include("cmake/example-tcp-server.cmake")
include("cmake/example-tcp-client.cmake")

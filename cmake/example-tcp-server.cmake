add_executable(cotask-example-tcp-server "")

set_property(TARGET cotask-example-tcp-server PROPERTY EXCLUDE_FROM_ALL true)
set_property(TARGET cotask-example-tcp-server PROPERTY CXX_STANDARD 20)
set_property(TARGET cotask-example-tcp-server PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
use_sanitizer(cotask-example-tcp-server)

target_sources(
  cotask-example-tcp-server
  PRIVATE
    example/tcp_server.cpp
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(
    cotask-example-tcp-server
    PRIVATE
      -Wall
      -Wextra
  )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(
    cotask-example-tcp-server
    PRIVATE
      /W3
      /sdl
  )
endif()

target_link_libraries(
  cotask-example-tcp-server
  PRIVATE
    cotask
)

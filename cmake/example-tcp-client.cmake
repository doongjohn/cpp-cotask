add_executable(cotask-example-tcp-client "")

set_property(TARGET cotask-example-tcp-client PROPERTY EXCLUDE_FROM_ALL true)
set_property(TARGET cotask-example-tcp-client PROPERTY CXX_STANDARD 20)
use_sanitizer(cotask-example-tcp-client)

target_sources(
  cotask-example-tcp-client
  PRIVATE
    example/tcp_client.cpp
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(
    cotask-example-tcp-client
    PRIVATE
      -Wall
      -Wextra
  )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(
    cotask-example-tcp-client
    PRIVATE
      /W3
      /sdl
  )
endif()

target_link_libraries(
  cotask-example-tcp-client
  PRIVATE
    cotask
)

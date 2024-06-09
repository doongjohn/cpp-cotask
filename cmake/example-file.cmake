add_executable(cotask-example-file "")

set_property(TARGET cotask-example-file PROPERTY EXCLUDE_FROM_ALL true)
set_property(TARGET cotask-example-file PROPERTY CXX_STANDARD 20)
use_sanitizer(cotask-example-file)

target_sources(
  cotask-example-file
  PRIVATE
    example/file.cpp
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(
    cotask-example-file
    PRIVATE
      -Wall
      -Wextra
  )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(
    cotask-example-file
    PRIVATE
      /W3
      /sdl
  )
endif()

target_link_libraries(
  cotask-example-file
  PRIVATE
    cotask
)

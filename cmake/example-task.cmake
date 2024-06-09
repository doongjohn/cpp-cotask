add_executable(cotask-example-task "")

set_property(TARGET cotask-example-task PROPERTY EXCLUDE_FROM_ALL true)
set_property(TARGET cotask-example-task PROPERTY CXX_STANDARD 20)
use_sanitizer(cotask-example-task)

target_sources(
  cotask-example-task
  PRIVATE
    example/task.cpp
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(
    cotask-example-task
    PRIVATE
      -Wall
      -Wextra
  )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(
    cotask-example-task
    PRIVATE
      /W3
      /sdl
  )
endif()

target_link_libraries(
  cotask-example-task
  PRIVATE
    cotask
)

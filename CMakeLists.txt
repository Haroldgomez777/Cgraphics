cmake_minimum_required(VERSION 3.20)
project(cgfx VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(CGFX_BUILD_EXAMPLES "Build example applications" ON)
option(CGFX_BUILD_TESTS "Build minimal tests" ON)

set(CGFX_PUBLIC_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(cgfx SHARED
  src/api/cgfx_api.cpp
  src/core/context.cpp
  src/core/window_impl.cpp
  src/render/gl_surface.cpp
)

target_include_directories(cgfx
  PUBLIC
    $<BUILD_INTERFACE:${CGFX_PUBLIC_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)


if(WIN32)
  target_sources(cgfx PRIVATE src/platform/platform_win32.cpp)
  target_compile_definitions(cgfx PRIVATE UNICODE _UNICODE)
  target_link_libraries(cgfx PRIVATE opengl32 gdi32 user32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_sources(cgfx PRIVATE src/platform/platform_linux_x11.cpp)
  find_package(X11 REQUIRED)
  find_package(OpenGL REQUIRED COMPONENTS OpenGL GLX)
  target_link_libraries(cgfx PRIVATE
    X11::X11
    OpenGL::GL
    OpenGL::GLX
  )
elseif(APPLE)
  message(FATAL_ERROR "cgfx Phase 1: macOS not implemented (use Linux or Windows).")
else()
  message(FATAL_ERROR "cgfx Phase 1: unsupported platform (${CMAKE_SYSTEM_NAME}).")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(cgfx PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
  target_compile_options(cgfx PRIVATE /W4)
endif()

set_target_properties(cgfx PROPERTIES
  OUTPUT_NAME cgfx
  WINDOWS_EXPORT_ALL_SYMBOLS OFF
)

if(CGFX_BUILD_EXAMPLES)
  add_executable(cgfx_demo examples/demo_main.c)
  target_link_libraries(cgfx_demo PRIVATE cgfx)
endif()

if(CGFX_BUILD_TESTS)
  enable_testing()
  add_executable(cgfx_minimal_shutdown tests/minimal_shutdown.c)
  target_link_libraries(cgfx_minimal_shutdown PRIVATE cgfx)
  add_test(NAME cgfx_minimal_shutdown COMMAND cgfx_minimal_shutdown)
endif()

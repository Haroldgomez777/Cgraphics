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
  src/core/widget_tree.cpp
  src/layout/flex_layout.cpp
  src/widgets/basic_widgets.cpp
  src/style/ui_theme.cpp
  src/style/widget_style_overrides.cpp
  src/animation/animation_clock.cpp
  src/animation/easing.cpp
  src/animation/widget_animation_system.cpp
  src/core/events/internal_event.cpp
  src/core/events/event_queue.cpp
  src/core/events/event_dispatch.cpp
  src/core/events/event_routing_diagnostics.cpp
  src/core/input_propagation.cpp
  src/core/window_impl.cpp
  src/render/render_command_list.cpp
  src/render/render_gl_device.cpp
  src/text/font_registry.cpp
  src/text/text_layout_stub.cpp
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

  add_executable(cgfx_professional_app
    examples/professional_app/main.c
    examples/professional_app/theme_setup.c
    examples/professional_app/ui_layout.c
  )
  target_link_libraries(cgfx_professional_app PRIVATE cgfx)
  target_include_directories(cgfx_professional_app PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/professional_app"
  )
endif()

if(CGFX_BUILD_TESTS)
  enable_testing()
  add_executable(cgfx_minimal_shutdown tests/minimal_shutdown.c)
  target_link_libraries(cgfx_minimal_shutdown PRIVATE cgfx)
  add_test(NAME cgfx_minimal_shutdown COMMAND cgfx_minimal_shutdown)

  add_executable(cgfx_event_trace_toggle_api_test
      tests/event_trace_toggle_api_test.c)
  target_link_libraries(cgfx_event_trace_toggle_api_test PRIVATE cgfx)
  add_test(NAME cgfx_event_trace_toggle_api_test
      COMMAND cgfx_event_trace_toggle_api_test)

  add_executable(cgfx_event_queue_test tests/event_queue_test.cpp
      src/core/events/event_queue.cpp
      src/core/events/internal_event.cpp
  )
  target_include_directories(cgfx_event_queue_test PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  set_target_properties(cgfx_event_queue_test PROPERTIES
      CXX_STANDARD 17
      CXX_EXTENSIONS OFF)
  target_compile_options(cgfx_event_queue_test PRIVATE
      $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)
  add_test(NAME cgfx_event_queue_test COMMAND cgfx_event_queue_test)

  add_executable(cgfx_widget_layout_test tests/widget_layout_test.cpp
      src/core/widget_tree.cpp
      src/layout/flex_layout.cpp
  )
  target_include_directories(cgfx_widget_layout_test PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  set_target_properties(cgfx_widget_layout_test PROPERTIES
      CXX_STANDARD 17
      CXX_EXTENSIONS OFF)
  target_compile_options(cgfx_widget_layout_test PRIVATE
      $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)
  add_test(NAME cgfx_widget_layout_test COMMAND cgfx_widget_layout_test)

  add_executable(cgfx_input_route_receivers_test tests/input_route_receivers_test.cpp
      src/core/widget_tree.cpp
      src/core/input_propagation.cpp
      src/layout/flex_layout.cpp
  )
  target_include_directories(cgfx_input_route_receivers_test PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  set_target_properties(cgfx_input_route_receivers_test PROPERTIES
      CXX_STANDARD 17
      CXX_EXTENSIONS OFF)
  target_compile_options(cgfx_input_route_receivers_test PRIVATE
      $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)
  add_test(NAME cgfx_input_route_receivers_test COMMAND cgfx_input_route_receivers_test)

  add_executable(cgfx_widget_input_routing_test tests/widget_input_routing_test.cpp
      src/core/widget_tree.cpp
      src/layout/flex_layout.cpp
  )
  target_include_directories(cgfx_widget_input_routing_test PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  set_target_properties(cgfx_widget_input_routing_test PROPERTIES
      CXX_STANDARD 17
      CXX_EXTENSIONS OFF)
  target_compile_options(cgfx_widget_input_routing_test PRIVATE
      $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)
  add_test(NAME cgfx_widget_input_routing_test COMMAND cgfx_widget_input_routing_test)

  add_executable(cgfx_basic_widgets_test tests/basic_widgets_test.cpp
      src/core/widget_tree.cpp
      src/layout/flex_layout.cpp
      src/widgets/basic_widgets.cpp
      src/style/ui_theme.cpp
      src/style/widget_style_overrides.cpp
      src/render/render_command_list.cpp
      src/text/text_layout_stub.cpp
  )
  target_include_directories(cgfx_basic_widgets_test PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  set_target_properties(cgfx_basic_widgets_test PROPERTIES
      CXX_STANDARD 17
      CXX_EXTENSIONS OFF)
  target_compile_options(cgfx_basic_widgets_test PRIVATE
      $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)
  add_test(NAME cgfx_basic_widgets_test COMMAND cgfx_basic_widgets_test)

  add_executable(cgfx_phase6_style_test tests/phase6_style_test.cpp
      src/core/widget_tree.cpp
      src/layout/flex_layout.cpp
      src/widgets/basic_widgets.cpp
      src/style/ui_theme.cpp
      src/style/widget_style_overrides.cpp
      src/render/render_command_list.cpp
      src/text/text_layout_stub.cpp
  )
  target_include_directories(cgfx_phase6_style_test PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  set_target_properties(cgfx_phase6_style_test PROPERTIES
      CXX_STANDARD 17
      CXX_EXTENSIONS OFF)
  target_compile_options(cgfx_phase6_style_test PRIVATE
      $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)
  add_test(NAME cgfx_phase6_style_test COMMAND cgfx_phase6_style_test)

  add_executable(cgfx_text_measurement_test tests/text_measurement_test.c)
  target_link_libraries(cgfx_text_measurement_test PRIVATE cgfx)
  add_test(NAME cgfx_text_measurement_test COMMAND cgfx_text_measurement_test)

  add_executable(cgfx_widget_text_integration_test tests/widget_text_integration_test.cpp
      src/core/widget_tree.cpp
      src/layout/flex_layout.cpp
      src/widgets/basic_widgets.cpp
      src/style/ui_theme.cpp
      src/style/widget_style_overrides.cpp
      src/render/render_command_list.cpp
      src/text/text_layout_stub.cpp
  )
  target_include_directories(cgfx_widget_text_integration_test PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  set_target_properties(cgfx_widget_text_integration_test PROPERTIES
      CXX_STANDARD 17
      CXX_EXTENSIONS OFF)
  target_compile_options(cgfx_widget_text_integration_test PRIVATE
      $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)
  add_test(NAME cgfx_widget_text_integration_test COMMAND cgfx_widget_text_integration_test)

  add_executable(cgfx_animation_phase8_test tests/animation_phase8_test.cpp
      src/animation/easing.cpp
      src/animation/widget_animation_system.cpp
      src/core/widget_tree.cpp
      src/layout/flex_layout.cpp
      src/widgets/basic_widgets.cpp
      src/style/ui_theme.cpp
      src/style/widget_style_overrides.cpp
      src/render/render_command_list.cpp
      src/text/text_layout_stub.cpp
  )
  target_include_directories(cgfx_animation_phase8_test PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  set_target_properties(cgfx_animation_phase8_test PROPERTIES
      CXX_STANDARD 17
      CXX_EXTENSIONS OFF)
  target_compile_options(cgfx_animation_phase8_test PRIVATE
      $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)
  add_test(NAME cgfx_animation_phase8_test COMMAND cgfx_animation_phase8_test)

  add_executable(cgfx_professional_app_api_smoke
    tests/professional_app_api_smoke.c)
  target_link_libraries(cgfx_professional_app_api_smoke PRIVATE cgfx)
  add_test(NAME cgfx_professional_app_api_smoke
    COMMAND cgfx_professional_app_api_smoke)
endif()

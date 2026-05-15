#pragma once

#include <cgfx/cgfx_api.h>

#include <cstddef>
#include <variant>

namespace cgfx {

class CgfxWindow;

struct EventClose {
  cgfx_event_close_payload payload{};
};

struct EventResize {
  cgfx_event_resize_payload payload{};
};

struct EventMouseMove {
  cgfx_event_mouse_move_payload payload{};
};

struct EventMouseButton {
  cgfx_event_mouse_button_payload payload{};
};

struct EventKey {
  cgfx_event_key_payload payload{};
};

struct EventWidgetClick {
  cgfx_event_widget_click_payload payload{};
};

using EventBody =
    std::variant<EventClose, EventResize, EventMouseMove, EventMouseButton,
                 EventKey, EventWidgetClick>;

/** Typed platform event before C API serialization (variant payload + window).
 *  @p sequence assigned by EventQueue enqueue (0 until first queue admission). */
struct InternalEvent {
  CgfxWindow *window{};
  uint64_t sequence{};
  EventBody body{};

  static InternalEvent close_request(CgfxWindow *w,
                                     cgfx_event_close_payload p) noexcept;

  static InternalEvent resize(CgfxWindow *w,
                              cgfx_event_resize_payload p) noexcept;

  static InternalEvent mouse_move(CgfxWindow *w,
                                  cgfx_event_mouse_move_payload p) noexcept;

  static InternalEvent mouse_button(
      CgfxWindow *w, cgfx_event_mouse_button_payload p) noexcept;

  static InternalEvent key(CgfxWindow *w, cgfx_event_key_payload p) noexcept;

  static InternalEvent widget_click(
      CgfxWindow *w, cgfx_event_widget_click_payload p) noexcept;
};

cgfx_event_type internal_event_kind(const InternalEvent &e) noexcept;

CgfxWindow *internal_event_window(const InternalEvent &e) noexcept;

size_t internal_event_payload_byte_size(const InternalEvent &e) noexcept;

/** Copies payload for @p e into @p dst (must hold at least internal_event_payload_byte_size bytes). */
void internal_event_copy_payload_bytes(const InternalEvent &e, void *dst) noexcept;

inline uint64_t internal_event_sequence(const InternalEvent &e) noexcept {
  return e.sequence;
}

} // namespace cgfx

#include "core/events/internal_event.hpp"

#include <cstring>

namespace cgfx {

InternalEvent InternalEvent::close_request(
    CgfxWindow *w, cgfx_event_close_payload p) noexcept {
  InternalEvent e{};
  e.window = w;
  e.body = EventClose{p};
  return e;
}

InternalEvent InternalEvent::resize(CgfxWindow *w,
                                   cgfx_event_resize_payload p) noexcept {
  InternalEvent e{};
  e.window = w;
  e.body = EventResize{p};
  return e;
}

InternalEvent InternalEvent::mouse_move(
    CgfxWindow *w, cgfx_event_mouse_move_payload p) noexcept {
  InternalEvent e{};
  e.window = w;
  e.body = EventMouseMove{p};
  return e;
}

InternalEvent InternalEvent::mouse_button(
    CgfxWindow *w, cgfx_event_mouse_button_payload p) noexcept {
  InternalEvent e{};
  e.window = w;
  e.body = EventMouseButton{p};
  return e;
}

InternalEvent InternalEvent::key(CgfxWindow *w,
                                   cgfx_event_key_payload p) noexcept {
  InternalEvent e{};
  e.window = w;
  e.body = EventKey{p};
  return e;
}

cgfx_event_type internal_event_kind(const InternalEvent &e) noexcept {
  return std::visit(
      [](const auto &payload) noexcept -> cgfx_event_type {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, EventClose>) {
          return CGFX_EVENT_CLOSE_REQUEST;
        }
        if constexpr (std::is_same_v<T, EventResize>) {
          return CGFX_EVENT_RESIZE;
        }
        if constexpr (std::is_same_v<T, EventMouseMove>) {
          return CGFX_EVENT_MOUSE_MOVE;
        }
        if constexpr (std::is_same_v<T, EventMouseButton>) {
          return CGFX_EVENT_MOUSE_BUTTON;
        }
        if constexpr (std::is_same_v<T, EventKey>) {
          return CGFX_EVENT_KEY;
        }
        return static_cast<cgfx_event_type>(0);
      },
      e.body);
}

CgfxWindow *internal_event_window(const InternalEvent &e) noexcept {
  return e.window;
}

size_t internal_event_payload_byte_size(const InternalEvent &e) noexcept {
  return std::visit(
      [](const auto &payload) noexcept -> size_t {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, EventClose>) {
          return sizeof(payload.payload);
        }
        if constexpr (std::is_same_v<T, EventResize>) {
          return sizeof(payload.payload);
        }
        if constexpr (std::is_same_v<T, EventMouseMove>) {
          return sizeof(payload.payload);
        }
        if constexpr (std::is_same_v<T, EventMouseButton>) {
          return sizeof(payload.payload);
        }
        if constexpr (std::is_same_v<T, EventKey>) {
          return sizeof(payload.payload);
        }
        return 0;
      },
      e.body);
}

void internal_event_copy_payload_bytes(const InternalEvent &e,
                                       void *dst) noexcept {
  if (!dst) {
    return;
  }
  std::visit(
      [dst](const auto &payload) noexcept {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, EventClose>) {
          std::memcpy(dst, &payload.payload, sizeof(payload.payload));
        } else if constexpr (std::is_same_v<T, EventResize>) {
          std::memcpy(dst, &payload.payload, sizeof(payload.payload));
        } else if constexpr (std::is_same_v<T, EventMouseMove>) {
          std::memcpy(dst, &payload.payload, sizeof(payload.payload));
        } else if constexpr (std::is_same_v<T, EventMouseButton>) {
          std::memcpy(dst, &payload.payload, sizeof(payload.payload));
        } else if constexpr (std::is_same_v<T, EventKey>) {
          std::memcpy(dst, &payload.payload, sizeof(payload.payload));
        }
      },
      e.body);
}

} // namespace cgfx

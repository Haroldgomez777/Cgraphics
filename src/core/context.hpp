#pragma once

#include <cgfx/cgfx_api.h>

#include <deque>
#include <memory>

namespace cgfx {

class PlatformBackend;
class CgfxWindow;

struct QueuedEvent {
  cgfx_event_type type{};
  CgfxWindow *window{};
  cgfx_event_resize_payload resize{};
  cgfx_event_mouse_move_payload move{};
  cgfx_event_mouse_button_payload mb{};
  cgfx_event_key_payload key{};
  cgfx_event_close_payload close{};
};

class CgfxContext {
public:
  static CgfxContext *from_opaque(cgfx_context *p) {
    return reinterpret_cast<CgfxContext *>(p);
  }

  cgfx_context *opaque() { return reinterpret_cast<cgfx_context *>(this); }

  CgfxContext();
  ~CgfxContext();

  CgfxContext(const CgfxContext &) = delete;
  CgfxContext &operator=(const CgfxContext &) = delete;

  static cgfx_result create(std::unique_ptr<CgfxContext> &out);

  cgfx_result create_window_impl(const cgfx_window_desc *desc,
                                 cgfx_window **out_window);

  void destroy_window_impl(CgfxWindow *wnd);

  void push_event(const QueuedEvent &ev);
  bool pop_event(QueuedEvent &ev);

  void poll_events_impl();

  PlatformBackend *platform_backend() { return backend_.get(); }

  void push_priority_event_front(const QueuedEvent &ev);

private:
  std::unique_ptr<PlatformBackend> backend_{};
  std::deque<QueuedEvent> events_{};
};

} // namespace cgfx


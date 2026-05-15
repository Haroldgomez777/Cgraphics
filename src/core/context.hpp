#pragma once

#include <cgfx/cgfx_api.h>

#include "core/events/event_queue.hpp"

#include <memory>

namespace cgfx {

class PlatformBackend;
class CgfxWindow;

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

  void enqueue_event(InternalEvent ev);

  /** @see cgfx_next_event undersized-buffer retry path */
  void push_priority_event_front(const InternalEvent &ev);

  bool pop_event(InternalEvent &ev);

  void poll_events_impl();

  PlatformBackend *platform_backend() { return backend_.get(); }

  EventQueue &mutable_event_queue() noexcept { return event_queue_; }
  const EventQueue &event_queue() const noexcept { return event_queue_; }

private:
  std::unique_ptr<PlatformBackend> backend_{};
  EventQueue event_queue_{};
};

} // namespace cgfx

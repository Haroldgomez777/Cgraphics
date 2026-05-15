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

  cgfx_input_propagation_policy default_input_propagation_policy() const noexcept {
    return default_propagation_policy_;
  }

  void set_default_input_propagation_policy(
      cgfx_input_propagation_policy policy) noexcept {
    default_propagation_policy_ = policy;
  }

  void set_input_routing_trace_enabled(bool enabled) noexcept {
    input_routing_trace_enabled_ = enabled;
  }

  bool input_routing_trace_enabled() const noexcept {
    return input_routing_trace_enabled_;
  }

  uint64_t last_enqueued_event_sequence() const noexcept {
    return event_queue_.last_enqueued_sequence();
  }

private:
  std::unique_ptr<PlatformBackend> backend_{};
  EventQueue event_queue_{};
  cgfx_input_propagation_policy default_propagation_policy_{
      CGFX_INPUT_PROPAGATION_TARGET_ONLY};
  bool input_routing_trace_enabled_{false};
};

} // namespace cgfx

#include "core/events/event_dispatch.hpp"

#include "core/context.hpp"
#include "core/input_propagation.hpp"
#include "core/events/event_routing_diagnostics.hpp"
#include "core/window_impl.hpp"

#include <vector>

namespace cgfx {

namespace {

void enqueue_mouse_button_propagating(CgfxContext &ctx, CgfxWindow *window,
                                      cgfx_event_mouse_button_payload p) {
  std::vector<cgfx_widget_id> receivers;
  build_input_route_receiver_ids(window->widget_tree(), p.target_widget,
                                 window->input_propagation_policy(),
                                 receivers);

  for (cgfx_widget_id routed : receivers) {
    cgfx_event_mouse_button_payload copy = p;
    copy.routed_widget = routed;
    ctx.enqueue_event(InternalEvent::mouse_button(window, copy));
    if (ctx.input_routing_trace_enabled()) {
      emit_input_routing_route_trace(
          CGFX_EVENT_MOUSE_BUTTON, ctx.last_enqueued_event_sequence(),
          p.target_widget, routed, window->input_propagation_policy());
    }
  }
}

void enqueue_key_propagating(CgfxContext &ctx, CgfxWindow *window,
                             cgfx_event_key_payload p) {
  std::vector<cgfx_widget_id> receivers;
  build_input_route_receiver_ids(window->widget_tree(), p.target_widget,
                                 window->input_propagation_policy(), receivers);

  for (cgfx_widget_id routed : receivers) {
    cgfx_event_key_payload copy = p;
    copy.routed_widget = routed;
    ctx.enqueue_event(InternalEvent::key(window, copy));
    if (ctx.input_routing_trace_enabled()) {
      emit_input_routing_route_trace(
          CGFX_EVENT_KEY, ctx.last_enqueued_event_sequence(),
          p.target_widget, routed, window->input_propagation_policy());
    }
  }
}

} // namespace

void event_dispatch_close(CgfxContext &ctx, CgfxWindow *window) {
  cgfx_event_close_payload p{};
  p.unused = 0;
  ctx.enqueue_event(InternalEvent::close_request(window, p));
}

void event_dispatch_resize(CgfxContext &ctx, CgfxWindow *window,
                           uint32_t width_px, uint32_t height_px) {
  cgfx_event_resize_payload p{};
  p.width = width_px;
  p.height = height_px;
  ctx.enqueue_event(InternalEvent::resize(window, p));
}

void event_dispatch_mouse_move(CgfxContext &ctx, CgfxWindow *window, int32_t x,
                               int32_t y) {
  cgfx_event_mouse_move_payload p{};
  p.target_widget = CGFX_WIDGET_ID_NONE;
  p.x = x;
  p.y = y;
  routing_sync_pick_mouse_move_targets(window, &p);
  ctx.enqueue_event(InternalEvent::mouse_move(window, p));
}

void event_dispatch_mouse_button(CgfxContext &ctx, CgfxWindow *window,
                                 cgfx_mouse_button button,
                                 cgfx_input_action action, int32_t x,
                                 int32_t y) {
  cgfx_event_mouse_button_payload p{};
  p.button = button;
  p.action = action;
  p.x = x;
  p.y = y;
  p.target_widget = CGFX_WIDGET_ID_NONE;
  routing_sync_pick_mouse_button_targets(window, &p);
  enqueue_mouse_button_propagating(ctx, window, p);
}

void event_dispatch_key(CgfxContext &ctx, CgfxWindow *window, cgfx_key key,
                        uint32_t native_code, cgfx_input_action action,
                        int repeat) {
  cgfx_event_key_payload p{};
  p.target_widget = CGFX_WIDGET_ID_NONE;
  p.key = key;
  p.native_code = native_code;
  p.action = action;
  p.repeat = repeat;
  routing_pick_key_focus_targets(window, &p);
  enqueue_key_propagating(ctx, window, p);
}

} // namespace cgfx

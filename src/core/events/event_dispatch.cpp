#include "core/events/event_dispatch.hpp"

#include "core/context.hpp"

namespace cgfx {

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
  p.x = x;
  p.y = y;
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
  ctx.enqueue_event(InternalEvent::mouse_button(window, p));
}

void event_dispatch_key(CgfxContext &ctx, CgfxWindow *window, cgfx_key key,
                        uint32_t native_code, cgfx_input_action action,
                        int repeat) {
  cgfx_event_key_payload p{};
  p.key = key;
  p.native_code = native_code;
  p.action = action;
  p.repeat = repeat;
  ctx.enqueue_event(InternalEvent::key(window, p));
}

} // namespace cgfx

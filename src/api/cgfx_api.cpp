#include <cgfx/cgfx_api.h>

#include "core/context.hpp"
#include "core/window_impl.hpp"

#include <cstring>
#include <memory>

namespace {

size_t payload_bytes_for_type(cgfx_event_type type) noexcept {
  switch (type) {
  case CGFX_EVENT_CLOSE_REQUEST:
    return sizeof(cgfx_event_close_payload);
  case CGFX_EVENT_RESIZE:
    return sizeof(cgfx_event_resize_payload);
  case CGFX_EVENT_MOUSE_MOVE:
    return sizeof(cgfx_event_mouse_move_payload);
  case CGFX_EVENT_MOUSE_BUTTON:
    return sizeof(cgfx_event_mouse_button_payload);
  case CGFX_EVENT_KEY:
    return sizeof(cgfx_event_key_payload);
  default:
    return 0;
  }
}

} // namespace

extern "C" {

cgfx_result cgfx_context_create(cgfx_context **out_context) {
  if (!out_context) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  std::unique_ptr<cgfx::CgfxContext> ctx;
  const cgfx_result rc = cgfx::CgfxContext::create(ctx);
  if (rc != CGFX_OK) {
    return rc;
  }

  cgfx::CgfxContext *raw = ctx.release();
  *out_context = raw->opaque();
  return CGFX_OK;
}

void cgfx_context_destroy(cgfx_context *context) {
  delete cgfx::CgfxContext::from_opaque(context);
}

cgfx_result cgfx_window_create(cgfx_context *context,
                               const cgfx_window_desc *desc,
                               cgfx_window **out_window) {
  if (!context || !desc || !out_window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  return cgfx::CgfxContext::from_opaque(context)->create_window_impl(desc,
                                                                      out_window);
}

void cgfx_window_destroy(cgfx_window *window) {
  if (!window) {
    return;
  }

  cgfx::CgfxWindow *wnd = cgfx::CgfxWindow::from_opaque(window);
  cgfx::CgfxContext &owning_context = wnd->context();
  owning_context.destroy_window_impl(wnd);
}

cgfx_result cgfx_poll_events(cgfx_context *context) {
  if (!context) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  cgfx::CgfxContext::from_opaque(context)->poll_events_impl();
  return CGFX_OK;
}

bool cgfx_next_event(cgfx_context *context, cgfx_event_type *type,
                     cgfx_window **window, void *event_payload_bytes,
                     size_t payload_capacity, size_t *out_payload_used) {
  if (!context || !type || !window) {
    return false;
  }

  if (out_payload_used) {
    *out_payload_used = 0;
  }

  cgfx::CgfxContext *ctx_impl = cgfx::CgfxContext::from_opaque(context);

  cgfx::QueuedEvent ev{};
  if (!ctx_impl->pop_event(ev)) {
    return false;
  }

  const size_t needed = payload_bytes_for_type(ev.type);

  if (out_payload_used) {
    *out_payload_used = needed;
  }

  if (event_payload_bytes) {
    if (payload_capacity < needed) {
      ctx_impl->push_priority_event_front(ev);
      if (out_payload_used) {
        *out_payload_used = 0;
      }
      return false;
    }

    switch (ev.type) {
    case CGFX_EVENT_CLOSE_REQUEST:
      std::memcpy(event_payload_bytes, &ev.close, sizeof(ev.close));
      break;
    case CGFX_EVENT_RESIZE:
      std::memcpy(event_payload_bytes, &ev.resize, sizeof(ev.resize));
      break;
    case CGFX_EVENT_MOUSE_MOVE:
      std::memcpy(event_payload_bytes, &ev.move, sizeof(ev.move));
      break;
    case CGFX_EVENT_MOUSE_BUTTON:
      std::memcpy(event_payload_bytes, &ev.mb, sizeof(ev.mb));
      break;
    case CGFX_EVENT_KEY:
      std::memcpy(event_payload_bytes, &ev.key, sizeof(ev.key));
      break;
    default:
      break;
    }
  }

  *type = ev.type;
  *window = ev.window ? ev.window->opaque() : nullptr;

  return true;
}

cgfx_result cgfx_window_begin_present_pass(cgfx_window *window,
                                           uint32_t *out_width_px,
                                           uint32_t *out_height_px,
                                           float *out_dpi_scale) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)->begin_present_pass(
      out_width_px, out_height_px, out_dpi_scale);
}

void cgfx_window_end_present_pass(cgfx_window *window) {
  if (!window) {
    return;
  }
  cgfx::CgfxWindow::from_opaque(window)->end_present_pass();
}

cgfx_result cgfx_surface_clear_normalized_rgba(cgfx_window *window, float red,
                                             float green, float blue,
                                             float alpha) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)->clear_present_surface(
      red, green, blue, alpha);
}

cgfx_result cgfx_surface_fill_rect_pixels(cgfx_window *window, int32_t x_px,
                                          int32_t y_px, uint32_t width_px,
                                          uint32_t height_px, float red,
                                          float green, float blue,
                                          float alpha) {
  if (!window || width_px == 0U || height_px == 0U) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)->fill_rect_present_surface(
      x_px, y_px, width_px, height_px, red, green, blue, alpha);
}

cgfx_result cgfx_surface_fill_rect_batch_pixels(cgfx_window *window,
                                                const cgfx_surface_fill_rect_item *items,
                                                size_t item_count,
                                                size_t stride_bytes) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  if (item_count == 0U) {
    return CGFX_OK;
  }
  if (!items) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  return cgfx::CgfxWindow::from_opaque(window)->fill_rect_batch_present_surface(
      items, item_count, stride_bytes);
}

cgfx_result cgfx_window_get_size_pixels(const cgfx_window *window,
                                        uint32_t *out_width,
                                        uint32_t *out_height) {
  if (!window || !out_width || !out_height) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  cgfx::PlatformSurface *surface =
      cgfx::CgfxWindow::from_opaque(const_cast<cgfx_window *>(window))
          ->surface();

  if (!surface) {
    return CGFX_ERROR_PLATFORM;
  }

  return surface->query_size_px(out_width, out_height);
}

cgfx_result cgfx_window_get_dpi_scale(const cgfx_window *window,
                                      float *out_scale) {
  if (!window || !out_scale) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  cgfx::PlatformSurface *surface =
      cgfx::CgfxWindow::from_opaque(const_cast<cgfx_window *>(window))
          ->surface();

  if (!surface) {
    return CGFX_ERROR_PLATFORM;
  }

  return surface->query_dpi_scale(out_scale);
}

} // extern "C"

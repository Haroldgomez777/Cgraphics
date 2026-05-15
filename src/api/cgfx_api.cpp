#include <cgfx/cgfx_api.h>

#include "core/context.hpp"
#include "core/events/internal_event.hpp"
#include "core/window_impl.hpp"

#include <cstring>
#include <memory>
#include <type_traits>
#include <variant>

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

cgfx::EventQueueOverflowPolicy to_internal_overflow(
    cgfx_event_queue_overflow_policy p) noexcept {
  switch (p) {
  case CGFX_EVENT_QUEUE_OVERFLOW_DROP_NEWEST:
    return cgfx::EventQueueOverflowPolicy::DropNewest;
  case CGFX_EVENT_QUEUE_OVERFLOW_DROP_OLDEST:
    return cgfx::EventQueueOverflowPolicy::DropOldest;
  default:
    return cgfx::EventQueueOverflowPolicy::DropOldest;
  }
}

cgfx_event_queue_overflow_policy to_c_overflow(
    cgfx::EventQueueOverflowPolicy p) noexcept {
  switch (p) {
  case cgfx::EventQueueOverflowPolicy::DropNewest:
    return CGFX_EVENT_QUEUE_OVERFLOW_DROP_NEWEST;
  case cgfx::EventQueueOverflowPolicy::DropOldest:
  default:
    return CGFX_EVENT_QUEUE_OVERFLOW_DROP_OLDEST;
  }
}

void internal_event_to_out(const cgfx::InternalEvent &iev,
                           cgfx_event *out_ev) noexcept {
  out_ev->type = cgfx::internal_event_kind(iev);
  cgfx::CgfxWindow *w = cgfx::internal_event_window(iev);
  out_ev->window = w ? w->opaque() : nullptr;

  std::visit(
      [out_ev](const auto &body) noexcept {
        using T = std::decay_t<decltype(body)>;
        if constexpr (std::is_same_v<T, cgfx::EventClose>) {
          out_ev->payload.close = body.payload;
        } else if constexpr (std::is_same_v<T, cgfx::EventResize>) {
          out_ev->payload.resize = body.payload;
        } else if constexpr (std::is_same_v<T, cgfx::EventMouseMove>) {
          out_ev->payload.mouse_move = body.payload;
        } else if constexpr (std::is_same_v<T, cgfx::EventMouseButton>) {
          out_ev->payload.mouse_button = body.payload;
        } else if constexpr (std::is_same_v<T, cgfx::EventKey>) {
          out_ev->payload.key = body.payload;
        }
      },
      iev.body);
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

void cgfx_context_set_default_input_propagation_policy(
    cgfx_context *context, cgfx_input_propagation_policy policy) {
  if (!context) {
    return;
  }
  cgfx::CgfxContext::from_opaque(context)
      ->set_default_input_propagation_policy(policy);
}

cgfx_input_propagation_policy cgfx_context_get_default_input_propagation_policy(
    const cgfx_context *context) {
  if (!context) {
    return CGFX_INPUT_PROPAGATION_TARGET_ONLY;
  }
  return cgfx::CgfxContext::from_opaque(
             const_cast<cgfx_context *>(context))
      ->default_input_propagation_policy();
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

void cgfx_window_set_input_propagation_policy(
    cgfx_window *window, cgfx_input_propagation_policy policy) {
  if (!window) {
    return;
  }
  cgfx::CgfxWindow::from_opaque(window)->set_input_propagation_policy(policy);
}

cgfx_input_propagation_policy cgfx_window_get_input_propagation_policy(
    const cgfx_window *window) {
  if (!window) {
    return CGFX_INPUT_PROPAGATION_TARGET_ONLY;
  }
  return cgfx::CgfxWindow::from_opaque(const_cast<cgfx_window *>(window))
      ->input_propagation_policy();
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

  cgfx::InternalEvent iev{};
  if (!ctx_impl->pop_event(iev)) {
    return false;
  }

  const size_t needed = cgfx::internal_event_payload_byte_size(iev);

  if (out_payload_used) {
    *out_payload_used = needed;
  }

  if (event_payload_bytes) {
    if (payload_capacity < needed) {
      ctx_impl->push_priority_event_front(iev);
      if (out_payload_used) {
        *out_payload_used = 0;
      }
      return false;
    }

    cgfx::internal_event_copy_payload_bytes(iev, event_payload_bytes);
  }

  *type = cgfx::internal_event_kind(iev);
  cgfx::CgfxWindow *w = cgfx::internal_event_window(iev);
  *window = w ? w->opaque() : nullptr;

  return true;
}

size_t cgfx_event_payload_byte_size(cgfx_event_type type) {
  return payload_bytes_for_type(type);
}

cgfx_result cgfx_context_set_event_queue_limits(
    cgfx_context *context, size_t max_pending_events,
    cgfx_event_queue_overflow_policy overflow_policy) {
  if (!context) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  cgfx::EventQueue &q =
      cgfx::CgfxContext::from_opaque(context)->mutable_event_queue();
  q.set_overflow_policy(to_internal_overflow(overflow_policy));

  const size_t cap =
      max_pending_events == 0U
          ? static_cast<size_t>(cgfx::EventQueue::kDefaultMaxDepth)
          : max_pending_events;

  q.set_max_depth(cap);
  return CGFX_OK;
}

cgfx_result cgfx_context_get_event_queue_limits(
    const cgfx_context *context, size_t *out_max_pending,
    cgfx_event_queue_overflow_policy *out_overflow_policy) {
  if (!context) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  const cgfx::EventQueue &q =
      cgfx::CgfxContext::from_opaque(const_cast<cgfx_context *>(context))
          ->event_queue();

  if (out_max_pending) {
    *out_max_pending = q.max_depth();
  }
  if (out_overflow_policy) {
    *out_overflow_policy = to_c_overflow(q.overflow_policy());
  }
  return CGFX_OK;
}

void cgfx_context_set_event_resize_coalesce(cgfx_context *context,
                                             bool enabled) {
  if (!context) {
    return;
  }
  cgfx::CgfxContext::from_opaque(context)
      ->mutable_event_queue()
      .set_coalesce_resize(enabled);
}

bool cgfx_context_get_event_resize_coalesce(const cgfx_context *context) {
  if (!context) {
    return true;
  }
  return cgfx::CgfxContext::from_opaque(const_cast<cgfx_context *>(context))
      ->event_queue()
      .coalesce_resize();
}

uint64_t cgfx_context_event_queue_drop_count(const cgfx_context *context) {
  if (!context) {
    return 0;
  }
  return cgfx::CgfxContext::from_opaque(const_cast<cgfx_context *>(context))
      ->event_queue()
      .dropped_event_count();
}

void cgfx_context_event_queue_reset_drop_count(cgfx_context *context) {
  if (!context) {
    return;
  }
  cgfx::CgfxContext::from_opaque(context)
      ->mutable_event_queue()
      .reset_drop_count();
}

bool cgfx_next_event_into(cgfx_context *context, cgfx_event *out_event) {
  if (!context || !out_event) {
    return false;
  }

  cgfx::InternalEvent iev{};
  if (!cgfx::CgfxContext::from_opaque(context)->pop_event(iev)) {
    return false;
  }

  internal_event_to_out(iev, out_event);
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
                                                const void *items,
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

cgfx_widget_id cgfx_window_widget_root(const cgfx_window *window) {
  if (!window) {
    return 0;
  }
  const cgfx::CgfxWindow *w =
      cgfx::CgfxWindow::from_opaque(const_cast<cgfx_window *>(window));
  return w->widget_tree().root_id();
}

cgfx_result cgfx_widget_create_child(cgfx_window *window,
                                     cgfx_widget_id parent_id,
                                     cgfx_widget_id *out_child_id) {
  if (!window || !out_child_id) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  return w->widget_tree_mut().create_child(parent_id,
                                            reinterpret_cast<uint64_t *>(out_child_id));
}

cgfx_result cgfx_widget_destroy(cgfx_window *window, cgfx_widget_id widget_id) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  const cgfx_result r = w->widget_tree_mut().destroy_subtree(widget_id);
  if (r == CGFX_OK) {
    w->reconcile_focus_after_structure_change();
  }
  return r;
}

cgfx_result cgfx_widget_reparent(cgfx_window *window,
                                 cgfx_widget_id widget_id,
                                 cgfx_widget_id new_parent_id) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  return w->widget_tree_mut().set_parent(widget_id, new_parent_id);
}

cgfx_result cgfx_widget_set_layout_axis(cgfx_window *window,
                                        cgfx_widget_id widget_id,
                                        cgfx_layout_axis axis) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)
      ->widget_tree_mut()
      .set_layout_axis(widget_id, axis);
}

cgfx_result cgfx_widget_set_width(cgfx_window *window,
                                cgfx_widget_id widget_id,
                                cgfx_layout_size_kind kind, uint32_t fixed_px) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)
      ->widget_tree_mut()
      .set_width(widget_id, kind, fixed_px);
}

cgfx_result cgfx_widget_set_height(cgfx_window *window,
                                   cgfx_widget_id widget_id,
                                   cgfx_layout_size_kind kind,
                                   uint32_t fixed_px) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)
      ->widget_tree_mut()
      .set_height(widget_id, kind, fixed_px);
}

cgfx_result cgfx_widget_set_flex_grow(cgfx_window *window,
                                      cgfx_widget_id widget_id,
                                      float flex_grow) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)
      ->widget_tree_mut()
      .set_flex_grow(widget_id, flex_grow);
}

cgfx_result cgfx_widget_set_flex_shrink(cgfx_window *window,
                                        cgfx_widget_id widget_id,
                                        float flex_shrink) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)
      ->widget_tree_mut()
      .set_flex_shrink(widget_id, flex_shrink);
}

cgfx_result cgfx_widget_bounds_logical_px(const cgfx_window *window,
                                          cgfx_widget_id widget_id,
                                          cgfx_layout_rect *out_bounds) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  const cgfx::CgfxWindow *w =
      cgfx::CgfxWindow::from_opaque(const_cast<cgfx_window *>(window));
  return w->widget_tree().get_bounds(widget_id, out_bounds);
}

cgfx_result cgfx_window_hit_test_logical_px(cgfx_window *window, int32_t x,
                                             int32_t y,
                                             cgfx_widget_id *out_target) {
  if (!window || !out_target) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);

  /** Match input routing snapshot */
  w->sync_widget_layout_logical_from_surface();
  *out_target = w->widget_tree().hit_test_logical(x, y);
  return CGFX_OK;
}

cgfx_widget_id cgfx_window_focus_widget(const cgfx_window *window) {
  if (!window) {
    return CGFX_WIDGET_ID_NONE;
  }
  return cgfx::CgfxWindow::from_opaque(const_cast<cgfx_window *>(window))
      ->resolved_focus_widget_id();
}

cgfx_result cgfx_window_set_focus_widget(cgfx_window *window,
                                        cgfx_widget_id widget_id) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)->assign_focus_widget_logical(
      widget_id);
}

} // extern "C"

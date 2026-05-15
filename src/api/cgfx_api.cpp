#include <cgfx/cgfx_api.h>

#include "core/context.hpp"
#include "core/events/internal_event.hpp"
#include "core/window_impl.hpp"
#include "style/ui_theme.hpp"
#include "style/widget_style_overrides.hpp"
#include "text/text_layout_stub.hpp"
#include "widgets/basic_widgets.hpp"

#include <cmath>
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
  case CGFX_EVENT_WIDGET_CLICK:
    return sizeof(cgfx_event_widget_click_payload);
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
  out_ev->sequence = cgfx::internal_event_sequence(iev);

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
        } else if constexpr (std::is_same_v<T, cgfx::EventWidgetClick>) {
          out_ev->payload.widget_click = body.payload;
        }
      },
      iev.body);
}

bool dequeue_event_copy_payload_try(cgfx::CgfxContext *ctx_impl,
                                    cgfx::InternalEvent &iev,
                                    cgfx_event_type *type,
                                    cgfx_window **window_handle,
                                    void *event_payload_bytes,
                                    size_t payload_capacity,
                                    size_t *out_payload_used,
                                    uint64_t *out_sequence_optional) noexcept {
  if (out_payload_used) {
    *out_payload_used = 0;
  }

  if (!ctx_impl->pop_event(iev)) {
    return false;
  }

  if (out_sequence_optional) {
    *out_sequence_optional = cgfx::internal_event_sequence(iev);
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
      if (out_sequence_optional) {
        *out_sequence_optional = 0;
      }
      return false;
    }

    cgfx::internal_event_copy_payload_bytes(iev, event_payload_bytes);
  }

  *type = cgfx::internal_event_kind(iev);
  cgfx::CgfxWindow *w = cgfx::internal_event_window(iev);
  *window_handle = w ? w->opaque() : nullptr;

  return true;
}

cgfx::RgbaNormalized *mutable_theme_color(cgfx::UiTheme &th,
                                          cgfx_theme_color_token token) noexcept {
  switch (token) {
  case CGFX_THEME_COLOR_PANEL_BACKGROUND:
    return &th.panel_background;
  case CGFX_THEME_COLOR_LABEL_TEXT:
    return &th.label_text_color;
  case CGFX_THEME_COLOR_LABEL_PLACEHOLDER:
    return &th.label_placeholder;
  case CGFX_THEME_COLOR_BUTTON_BACKGROUND_NORMAL:
    return &th.button_background_normal;
  case CGFX_THEME_COLOR_BUTTON_BACKGROUND_HOVER:
    return &th.button_background_hover;
  case CGFX_THEME_COLOR_BUTTON_BACKGROUND_PRESSED:
    return &th.button_background_pressed;
  case CGFX_THEME_COLOR_BUTTON_BACKGROUND_DISABLED:
    return &th.button_background_disabled;
  case CGFX_THEME_COLOR_BUTTON_CAPTION_PLACEHOLDER:
    return &th.button_caption_placeholder;
  case CGFX_THEME_COLOR_BUTTON_TEXT:
    return &th.button_text_color;
  default:
    return nullptr;
  }
}

const cgfx::RgbaNormalized *
const_theme_color(const cgfx::UiTheme &th,
                  cgfx_theme_color_token token) noexcept {
  return mutable_theme_color(const_cast<cgfx::UiTheme &>(th), token);
}

cgfx_result set_theme_metric(cgfx::UiTheme &th, cgfx_theme_metric_token token,
                             float metric) noexcept {
  switch (token) {
  case CGFX_THEME_METRIC_LABEL_FONT_SIZE_SP:
    th.label_font_size_sp = metric;
    return CGFX_OK;
  case CGFX_THEME_METRIC_BUTTON_FONT_SIZE_SP:
    th.button_font_size_sp = metric;
    return CGFX_OK;
  case CGFX_THEME_METRIC_SPACING_UNIT_PX:
    th.spacing_unit_px = metric;
    return CGFX_OK;
  case CGFX_THEME_METRIC_PADDING_SM_PX:
    th.padding_sm_px = metric;
    return CGFX_OK;
  case CGFX_THEME_METRIC_PADDING_MD_PX:
    th.padding_md_px = metric;
    return CGFX_OK;
  default:
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
}

cgfx_result get_theme_metric(const cgfx::UiTheme &th, cgfx_theme_metric_token token,
                             float *out) noexcept {
  if (!out) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  switch (token) {
  case CGFX_THEME_METRIC_LABEL_FONT_SIZE_SP:
    *out = th.label_font_size_sp;
    return CGFX_OK;
  case CGFX_THEME_METRIC_BUTTON_FONT_SIZE_SP:
    *out = th.button_font_size_sp;
    return CGFX_OK;
  case CGFX_THEME_METRIC_SPACING_UNIT_PX:
    *out = th.spacing_unit_px;
    return CGFX_OK;
  case CGFX_THEME_METRIC_PADDING_SM_PX:
    *out = th.padding_sm_px;
    return CGFX_OK;
  case CGFX_THEME_METRIC_PADDING_MD_PX:
    *out = th.padding_md_px;
    return CGFX_OK;
  default:
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
}

void rgba_to_c(const cgfx::RgbaNormalized &c, cgfx_color_rgba *out) noexcept {
  out->r = c.r;
  out->g = c.g;
  out->b = c.b;
  out->a = c.a;
}

cgfx::ButtonFaceQueryScenario
map_button_face_query(cgfx_button_face_query_scenario scenario) noexcept {
  switch (scenario) {
  case CGFX_BUTTON_FACE_QUERY_NORMAL:
    return cgfx::ButtonFaceQueryScenario::Normal;
  case CGFX_BUTTON_FACE_QUERY_HOVERED:
    return cgfx::ButtonFaceQueryScenario::Hovered;
  case CGFX_BUTTON_FACE_QUERY_PRESSED:
    return cgfx::ButtonFaceQueryScenario::Pressed;
  case CGFX_BUTTON_FACE_QUERY_DISABLED:
    return cgfx::ButtonFaceQueryScenario::Disabled;
  default:
    return cgfx::ButtonFaceQueryScenario::Normal;
  }
}

cgfx::UiTheme *unwrap_theme_mut(cgfx_theme *t) noexcept {
  return reinterpret_cast<cgfx::UiTheme *>(t);
}

const cgfx::UiTheme *unwrap_theme_const(const cgfx_theme *t) noexcept {
  return reinterpret_cast<const cgfx::UiTheme *>(t);
}

bool window_widget_alive_ok(cgfx::CgfxWindow *w, cgfx_widget_id id) noexcept {
  if (!w || id == CGFX_WIDGET_ID_NONE) {
    return false;
  }
  size_t ix{};
  return w->widget_tree().index_of_widget(id, ix) == CGFX_OK;
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

void cgfx_context_set_input_routing_trace_enabled(cgfx_context *context,
                                                  bool enabled) {
  if (!context) {
    return;
  }
  cgfx::CgfxContext::from_opaque(context)
      ->set_input_routing_trace_enabled(enabled);
}

bool cgfx_context_get_input_routing_trace_enabled(const cgfx_context *context) {
  if (!context) {
    return false;
  }
  return cgfx::CgfxContext::from_opaque(const_cast<cgfx_context *>(context))
      ->input_routing_trace_enabled();
}

bool cgfx_next_event(cgfx_context *context, cgfx_event_type *type,
                     cgfx_window **window_handle, void *event_payload_bytes,
                     size_t payload_capacity, size_t *out_payload_used) {
  if (!context || !type || !window_handle) {
    return false;
  }

  cgfx::CgfxContext *ctx_impl = cgfx::CgfxContext::from_opaque(context);
  cgfx::InternalEvent iev{};
  return dequeue_event_copy_payload_try(ctx_impl, iev, type, window_handle,
                                        event_payload_bytes, payload_capacity,
                                        out_payload_used, nullptr);
}

bool cgfx_next_event_with_sequence(cgfx_context *context, cgfx_event_type *type,
                                   cgfx_window **window_handle,
                                   void *event_payload_bytes,
                                   size_t payload_capacity,
                                   size_t *out_payload_used,
                                   uint64_t *out_sequence) {
  if (!context || !type || !window_handle) {
    return false;
  }

  cgfx::CgfxContext *ctx_impl = cgfx::CgfxContext::from_opaque(context);

  cgfx::InternalEvent iev{};
  if (out_sequence) {
    *out_sequence = 0;
  }
  return dequeue_event_copy_payload_try(ctx_impl, iev, type, window_handle,
                                        event_payload_bytes, payload_capacity,
                                        out_payload_used, out_sequence);
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
  w->widget_style_overrides_mut().purge_subtree(w->widget_tree(), widget_id);
  w->basic_widgets_mut().purge_subtree(w->widget_tree(), widget_id);
  w->animations_mut().purge_subtree(w->widget_tree(), widget_id);
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

static cgfx_basic_widget_kind to_public_basic_kind(
    cgfx::BasicWidgetKind k) noexcept {
  switch (k) {
  case cgfx::BasicWidgetKind::Panel:
    return CGFX_BASIC_WIDGET_KIND_PANEL;
  case cgfx::BasicWidgetKind::Label:
    return CGFX_BASIC_WIDGET_KIND_LABEL;
  case cgfx::BasicWidgetKind::Button:
    return CGFX_BASIC_WIDGET_KIND_BUTTON;
  default:
    return CGFX_BASIC_WIDGET_KIND_NONE;
  }
}

cgfx_result cgfx_basic_widget_kind_of(cgfx_window *window,
                                      cgfx_widget_id widget_id,
                                      cgfx_basic_widget_kind *out_kind) {
  if (!window || !out_kind) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::BasicWidgetKind bk{};
  const bool has = cgfx::CgfxWindow::from_opaque(window)
                       ->basic_widgets()
                       .try_get_kind(widget_id, &bk);
  *out_kind = has ? to_public_basic_kind(bk) : CGFX_BASIC_WIDGET_KIND_NONE;
  return CGFX_OK;
}

cgfx_result cgfx_basic_widget_panel_create(cgfx_window *window,
                                           cgfx_widget_id parent_id,
                                           cgfx_widget_id *out_id) {
  if (!window || !out_id) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  return w->basic_widgets_mut().create_panel(w->widget_tree_mut(), parent_id,
                                             out_id);
}

cgfx_result cgfx_basic_widget_label_create(cgfx_window *window,
                                           cgfx_widget_id parent_id,
                                           cgfx_widget_id *out_id) {
  if (!window || !out_id) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  return w->basic_widgets_mut().create_label(w->widget_tree_mut(), parent_id,
                                             out_id);
}

cgfx_result cgfx_basic_widget_button_create(cgfx_window *window,
                                            cgfx_widget_id parent_id,
                                            cgfx_widget_id *out_id) {
  if (!window || !out_id) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  return w->basic_widgets_mut().create_button(w->widget_tree_mut(), parent_id,
                                              out_id);
}

cgfx_result cgfx_window_draw_basic_widgets(cgfx_window *window) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)->draw_basic_widgets();
}

cgfx_result cgfx_basic_widget_set_visible(cgfx_window *window,
                                          cgfx_widget_id widget_id,
                                          int visible) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)
      ->basic_widgets_mut()
      .set_visible(widget_id, visible != 0);
}

cgfx_result cgfx_basic_widget_get_visible(cgfx_window *window,
                                          cgfx_widget_id widget_id,
                                          int *out_visible) {
  if (!window || !out_visible) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  bool v{};
  const cgfx_result r = cgfx::CgfxWindow::from_opaque(window)
                            ->basic_widgets()
                            .get_visible(widget_id, &v);
  *out_visible = v ? 1 : 0;
  return r;
}

cgfx_result cgfx_basic_widget_panel_set_background_rgba_normalized(
    cgfx_window *window, cgfx_widget_id panel_id, float r, float g, float b,
    float a) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  /* Phase 5 facet path; see header for precedence vs `cgfx_widget_style_set_panel_background_*`. */
  return cgfx::CgfxWindow::from_opaque(window)
      ->basic_widgets_mut()
      .set_panel_background_rgba(panel_id, r, g, b, a);
}

cgfx_result cgfx_basic_widget_button_set_enabled(cgfx_window *window,
                                                 cgfx_widget_id button_id,
                                                 int enabled) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)
      ->basic_widgets_mut()
      .set_button_enabled(button_id, enabled != 0);
}

cgfx_result cgfx_basic_widget_button_get_enabled(cgfx_window *window,
                                                 cgfx_widget_id button_id,
                                                 int *out_enabled) {
  if (!window || !out_enabled) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  bool e{};
  const cgfx_result r = cgfx::CgfxWindow::from_opaque(window)
                            ->basic_widgets()
                            .get_button_enabled(button_id, &e);
  *out_enabled = e ? 1 : 0;
  return r;
}

cgfx_result cgfx_basic_widget_utf8_text_set(cgfx_window *window,
                                            cgfx_widget_id widget_id,
                                            const char *utf8_bytes,
                                            size_t utf8_byte_length) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  cgfx::BasicWidgetKind kk{};
  if (!w->basic_widgets().try_get_kind(widget_id, &kk)) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  if (kk != cgfx::BasicWidgetKind::Label && kk != cgfx::BasicWidgetKind::Button) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  size_t nbytes = utf8_byte_length;
  if (utf8_bytes && nbytes == 0U) {
    nbytes = std::strlen(utf8_bytes);
  }

  return w->basic_widgets_mut().set_utf8_text(widget_id, kk, utf8_bytes,
                                              nbytes);
}

cgfx_result cgfx_basic_widget_utf8_text_get_length(cgfx_window *window,
                                                   cgfx_widget_id widget_id,
                                                   size_t *out_byte_length) {
  if (!window || !out_byte_length) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  cgfx::BasicWidgetKind kk{};
  if (!w->basic_widgets().try_get_kind(widget_id, &kk)) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  if (kk != cgfx::BasicWidgetKind::Label && kk != cgfx::BasicWidgetKind::Button) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return w->basic_widgets().utf8_text_byte_length(widget_id, kk,
                                                 out_byte_length);
}

cgfx_result cgfx_basic_widget_utf8_text_get(cgfx_window *window,
                                          cgfx_widget_id widget_id,
                                          char *out_bytes,
                                          size_t out_capacity,
                                          size_t *out_written_including_null) {
  if (!window || !out_written_including_null) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  cgfx::BasicWidgetKind kk{};
  if (!w->basic_widgets().try_get_kind(widget_id, &kk)) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  if (kk != cgfx::BasicWidgetKind::Label && kk != cgfx::BasicWidgetKind::Button) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return w->basic_widgets().get_utf8_text(widget_id, kk, out_bytes,
                                         out_capacity, out_written_including_null);
}

cgfx_result cgfx_theme_create(cgfx_theme **out_theme) {
  if (!out_theme) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  *out_theme = nullptr;
  try {
    auto *p = new cgfx::UiTheme(cgfx::UiTheme::make_phase5_builtin());
    *out_theme = reinterpret_cast<cgfx_theme *>(p);
    return CGFX_OK;
  } catch (...) {
    return CGFX_ERROR_OUT_OF_MEMORY;
  }
}

void cgfx_theme_destroy(cgfx_theme *theme) {
  delete unwrap_theme_mut(theme);
}

cgfx_result cgfx_theme_reset_to_defaults(cgfx_theme *theme) {
  if (!theme) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  *unwrap_theme_mut(theme) = cgfx::UiTheme::make_phase5_builtin();
  return CGFX_OK;
}

cgfx_result cgfx_theme_set_color_rgba(cgfx_theme *theme, cgfx_theme_color_token token,
                                      const cgfx_color_rgba *color) {
  if (!theme || !color) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::RgbaNormalized *slot = mutable_theme_color(*unwrap_theme_mut(theme), token);
  if (!slot) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  slot->r = color->r;
  slot->g = color->g;
  slot->b = color->b;
  slot->a = color->a;
  return CGFX_OK;
}

cgfx_result cgfx_theme_get_color_rgba(const cgfx_theme *theme,
                                      cgfx_theme_color_token token,
                                      cgfx_color_rgba *out_color) {
  if (!theme || !out_color) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  const cgfx::RgbaNormalized *slot =
      const_theme_color(*unwrap_theme_const(theme), token);
  if (!slot) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  rgba_to_c(*slot, out_color);
  return CGFX_OK;
}

cgfx_result cgfx_theme_set_metric(cgfx_theme *theme, cgfx_theme_metric_token token,
                                  float metric) {
  if (!theme) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return set_theme_metric(*unwrap_theme_mut(theme), token, metric);
}

cgfx_result cgfx_theme_get_metric(const cgfx_theme *theme,
                                  cgfx_theme_metric_token token, float *out_metric) {
  if (!theme) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return get_theme_metric(*unwrap_theme_const(theme), token, out_metric);
}

cgfx_result cgfx_window_theme_reset_to_defaults(cgfx_window *window) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  w->ui_theme_mut() = cgfx::UiTheme::make_phase5_builtin();
  return CGFX_OK;
}

cgfx_result cgfx_window_theme_set_color_rgba(
    cgfx_window *window, cgfx_theme_color_token token,
    const cgfx_color_rgba *color) {
  if (!window || !color) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::RgbaNormalized *slot =
      mutable_theme_color(cgfx::CgfxWindow::from_opaque(window)->ui_theme_mut(), token);
  if (!slot) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  slot->r = color->r;
  slot->g = color->g;
  slot->b = color->b;
  slot->a = color->a;
  return CGFX_OK;
}

cgfx_result cgfx_window_theme_get_color_rgba(const cgfx_window *window,
                                             cgfx_theme_color_token token,
                                             cgfx_color_rgba *out_color) {
  if (!window || !out_color) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  const cgfx::RgbaNormalized *slot = const_theme_color(
      cgfx::CgfxWindow::from_opaque(const_cast<cgfx_window *>(window))->ui_theme(),
      token);
  if (!slot) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  rgba_to_c(*slot, out_color);
  return CGFX_OK;
}

cgfx_result cgfx_window_theme_set_metric(cgfx_window *window,
                                         cgfx_theme_metric_token token,
                                         float metric) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return set_theme_metric(cgfx::CgfxWindow::from_opaque(window)->ui_theme_mut(), token,
                          metric);
}

cgfx_result cgfx_window_theme_get_metric(const cgfx_window *window,
                                         cgfx_theme_metric_token token,
                                         float *out_metric) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return get_theme_metric(
      cgfx::CgfxWindow::from_opaque(const_cast<cgfx_window *>(window))->ui_theme(), token,
      out_metric);
}

cgfx_result cgfx_window_theme_apply(cgfx_window *window, const cgfx_theme *theme) {
  if (!window || !theme) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow::from_opaque(window)->ui_theme_mut() = *unwrap_theme_const(theme);
  return CGFX_OK;
}

cgfx_result cgfx_window_theme_copy_to(const cgfx_window *window,
                                      cgfx_theme *out_theme) {
  if (!window || !out_theme) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  *unwrap_theme_mut(out_theme) =
      cgfx::CgfxWindow::from_opaque(const_cast<cgfx_window *>(window))->ui_theme();
  return CGFX_OK;
}

cgfx_result cgfx_widget_style_set_panel_background_rgba_normalized(
    cgfx_window *window, cgfx_widget_id widget_id, float r, float g, float b, float a) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  /* Canonical Phase 6 panel background; see `cgfx_basic_widget_panel_set_background_*` for facet path. */
  return cgfx::CgfxWindow::from_opaque(window)
      ->widget_style_overrides_mut()
      .set_panel_background(widget_id, r, g, b, a);
}

cgfx_result cgfx_widget_style_set_label_placeholder_rgba_normalized(
    cgfx_window *window, cgfx_widget_id label_id, float r, float g, float b, float a) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)
      ->widget_style_overrides_mut()
      .set_label_placeholder_color(label_id, r, g, b, a);
}

cgfx_result cgfx_widget_style_set_label_text_placeholder_rgba_normalized(
    cgfx_window *window, cgfx_widget_id label_id, float r, float g, float b, float a) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)
      ->widget_style_overrides_mut()
      .set_label_text_placeholder(label_id, r, g, b, a);
}

cgfx_result cgfx_widget_style_set_label_font_size_sp_placeholder(
    cgfx_window *window, cgfx_widget_id label_id, float font_size_sp) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)
      ->widget_style_overrides_mut()
      .set_label_font_size_sp(label_id, font_size_sp);
}

cgfx_result cgfx_widget_style_set_button_background_normal_rgba_normalized(
    cgfx_window *window, cgfx_widget_id button_id, float r, float g, float b, float a) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)
      ->widget_style_overrides_mut()
      .set_button_background_normal(button_id, r, g, b, a);
}

cgfx_result cgfx_widget_style_set_button_background_hover_rgba_normalized(
    cgfx_window *window, cgfx_widget_id button_id, float r, float g, float b, float a) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)
      ->widget_style_overrides_mut()
      .set_button_background_hover(button_id, r, g, b, a);
}

cgfx_result cgfx_widget_style_set_button_background_pressed_rgba_normalized(
    cgfx_window *window, cgfx_widget_id button_id, float r, float g, float b, float a) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)
      ->widget_style_overrides_mut()
      .set_button_background_pressed(button_id, r, g, b, a);
}

cgfx_result cgfx_widget_style_set_button_background_disabled_rgba_normalized(
    cgfx_window *window, cgfx_widget_id button_id, float r, float g, float b, float a) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)
      ->widget_style_overrides_mut()
      .set_button_background_disabled(button_id, r, g, b, a);
}

cgfx_result cgfx_widget_style_set_button_caption_placeholder_rgba_normalized(
    cgfx_window *window, cgfx_widget_id button_id, float r, float g, float b, float a) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)
      ->widget_style_overrides_mut()
      .set_button_caption_placeholder(button_id, r, g, b, a);
}

cgfx_result cgfx_widget_style_set_button_text_placeholder_rgba_normalized(
    cgfx_window *window, cgfx_widget_id button_id, float r, float g, float b, float a) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)
      ->widget_style_overrides_mut()
      .set_button_text_placeholder(button_id, r, g, b, a);
}

cgfx_result cgfx_widget_style_set_button_font_size_sp_placeholder(
    cgfx_window *window, cgfx_widget_id button_id, float font_size_sp) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxWindow::from_opaque(window)
      ->widget_style_overrides_mut()
      .set_button_font_size_sp(button_id, font_size_sp);
}

cgfx_result cgfx_widget_style_clear_overrides(cgfx_window *window,
                                              cgfx_widget_id widget_id,
                                              uint32_t mask_bits) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow::from_opaque(window)
      ->widget_style_overrides_mut()
      .clear(widget_id, mask_bits);
  return CGFX_OK;
}

cgfx_result cgfx_widget_style_clear_all_overrides(cgfx_window *window,
                                                cgfx_widget_id widget_id) {
  if (!window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow::from_opaque(window)
      ->widget_style_overrides_mut()
      .clear_all(widget_id);
  return CGFX_OK;
}

cgfx_result cgfx_widget_style_query_resolved_panel_background_rgba_normalized(
    cgfx_window *window, cgfx_widget_id panel_id, cgfx_color_rgba *out_color) {
  if (!window || !out_color) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  float r{}, g{}, b{}, a{};
  const cgfx_result rc =
      w->basic_widgets().query_resolved_panel_background_rgba_normalized(
          panel_id, w->ui_theme(), w->widget_style_overrides(), &r, &g, &b, &a);
  if (rc != CGFX_OK) {
    return rc;
  }
  out_color->r = r;
  out_color->g = g;
  out_color->b = b;
  out_color->a = a;
  return CGFX_OK;
}

cgfx_result cgfx_widget_style_query_resolved_label_placeholder_rgba_normalized(
    cgfx_window *window, cgfx_widget_id label_id, cgfx_color_rgba *out_color) {
  if (!window || !out_color) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  float r{}, g{}, b{}, a{};
  const cgfx_result rc =
      w->basic_widgets().query_resolved_label_placeholder_rgba_normalized(
          label_id, w->ui_theme(), w->widget_style_overrides(), &r, &g, &b, &a);
  if (rc != CGFX_OK) {
    return rc;
  }
  out_color->r = r;
  out_color->g = g;
  out_color->b = b;
  out_color->a = a;
  return CGFX_OK;
}

cgfx_result
cgfx_widget_style_query_resolved_label_text_color_placeholder_rgba_normalized(
    cgfx_window *window, cgfx_widget_id label_id, cgfx_color_rgba *out_color) {
  if (!window || !out_color) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  float r{}, g{}, b{}, a{};
  const cgfx_result rc =
      w->basic_widgets().query_resolved_label_text_color_placeholder_rgba_normalized(
          label_id, w->ui_theme(), w->widget_style_overrides(), &r, &g, &b, &a);
  if (rc != CGFX_OK) {
    return rc;
  }
  out_color->r = r;
  out_color->g = g;
  out_color->b = b;
  out_color->a = a;
  return CGFX_OK;
}

cgfx_result cgfx_widget_style_query_resolved_label_font_size_sp_placeholder(
    cgfx_window *window, cgfx_widget_id label_id, float *out_font_size_sp) {
  if (!window || !out_font_size_sp) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  return w->basic_widgets().query_resolved_label_font_size_sp_placeholder(
      label_id, w->ui_theme(), w->widget_style_overrides(), out_font_size_sp);
}

cgfx_result cgfx_widget_style_query_resolved_button_face_rgba_normalized(
    cgfx_window *window, cgfx_widget_id button_id,
    cgfx_button_face_query_scenario scenario, cgfx_color_rgba *out_color) {
  if (!window || !out_color) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  float r{}, g{}, b{}, a{};
  const cgfx_result rc =
      w->basic_widgets().query_resolved_button_face_rgba_normalized(
          button_id, w->ui_theme(), w->widget_style_overrides(),
          map_button_face_query(scenario), &r, &g, &b, &a);
  if (rc != CGFX_OK) {
    return rc;
  }
  out_color->r = r;
  out_color->g = g;
  out_color->b = b;
  out_color->a = a;
  return CGFX_OK;
}

cgfx_result
cgfx_widget_style_query_resolved_button_caption_placeholder_rgba_normalized(
    cgfx_window *window, cgfx_widget_id button_id, cgfx_color_rgba *out_color) {
  if (!window || !out_color) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  float r{}, g{}, b{}, a{};
  const cgfx_result rc =
      w->basic_widgets().query_resolved_button_caption_rgba_normalized(
          button_id, w->ui_theme(), w->widget_style_overrides(), &r, &g, &b, &a);
  if (rc != CGFX_OK) {
    return rc;
  }
  out_color->r = r;
  out_color->g = g;
  out_color->b = b;
  out_color->a = a;
  return CGFX_OK;
}

cgfx_result
cgfx_widget_style_query_resolved_button_text_placeholder_rgba_normalized(
    cgfx_window *window, cgfx_widget_id button_id, cgfx_color_rgba *out_color) {
  if (!window || !out_color) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  float r{}, g{}, b{}, a{};
  const cgfx_result rc =
      w->basic_widgets().query_resolved_button_text_color_placeholder_rgba_normalized(
          button_id, w->ui_theme(), w->widget_style_overrides(), &r, &g, &b, &a);
  if (rc != CGFX_OK) {
    return rc;
  }
  out_color->r = r;
  out_color->g = g;
  out_color->b = b;
  out_color->a = a;
  return CGFX_OK;
}

cgfx_result cgfx_widget_style_query_resolved_button_font_size_sp_placeholder(
    cgfx_window *window, cgfx_widget_id button_id, float *out_font_size_sp) {
  if (!window || !out_font_size_sp) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  return w->basic_widgets().query_resolved_button_font_size_sp_placeholder(
      button_id, w->ui_theme(), w->widget_style_overrides(), out_font_size_sp);
}

cgfx_result cgfx_font_builtin_acquire(const cgfx_context *context,
                                      cgfx_font_builtin_kind kind,
                                      cgfx_font_id *out_font_id) {
  if (!out_font_id) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  *out_font_id = CGFX_FONT_ID_INVALID;
  if (!context) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  if (kind != CGFX_FONT_BUILTIN_MONO_STUB) {
    return CGFX_ERROR_UNSUPPORTED;
  }
  *out_font_id = CGFX_FONT_ID_BUILTIN_DEFAULT;
  return CGFX_OK;
}

cgfx_result cgfx_font_builtin_acquire_mono_stub(const cgfx_context *context,
                                                cgfx_font_id *out_font_id) {
  return cgfx_font_builtin_acquire(context, CGFX_FONT_BUILTIN_MONO_STUB,
                                   out_font_id);
}

cgfx_result cgfx_context_text_font_select(cgfx_context *context,
                                          cgfx_font_id font_id) {
  if (!context) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return cgfx::CgfxContext::from_opaque(context)->set_selected_text_font(font_id);
}

cgfx_result cgfx_context_text_font_selected(const cgfx_context *context,
                                            cgfx_font_id *out_font_id) {
  if (!context || !out_font_id) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  *out_font_id =
      cgfx::CgfxContext::from_opaque(const_cast<cgfx_context *>(context))
          ->selected_text_font();
  return CGFX_OK;
}

cgfx_result cgfx_context_set_text_font(cgfx_context *context,
                                       cgfx_font_id font_id) {
  return cgfx_context_text_font_select(context, font_id);
}

cgfx_result cgfx_context_get_text_font(const cgfx_context *context,
                                       cgfx_font_id *out_font_id) {
  return cgfx_context_text_font_selected(context, out_font_id);
}

cgfx_result cgfx_text_measure_utf8_line_pixels(const cgfx_context *context,
                                             cgfx_font_id font_id,
                                             const char *utf8_bytes,
                                             size_t utf8_byte_length,
                                             float font_size_sp, float dpi_scale,
                                             cgfx_text_line_metrics *out_metrics) {
  if (!context || !out_metrics) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  std::memset(out_metrics, 0, sizeof(*out_metrics));
  if (!std::isfinite(font_size_sp) || font_size_sp <= 0.f) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  const char *bytes = utf8_bytes;
  size_t nbytes = utf8_byte_length;
  if (!bytes) {
    if (nbytes != 0U) {
      return CGFX_ERROR_INVALID_ARGUMENT;
    }
    bytes = "";
  } else if (nbytes == 0U) {
    nbytes = std::strlen(bytes);
  }
  cgfx::CgfxContext *ctx =
      cgfx::CgfxContext::from_opaque(const_cast<cgfx_context *>(context));
  cgfx_font_id fid = font_id;
  if (fid == CGFX_FONT_ID_INVALID) {
    fid = ctx->selected_text_font();
  }
  if (!ctx->font_registry().is_valid(fid)) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  float d = dpi_scale;
  if (!std::isfinite(d) || d <= 0.f) {
    d = 1.f;
  }
  const uint32_t fpx = cgfx::text_logical_font_px_round(font_size_sp, d);
  if (fpx == 0U) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  const cgfx::TextLineMetrics m =
      cgfx::text_measure_utf8_line_stub(fpx, bytes, nbytes);
  out_metrics->width_px = m.width_px;
  out_metrics->ascent_px = m.ascent_px;
  out_metrics->descent_px = m.descent_px;
  out_metrics->line_height_px = m.line_height_px;
  out_metrics->logical_font_px = fpx;
  return CGFX_OK;
}

cgfx_result cgfx_text_measure_utf8_line_cstr_pixels(
    const cgfx_context *context, cgfx_font_id font_id, const char *utf8_cstring,
    float font_size_sp, float dpi_scale, cgfx_text_line_metrics *out_metrics) {
  return cgfx_text_measure_utf8_line_pixels(context, font_id, utf8_cstring, 0,
                                            font_size_sp, dpi_scale, out_metrics);
}

void cgfx_context_set_animation_speed_scale(cgfx_context *context, float scale) {
  if (!context) {
    return;
  }
  cgfx::CgfxContext::from_opaque(context)->set_animation_speed_scale(scale);
}

float cgfx_context_get_animation_speed_scale(const cgfx_context *context) {
  if (!context) {
    return 1.f;
  }
  return cgfx::CgfxContext::from_opaque(const_cast<cgfx_context *>(context))
      ->animation_speed_scale_value();
}

void cgfx_context_animation_set_manual_clock_enabled(cgfx_context *context,
                                                      int enabled) {
  if (!context) {
    return;
  }
  cgfx::CgfxContext::from_opaque(context)->animation_clock_mut().set_manual_mode(
      enabled != 0);
}

int cgfx_context_animation_get_manual_clock_enabled(const cgfx_context *context) {
  if (!context) {
    return 0;
  }
  return cgfx::CgfxContext::from_opaque(const_cast<cgfx_context *>(context))
             ->animation_clock()
             .manual_mode()
      ? 1
      : 0;
}

void cgfx_context_animation_clock_set_seconds(cgfx_context *context, double seconds) {
  if (!context) {
    return;
  }
  cgfx::CgfxContext::from_opaque(context)->animation_clock_mut().set_manual_time_seconds(
      seconds);
}

double cgfx_context_animation_clock_get_seconds(const cgfx_context *context) {
  if (!context) {
    return 0.;
  }
  return cgfx::CgfxContext::from_opaque(const_cast<cgfx_context *>(context))
      ->animation_clock()
      .now_seconds();
}

void cgfx_context_animation_manual_advance_seconds(cgfx_context *context,
                                                     double dt_seconds) {
  if (!context) {
    return;
  }
  cgfx::CgfxContext::from_opaque(context)->animation_clock_mut().manual_advance_seconds(
      dt_seconds);
}

cgfx_result cgfx_animation_start_translate_logical_px(
    cgfx_window *window, cgfx_widget_id widget_id, float start_x_px, float start_y_px,
    float end_x_px, float end_y_px, float duration_seconds, cgfx_anim_ease ease,
    cgfx_animation_id *out_animation_id) {
  if (!window || !out_animation_id) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  if (!window_widget_alive_ok(w, widget_id)) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return w->animations_mut().start_translate(widget_id, start_x_px, start_y_px, end_x_px,
                                             end_y_px, duration_seconds, static_cast<int>(ease),
                                             out_animation_id);
}

cgfx_result cgfx_animation_start_opacity(
    cgfx_window *window, cgfx_widget_id widget_id, float start_alpha, float end_alpha,
    float duration_seconds, cgfx_anim_ease ease, cgfx_animation_id *out_animation_id) {
  if (!window || !out_animation_id) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  if (!window_widget_alive_ok(w, widget_id)) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return w->animations_mut().start_opacity(widget_id, start_alpha, end_alpha,
                                            duration_seconds, static_cast<int>(ease),
                                            out_animation_id);
}

cgfx_result cgfx_animation_start_fill_rgba(cgfx_window *window, cgfx_widget_id widget_id,
                                          const cgfx_color_rgba *start_color,
                                          const cgfx_color_rgba *end_color,
                                          float duration_seconds, cgfx_anim_ease ease,
                                          cgfx_animation_id *out_animation_id) {
  if (!window || !out_animation_id || !start_color || !end_color) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx::CgfxWindow *w = cgfx::CgfxWindow::from_opaque(window);
  if (!window_widget_alive_ok(w, widget_id)) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return w->animations_mut().start_fill_rgba(
      widget_id, start_color->r, start_color->g, start_color->b, start_color->a,
      end_color->r, end_color->g, end_color->b, end_color->a, duration_seconds,
      static_cast<int>(ease), out_animation_id);
}

void cgfx_animation_stop(cgfx_window *window, cgfx_animation_id animation_id) {
  if (!window) {
    return;
  }
  cgfx::CgfxWindow::from_opaque(window)->animations_mut().stop(animation_id);
}

void cgfx_animation_stop_widget_property(cgfx_window *window, cgfx_widget_id widget_id,
                                          cgfx_anim_property property) {
  if (!window) {
    return;
  }
  cgfx::CgfxWindow::from_opaque(window)->animations_mut().stop_widget_property(
      widget_id, static_cast<int>(property));
}

int cgfx_animation_is_active(cgfx_window *window, cgfx_animation_id animation_id) {
  if (!window) {
    return 0;
  }
  return cgfx::CgfxWindow::from_opaque(window)->animations_mut().is_active(animation_id)
      ? 1
      : 0;
}

} // extern "C"

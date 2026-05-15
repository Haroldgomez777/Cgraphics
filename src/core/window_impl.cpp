#include "core/window_impl.hpp"
#include "core/context.hpp"
#include "layout/flex_layout.hpp"

namespace cgfx {

CgfxWindow::CgfxWindow(CgfxContext *ctx)
    : ctx_(ctx),
      focus_widget_id_raw_(widget_tree_.root_id()),
      input_propagation_policy_(ctx
                                    ? ctx->default_input_propagation_policy()
                                    : CGFX_INPUT_PROPAGATION_TARGET_ONLY) {}

CgfxWindow::~CgfxWindow() { shutdown(); }

cgfx_result CgfxWindow::initialize(const cgfx_window_desc *desc) {
  if (!ctx_ || !desc) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  cgfx_result status = CGFX_OK;
  surface_ = ctx_->platform_backend()->create_surface(this, desc, status);
  if (!surface_) {
    return status != CGFX_OK ? status : CGFX_ERROR_PLATFORM;
  }
  render_device_ = cgfx_make_opengl_render_device();
  if (!render_device_) {
    surface_->teardown();
    surface_.reset();
    return CGFX_ERROR_PLATFORM;
  }
  presenting_ = false;
  return status;
}

void CgfxWindow::shutdown() {
  if (presenting_) {
    end_present_pass();
  }
  render_device_.reset();
  if (surface_) {
    surface_->teardown();
    surface_.reset();
  }
}

cgfx_result CgfxWindow::begin_present_pass(uint32_t *out_width_px,
                                           uint32_t *out_height_px,
                                           float *out_dpi_scale) {
  if (!surface_ || !render_device_) {
    return CGFX_ERROR_PLATFORM;
  }
  if (presenting_) {
    return CGFX_ERROR_PLATFORM;
  }

  RenderFrameInfo frame{};
  cgfx_result rc =
      surface_->begin_present(&frame.width_px, &frame.height_px, &frame.dpi_scale);
  if (rc != CGFX_OK) {
    return rc;
  }

  run_flex_layout(widget_tree_, frame.width_px, frame.height_px);

  rc = render_device_->begin_frame(frame);
  if (rc != CGFX_OK) {
    surface_->end_present();
    return rc;
  }

  command_list_.reset();
  presenting_ = true;

  if (out_width_px) {
    *out_width_px = frame.width_px;
  }
  if (out_height_px) {
    *out_height_px = frame.height_px;
  }
  if (out_dpi_scale) {
    *out_dpi_scale = frame.dpi_scale;
  }

  return CGFX_OK;
}

cgfx_result CgfxWindow::clear_present_surface(float red, float green, float blue,
                                              float alpha) {
  if (!presenting_) {
    return CGFX_ERROR_PLATFORM;
  }
  return command_list_.append_clear_color(red, green, blue, alpha);
}

cgfx_result CgfxWindow::fill_rect_present_surface(int32_t x_px, int32_t y_px,
                                                  uint32_t width_px,
                                                  uint32_t height_px, float red,
                                                  float green, float blue,
                                                  float alpha) {
  if (!presenting_) {
    return CGFX_ERROR_PLATFORM;
  }
  if (width_px == 0U || height_px == 0U) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return command_list_.append_fill_rect(x_px, y_px, width_px, height_px, red,
                                        green, blue, alpha);
}

cgfx_result CgfxWindow::fill_rect_batch_present_surface(const void *items,
                                                        size_t item_count,
                                                        size_t stride_bytes) {
  if (!presenting_) {
    return CGFX_ERROR_PLATFORM;
  }
  return command_list_.append_fill_rect_batch_interleaved(items, item_count,
                                                          stride_bytes);
}

void CgfxWindow::end_present_pass() {
  if (!presenting_) {
    return;
  }
  if (render_device_) {
    (void)render_device_->submit(command_list_);
    render_device_->end_frame();
  }
  if (surface_) {
    surface_->end_present();
  }
  command_list_.reset();
  presenting_ = false;
}

void CgfxWindow::sync_widget_layout_logical_from_surface() noexcept {
  if (!surface_) {
    return;
  }
  uint32_t w = 0U;
  uint32_t h = 0U;
  if (surface_->query_size_px(&w, &h) != CGFX_OK) {
    return;
  }
  run_flex_layout(widget_tree_mut(), w, h);
}

cgfx_result CgfxWindow::assign_focus_widget_logical(cgfx_widget_id id) noexcept {
  const cgfx_widget_id root = widget_tree_.root_id();
  if (id == CGFX_WIDGET_ID_NONE || id == root) {
    focus_widget_id_raw_ = root;
    return CGFX_OK;
  }
  size_t ignore{};
  if (widget_tree_.index_of_widget(id, ignore) != CGFX_OK) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  focus_widget_id_raw_ = id;
  return CGFX_OK;
}

void CgfxWindow::reconcile_focus_after_structure_change() noexcept {
  focus_widget_id_raw_ =
      widget_tree_.validated_widget_id_or_root(focus_widget_id_raw_);
}

void routing_sync_pick_mouse_move_targets(CgfxWindow *w,
                                          cgfx_event_mouse_move_payload *p) {
  if (!w || !p) {
    return;
  }
  w->sync_widget_layout_logical_from_surface();
  p->target_widget = w->widget_tree().hit_test_logical(p->x, p->y);
}

void routing_sync_pick_mouse_button_targets(
    CgfxWindow *w, cgfx_event_mouse_button_payload *p) {
  if (!w || !p) {
    return;
  }
  w->sync_widget_layout_logical_from_surface();
  const cgfx_widget_id hit = w->widget_tree().hit_test_logical(p->x, p->y);
  p->target_widget = hit;

  /** Primary-pointer press adopts focus for routed key stream (Phase 4.1). */
  if (p->action == CGFX_PRESS && hit != CGFX_WIDGET_ID_NONE) {
    (void)w->assign_focus_widget_logical(hit);
  }
}

void routing_pick_key_focus_targets(CgfxWindow *w,
                                    cgfx_event_key_payload *p) {
  if (!w || !p) {
    return;
  }
  p->target_widget = w->resolved_focus_widget_id();
}

} // namespace cgfx

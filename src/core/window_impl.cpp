#include "core/window_impl.hpp"
#include "core/context.hpp"

namespace cgfx {

CgfxWindow::CgfxWindow(CgfxContext *ctx) : ctx_(ctx) {}

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

  rc = render_device_->begin_frame(frame);
  if (rc != CGFX_OK) {
    surface_->end_present();
    return rc;
  }

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
  if (!presenting_ || !render_device_) {
    return CGFX_ERROR_PLATFORM;
  }
  return render_device_->clear_normalized_rgba(red, green, blue, alpha);
}

void CgfxWindow::end_present_pass() {
  if (!presenting_) {
    return;
  }
  if (render_device_) {
    render_device_->end_frame();
  }
  if (surface_) {
    surface_->end_present();
  }
  presenting_ = false;
}

} // namespace cgfx

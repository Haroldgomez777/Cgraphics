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
  return status;
}

void CgfxWindow::shutdown() {
  if (!surface_) {
    return;
  }
  surface_->teardown();
  surface_.reset();
}

} // namespace cgfx

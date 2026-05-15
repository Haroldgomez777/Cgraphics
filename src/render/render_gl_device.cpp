#include "render/render_device.hpp"

#include <cmath>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#elif defined(__linux__)
#include <GL/gl.h>
#endif

namespace cgfx {

namespace {

float clamp_unit(float value) noexcept {
  if (std::isnan(value)) {
    return 0.0f;
  }
  if (value < 0.0f) {
    return 0.0f;
  }
  if (value > 1.0f) {
    return 1.0f;
  }
  return value;
}

class OpenGlRenderDevice final : public RenderDevice {
public:
  OpenGlRenderDevice() = default;
  ~OpenGlRenderDevice() override = default;

  cgfx_result begin_frame(const RenderFrameInfo &frame) override {
    const int width_px = static_cast<int>(frame.width_px);
    const int height_px = static_cast<int>(frame.height_px);
    if (width_px <= 0 || height_px <= 0) {
      return CGFX_ERROR_INVALID_ARGUMENT;
    }
    glViewport(0, 0, width_px, height_px);
    return CGFX_OK;
  }

  cgfx_result clear_normalized_rgba(float r, float g, float b,
                                    float a) override {
    glClearColor(clamp_unit(r), clamp_unit(g), clamp_unit(b), clamp_unit(a));
    glClear(GL_COLOR_BUFFER_BIT);
    return CGFX_OK;
  }

  void end_frame() override {}
};

} // namespace

std::unique_ptr<RenderDevice> cgfx_make_opengl_render_device() {
  return std::make_unique<OpenGlRenderDevice>();
}

} // namespace cgfx

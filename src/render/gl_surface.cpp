#include "render/gl_surface.hpp"

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

namespace cgfx::gl_raster {

float clamp_unit(float value) {
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

cgfx_result clear_color_buffer_normalized(float red, float green, float blue,
                                          float alpha) {
  red = clamp_unit(red);
  green = clamp_unit(green);
  blue = clamp_unit(blue);
  alpha = clamp_unit(alpha);
  glClearColor(red, green, blue, alpha);
  glClear(GL_COLOR_BUFFER_BIT);
  return CGFX_OK;
}

void viewport_pixels(const int x, const int y, const int width_px,
                     const int height_px) {
  if (width_px <= 0 || height_px <= 0) {
    return;
  }
  glViewport(x, y, width_px, height_px);
}

} // namespace cgfx::gl_raster

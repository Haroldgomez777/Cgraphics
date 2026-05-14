#pragma once

#include <cgfx/cgfx_api.h>

namespace cgfx::gl_raster {

float clamp_unit(float value);
cgfx_result clear_color_buffer_normalized(float red, float green, float blue,
                                          float alpha);
void viewport_pixels(int x, int y, int width_px, int height_px);

} // namespace cgfx::gl_raster

#pragma once

#include <cgfx/cgfx_api.h>

#include "render/render_command_list.hpp"
#include "text/text_layout_stub.hpp"

namespace cgfx {
namespace text_raster_backend {

/** CPU raster atlas + textured quad batch for Phase 7 stub metrics (deterministic monochrome
 *  glyphs scaled to Phase-7 advances / line metrics). Intended for modular swap to FreeType/HarfBuzz later. */
cgfx_result submit_utf8_line_glyphs(
    RenderCommandList &cmds, int32_t line_origin_x, int32_t line_origin_y,
    uint32_t plan_width_px, uint32_t plan_height_px, const TextLineMetrics &metrics,
    uint32_t font_px, const char *utf8_bytes, size_t utf8_byte_len, float tint_r,
    float tint_g, float tint_b, float tint_a) noexcept;

} // namespace text_raster_backend
} // namespace cgfx

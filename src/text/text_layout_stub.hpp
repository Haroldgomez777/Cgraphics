#pragma once

#include <cgfx/cgfx_api.h>

#include <cstddef>
#include <cstdint>

namespace cgfx {

/** Rounded framebuffer pixel height for typography from theme `font_size_sp` and
 *  per-surface DPI scale (`dpi_scale` = dpi/96 on Windows backends). */
uint32_t text_logical_font_px_round(float font_size_sp, float dpi_scale) noexcept;

struct TextLineMetrics {
  uint32_t width_px{0};
  uint32_t ascent_px{0};
  uint32_t descent_px{0};
  uint32_t line_height_px{0};
};

/** Deterministic Phase-7 stub: single-line UTF-8 measure (stops at first `\\n` / `\\r\\n`).
 *  Invalid sequences advance one byte mapped to `U+FFFD` consistent with decoding rules in README.
 *
 *  `Phase 7.1`: replace advance tables with shaping / HarfBuzz + real outlines; see
 *  `SubmitGlyphRasterizationPlaceholder` (text_glyph_raster_placeholder.hpp).
 */
TextLineMetrics text_measure_utf8_line_stub(uint32_t font_px, const char *utf8_bytes,
                                            size_t utf8_byte_len) noexcept;

struct TextPlaceholderBox {
  int32_t origin_x{0};
  int32_t origin_y{0};
  uint32_t width_px{0};
  uint32_t height_px{0};
};

/** Center a measured line box inside @p bounds, clamping outer size to horizontal cap and bounds. */
void text_layout_placeholder_centered(const cgfx_layout_rect &bounds,
                                      const TextLineMetrics &metrics,
                                      uint32_t horizontal_cap_px,
                                      TextPlaceholderBox *out) noexcept;

} // namespace cgfx

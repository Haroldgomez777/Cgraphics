#pragma once

#include "text/text_layout_stub.hpp"

#include <cstddef>
#include <cstdint>

namespace cgfx {

/** Bundled NotoSans (OFL), parsed with stb_truetype — platform-neutral raster + metrics for
 *  default built-in logical font path. Fallback legacy stub activates if parsing fails.
 */
class BuiltinTruetypeFace {
public:
  static BuiltinTruetypeFace &instance() noexcept;

  bool ensure_init() noexcept;

  TextLineMetrics measure_utf8_line(uint32_t font_px, const char *utf8_bytes,
                                    size_t utf8_byte_len) noexcept;

  void utf8_line_foreach_glyph(uint32_t font_px, const char *utf8_bytes,
                               size_t utf8_byte_len, TextStubUtf8GlyphFn fn,
                               void *user_data) noexcept;

  void raster_glyph_white(char32_t cp, uint32_t font_px, uint32_t cell_w,
                          uint32_t cell_h, uint8_t *rgba, uint32_t atlas_stride_px) noexcept;

private:
  BuiltinTruetypeFace() noexcept = default;

  float scale_for_em_height_(float font_px) noexcept;
  uint32_t advance_px_(char32_t cp, float scale) noexcept;

  struct VMetricsPx {
    uint32_t ascent_px{1};
    uint32_t descent_px{1};
    uint32_t line_gap_px{0};
    uint32_t line_height_px{1};
    float baseline_from_top_px{0.f};
  };

  VMetricsPx vmetrics_px_(float scale) noexcept;
};

} // namespace cgfx

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

/** Single-line UTF‑8 measure / walker: **NotoSans** (**stb_truetype**) when the bundled face
 *  parses; **legacy stub** tables if not. Invalid UTF‑8 → **`U+FFFD`**; stops at first **`\\n`**. */
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

/** Single-line walker used by rasterization — matches `text_measure_utf8_line_stub` advances /
 * newline / invalid UTF-8 rules. */
typedef void (*TextStubUtf8GlyphFn)(char32_t codepoint, uint32_t advance_px,
                                    void *user_data);
void text_stub_utf8_line_foreach_glyph(uint32_t font_px, const char *utf8_bytes,
                                       size_t utf8_byte_len,
                                       TextStubUtf8GlyphFn fn,
                                       void *user_data) noexcept;

} // namespace cgfx

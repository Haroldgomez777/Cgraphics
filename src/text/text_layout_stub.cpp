#include "text/text_layout_stub.hpp"

#include "text/text_truetype_face.hpp"
#include "text/text_utf8.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace cgfx {

namespace {

uint32_t narrow_advance_px(uint32_t font_px) noexcept {
  if (font_px == 0U) {
    return 0U;
  }
  /** ~0.62em average advance for Latin; integer stable across platforms. */
  const uint64_t n = (static_cast<uint64_t>(font_px) * 62ULL + 99ULL) / 100ULL;
  return static_cast<uint32_t>(
      std::max<uint64_t>(1ULL, std::min<uint64_t>(n, UINT32_MAX)));
}

uint32_t wide_advance_px(uint32_t font_px) noexcept {
  if (font_px == 0U) {
    return 0U;
  }
  return std::max<uint32_t>(1U, font_px);
}

bool stub_is_narrow_latin(char32_t cp) noexcept {
  return cp >= U' ' && cp <= U'~';
}

/** East-Asian / fullwidth-ish scalars (conservative superset for deterministic wide cells). */
bool stub_is_wide_char(char32_t cp) noexcept {
  if (stub_is_narrow_latin(cp)) {
    return false;
  }
  if (cp < U'\u1100') {
    return false;
  }
  if (cp <= U'\u115F') {
    return true; // Jamo
  }
  if (cp >= U'\u2E80' && cp <= U'\u9FFF') {
    return true; // CJK core blocks
  }
  if (cp >= U'\uAC00' && cp <= U'\uD7A3') {
    return true; // Hangul syllables
  }
  if (cp >= U'\uFF00' && cp <= U'\uFFEF') {
    return true; // Fullwidth forms
  }
  if (cp >= U'\u3000' && cp <= U'\u303F') {
    return true; // CJK punctuation/symbols
  }
  return false;
}

uint32_t advance_for_codepoint(char32_t cp, uint32_t font_px) noexcept {
  if (cp <= 0xFFU) {
    if (cp == U'\t') {
      const uint32_t nav = narrow_advance_px(font_px);
      return nav * 8U;
    }
    if (cp == U'\r' || cp == U'\n' || cp == 0x0CU || cp == U'\uFEFF') {
      return 0U;
    }
    if (cp < U' ') {
      return 0U;
    }
  }
  if (stub_is_narrow_latin(cp)) {
    return narrow_advance_px(font_px);
  }
  if (stub_is_wide_char(cp)) {
    return wide_advance_px(font_px);
  }
  /** Other scripts: between narrow and square (stable stub). */
  const uint32_t n = narrow_advance_px(font_px);
  const uint32_t w = wide_advance_px(font_px);
  return std::max<uint32_t>(
      n, (n * 3U + (w * 2U) + 4U) / 5U); // ~0.6n + 0.4w blended
}

TextLineMetrics measure_utf8_line_fallback(uint32_t font_px, const char *utf8_bytes,
                                            size_t utf8_byte_len) noexcept {
  TextLineMetrics m{};
  if (utf8_bytes && utf8_byte_len == 0U) {
    utf8_byte_len = std::strlen(utf8_bytes);
  }
  if (font_px == 0U || !utf8_bytes) {
    return m;
  }
  m.ascent_px = std::max<uint32_t>(
      1U, static_cast<uint32_t>((static_cast<uint64_t>(font_px) * 4ULL + 4ULL) / 5ULL));
  m.descent_px = std::max<uint32_t>(1U, font_px - m.ascent_px);
  m.line_height_px = m.ascent_px + m.descent_px;

  const char *p = utf8_bytes;
  const char *end = utf8_bytes + utf8_byte_len;
  uint64_t acc = 0;
  while (p < end) {
    char32_t cp = 0;
    const char *before = p;
    (void)text_utf8_next_codepoint(p, end, cp);
    if (p == before) {
      break;
    }
    if (cp == U'\r') {
      continue;
    }
    if (cp == U'\n') {
      break;
    }
    const uint32_t adv = advance_for_codepoint(cp, font_px);
    acc += adv;
    if (acc > UINT32_MAX) {
      acc = UINT32_MAX;
      break;
    }
  }
  m.width_px = static_cast<uint32_t>(acc);
  return m;
}

void stub_utf8_line_foreach_glyph_fallback(uint32_t font_px, const char *utf8_bytes,
                                           size_t utf8_byte_len,
                                           TextStubUtf8GlyphFn fn,
                                           void *user_data) noexcept {
  if (!utf8_bytes || !fn || font_px == 0U) {
    return;
  }
  if (utf8_byte_len == 0U) {
    utf8_byte_len = std::strlen(utf8_bytes);
  }
  const char *p = utf8_bytes;
  const char *end = utf8_bytes + utf8_byte_len;
  while (p < end) {
    char32_t cp = 0;
    const char *before = p;
    (void)text_utf8_next_codepoint(p, end, cp);
    if (p == before) {
      break;
    }
    if (cp == U'\r') {
      continue;
    }
    if (cp == U'\n') {
      break;
    }
    fn(cp, advance_for_codepoint(cp, font_px), user_data);
  }
}

} // namespace

uint32_t text_logical_font_px_round(float font_size_sp, float dpi_scale) noexcept {
  if (!(font_size_sp > 0.f) || !std::isfinite(font_size_sp)) {
    return 0U;
  }
  float s = dpi_scale;
  if (!(s > 0.f) || !std::isfinite(s)) {
    s = 1.f;
  }
  const double scaled =
      std::trunc(static_cast<double>(font_size_sp) * static_cast<double>(s) +
                 1e-6);
  if (!(scaled >= 1.0)) {
    return 1U;
  }
  if (scaled >= static_cast<double>(UINT32_MAX)) {
    return UINT32_MAX;
  }
  return static_cast<uint32_t>(scaled);
}

TextLineMetrics text_measure_utf8_line_stub(uint32_t font_px, const char *utf8_bytes,
                                            size_t utf8_byte_len) noexcept {
  BuiltinTruetypeFace &face = BuiltinTruetypeFace::instance();
  if (face.ensure_init()) {
    return face.measure_utf8_line(font_px, utf8_bytes, utf8_byte_len);
  }
  return measure_utf8_line_fallback(font_px, utf8_bytes, utf8_byte_len);
}

void text_stub_utf8_line_foreach_glyph(uint32_t font_px, const char *utf8_bytes,
                                       size_t utf8_byte_len,
                                       TextStubUtf8GlyphFn fn,
                                       void *user_data) noexcept {
  BuiltinTruetypeFace &face = BuiltinTruetypeFace::instance();
  if (face.ensure_init()) {
    face.utf8_line_foreach_glyph(font_px, utf8_bytes, utf8_byte_len, fn, user_data);
    return;
  }
  stub_utf8_line_foreach_glyph_fallback(font_px, utf8_bytes, utf8_byte_len, fn, user_data);
}

void text_layout_placeholder_centered(const cgfx_layout_rect &bounds,
                                      const TextLineMetrics &metrics,
                                      uint32_t horizontal_cap_px,
                                      TextPlaceholderBox *out) noexcept {
  if (!out) {
    return;
  }
  *out = TextPlaceholderBox{};
  if (bounds.width == 0U || bounds.height == 0U) {
    return;
  }
  const uint32_t cap = std::min<uint32_t>(horizontal_cap_px, bounds.width);
  const uint32_t bw = std::min<uint32_t>(metrics.width_px, cap);
  const uint32_t bh = std::min<uint32_t>(metrics.line_height_px, bounds.height);
  const int64_t ox64 = static_cast<int64_t>(bounds.x) +
                       (static_cast<int64_t>(bounds.width) - static_cast<int64_t>(bw)) / 2;
  const int64_t oy64 = static_cast<int64_t>(bounds.y) +
                       (static_cast<int64_t>(bounds.height) - static_cast<int64_t>(bh)) / 2;
  out->origin_x = static_cast<int32_t>(ox64);
  out->origin_y = static_cast<int32_t>(oy64);
  out->width_px = bw;
  out->height_px = bh;
}

} // namespace cgfx

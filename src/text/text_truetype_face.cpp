#include "text/text_truetype_face.hpp"

#include "text/builtin_font_data.hpp"
#include "text/text_utf8.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../../third_party/stb_truetype.h"

namespace cgfx {
namespace {

struct TtState {
  stbtt_fontinfo fi{};
  bool ok{false};
};

static TtState &global_tt() noexcept {
  static TtState s{};
  return s;
}

} // namespace

BuiltinTruetypeFace &BuiltinTruetypeFace::instance() noexcept {
  static BuiltinTruetypeFace g{};
  return g;
}

bool BuiltinTruetypeFace::ensure_init() noexcept {
  TtState &g = global_tt();
  if (g.ok) {
    return true;
  }
  if (kBuiltinNotoSansTtfByteCount < 12U) {
    return false;
  }
  const int offset = stbtt_GetFontOffsetForIndex(kBuiltinNotoSansTtfBytes, 0);
  if (!stbtt_InitFont(&g.fi, kBuiltinNotoSansTtfBytes, offset)) {
    return false;
  }
  g.ok = true;
  return true;
}

float BuiltinTruetypeFace::scale_for_em_height_(float font_px) noexcept {
  float fp = font_px > 1.f ? font_px : 1.f;
  if (!(fp > 0.f) || !std::isfinite(fp)) {
    fp = 1.f;
  }
  TtState &g = global_tt();
  return stbtt_ScaleForPixelHeight(&g.fi, fp);
}

BuiltinTruetypeFace::VMetricsPx BuiltinTruetypeFace::vmetrics_px_(float scale) noexcept {
  VMetricsPx vm{};
  TtState &g = global_tt();
  int ascent = 0;
  int descent = 0;
  int line_gap = 0;
  stbtt_GetFontVMetrics(&g.fi, &ascent, &descent, &line_gap);
  const double asc_px = static_cast<double>(ascent) * static_cast<double>(scale);
  const double desc_px = static_cast<double>(-descent) * static_cast<double>(scale);
  const double lg_px = std::fabs(static_cast<double>(line_gap) * static_cast<double>(scale));

  vm.ascent_px =
      static_cast<uint32_t>((std::max)(1.0, std::floor(asc_px + 1.0e-6)));
  vm.descent_px =
      static_cast<uint32_t>((std::max)(1.0, std::floor(desc_px + 1.0e-6)));
  vm.line_gap_px = static_cast<uint32_t>((std::max)(0.0, std::floor(lg_px + 1.0e-6)));
  vm.line_height_px = vm.ascent_px + vm.descent_px + vm.line_gap_px;
  if (vm.line_height_px == 0U) {
    vm.line_height_px = vm.ascent_px + vm.descent_px;
    if (vm.line_height_px == 0U) {
      vm.line_height_px = 1U;
    }
  }
  vm.baseline_from_top_px =
      std::clamp(static_cast<float>((std::max)(1.0, std::floor(asc_px + 1.0e-6))),
                 1.f, static_cast<float>(vm.line_height_px));
  return vm;
}

uint32_t BuiltinTruetypeFace::advance_px_(char32_t cp, float scale) noexcept {
  if (cp <= static_cast<char32_t>(UINT8_MAX)) {
    if (cp == U'\r' || cp == U'\n' || cp == 0x0CU || cp == U'\uFEFF') {
      return 0U;
    }
    if (cp == U'\t') {
      const uint32_t sp = advance_px_(U' ', scale);
      return sp * 8U;
    }
    if (cp < U' ') {
      return 0U;
    }
  }

  TtState &g = global_tt();
  const int icp = cp <= static_cast<char32_t>(INT_MAX)
                      ? static_cast<int>(cp)
                      : static_cast<int>(char32_t{0xFFFD});

  int advance = 0;
  int lsb = 0;
  (void)lsb;
  stbtt_GetCodepointHMetrics(&g.fi, icp, &advance, &lsb);

  const double aw =
      std::fabs(static_cast<double>((std::max)(0, advance)) * static_cast<double>(scale));
  return static_cast<uint32_t>((std::max)(1LL, std::llround(aw)));
}

TextLineMetrics BuiltinTruetypeFace::measure_utf8_line(uint32_t font_px, const char *utf8_bytes,
                                                       size_t utf8_byte_len) noexcept {
  TextLineMetrics m{};
  if (!ensure_init() || font_px == 0U || utf8_bytes == nullptr) {
    return m;
  }
  const float scale = scale_for_em_height_(static_cast<float>(font_px));
  const VMetricsPx vm = vmetrics_px_(scale);
  m.ascent_px = vm.ascent_px;
  m.descent_px = vm.descent_px;
  m.line_height_px = vm.line_height_px;

  if (utf8_byte_len == 0U) {
    utf8_byte_len = std::strlen(utf8_bytes);
  }

  const char *p = utf8_bytes;
  const char *const end = utf8_bytes + utf8_byte_len;
  uint64_t acc = 0;
  while (p < end) {
    char32_t cp{};
    const char *before = p;
    if (!text_utf8_next_codepoint(p, end, cp) || p == before) {
      break;
    }
    if (cp == U'\r') {
      continue;
    }
    if (cp == U'\n') {
      break;
    }
    const uint32_t adv = advance_px_(cp, scale);
    acc += adv;
    if (acc > UINT32_MAX) {
      acc = UINT32_MAX;
      break;
    }
  }
  m.width_px = static_cast<uint32_t>(acc);
  return m;
}

void BuiltinTruetypeFace::utf8_line_foreach_glyph(uint32_t font_px, const char *utf8_bytes,
                                                  size_t utf8_byte_len, TextStubUtf8GlyphFn fn,
                                                  void *user_data) noexcept {
  if (!fn || utf8_bytes == nullptr || font_px == 0U || !ensure_init()) {
    return;
  }
  const float scale = scale_for_em_height_(static_cast<float>(font_px));
  if (utf8_byte_len == 0U) {
    utf8_byte_len = std::strlen(utf8_bytes);
  }
  const char *p = utf8_bytes;
  const char *const end = utf8_bytes + utf8_byte_len;
  while (p < end) {
    char32_t cp{};
    const char *before = p;
    if (!text_utf8_next_codepoint(p, end, cp) || p == before) {
      break;
    }
    if (cp == U'\r') {
      continue;
    }
    if (cp == U'\n') {
      break;
    }
    fn(cp, advance_px_(cp, scale), user_data);
  }
}

void BuiltinTruetypeFace::raster_glyph_white(char32_t cp, uint32_t font_px, uint32_t cell_w,
                                             uint32_t cell_h, uint8_t *rgba,
                                             uint32_t atlas_stride_px) noexcept {

  auto clear_cell = [&]() noexcept {
    for (uint32_t py = 0; py < cell_h; ++py) {
      for (uint32_t px = 0; px < cell_w; ++px) {
        uint8_t *row = rgba + py * atlas_stride_px * 4U + px * 4U;
        row[0] = row[1] = row[2] = row[3] = 0;
      }
    }
  };

  if (!rgba || cell_w == 0U || cell_h == 0U || font_px == 0U) {
    return;
  }
  clear_cell();
  if (cp == U' ' || cp == U'\t' || cp == U'\r' || cp == U'\n') {
    return;
  }
  if (!ensure_init()) {
    return;
  }

  const float scale = scale_for_em_height_(static_cast<float>(font_px));
  TtState &g = global_tt();

  const int icp = cp <= static_cast<char32_t>(INT_MAX)
                      ? static_cast<int>(cp)
                      : static_cast<int>(char32_t{0xFFFD});

  int advance_unused = 0;
  int lsb = 0;
  stbtt_GetCodepointHMetrics(&g.fi, icp, &advance_unused, &lsb);

  int ascent = 0;
  int descent_unused = 0;
  int line_gap_unused = 0;
  stbtt_GetFontVMetrics(&g.fi, &ascent, &descent_unused, &line_gap_unused);
  const int baseline =
      static_cast<int>((std::floor(static_cast<double>(ascent) * static_cast<double>(scale) +
                                   1.0e-6)));

  int bx0 = 0;
  int by0 = 0;
  int bx1 = 0;
  int by1 = 0;
  stbtt_GetCodepointBitmapBoxSubpixel(&g.fi, icp, scale, scale, 0.0f, 0.0f, &bx0, &by0,
                                      &bx1, &by1);
  const int bw = bx1 - bx0;
  const int bh = by1 - by0;
  if (bw <= 0 || bh <= 0) {
    return;
  }

  thread_local std::vector<unsigned char> scratch{};
  scratch.resize(static_cast<size_t>(bw) * static_cast<size_t>(bh));
  stbtt_MakeCodepointBitmap(&g.fi, scratch.data(), bw, bw, bw, scale, scale, icp);

  const int lsb_x = static_cast<int>(std::lround(static_cast<double>(lsb) * scale));

  for (int row = 0; row < bh; ++row) {
    const int iy = baseline + by0 + row;
    if (iy < 0) {
      continue;
    }
    if (iy >= static_cast<int>(cell_h)) {
      break;
    }
    const uint32_t py = static_cast<uint32_t>(iy);
    for (int col = 0; col < bw; ++col) {
      const uint8_t a = scratch[static_cast<size_t>(row * bw + col)];
      if (a == 0U) {
        continue;
      }
      const int ix = lsb_x + bx0 + col;
      if (ix < 0) {
        continue;
      }
      if (ix >= static_cast<int>(cell_w)) {
        break;
      }
      uint8_t *rowp = rgba + py * atlas_stride_px * 4U + static_cast<uint32_t>(ix) * 4U;
      rowp[0] = rowp[1] = rowp[2] = 255;
      rowp[3] = a;
    }
  }
}

} // namespace cgfx

#include "text/text_glyph_submit.hpp"

#include "text/text_truetype_face.hpp"

#include "render/render_command_list.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>
#include <vector>

namespace cgfx {
namespace text_raster_backend {
namespace {

constexpr int32_t kPatternRows = 13;
constexpr int32_t kPatternCols = 7;

static const char kTemplates[8][kPatternRows][kPatternCols + 1] = {
    {".######", "#......", "#......", "#.....#", "#.....#", "######.", "#.....#",
     "#.....#", "#.....#", "#.....#", "#.....#", "######.", "......."},
    {"...#...", "..###..", ".#.#.#.", "#..#..#", "#..#..#", "#######", "#..#..#",
     "#..#..#", "#..#..#", "#..#..#", "#..#..#", "#..#..#", "......."},
    {"#######", "#......", "#......", "#......", "#####..", "#......", "#......",
     "#......", "#......", "#......", "#......", "#######", "......."},
    {"#######", "#......", "#......", "#......", "#####..", "#......", "#......",
     "#......", "#......", "#......", "#......", "#......", "......."},
    {"....#..", "...##..", "..#.#..", ".#..#..", "#...#..", "#######", "....#..",
     "....#..", "....#..", "....#..", "....#..", "....#..", "......."},
    {"#######", "#......", "#......", "#......", "#######", "......#", "......#",
     "......#", "......#", "......#", "......#", "#######", "......."},
    {"....###", "...#...", "...#...", "...#...", "...#...", "...#...", "...#...",
     "...#...", "...#...", "...#...", "...#...", "...#...", "......."},
    {"..###..", ".#...#.", "#.....#", "#.....#", "#.....#", "#######", "#.....#",
     "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "......."}};

bool pattern_samples_ink(int pat_idx, int src_x, int src_y) noexcept {
  if (pat_idx < 0 || pat_idx >= 8 || src_y < 0 || src_y >= kPatternRows || src_x < 0 ||
      src_x >= kPatternCols) {
    return false;
  }
  return kTemplates[pat_idx][src_y][src_x] == '#';
}

void write_rgba(uint8_t *dst, uint32_t stride_px, uint32_t px, uint32_t py,
                const uint32_t px_rgba[4]) noexcept {
  uint8_t *row = dst + py * stride_px * 4U + px * 4U;
  row[0] = static_cast<uint8_t>(px_rgba[0]);
  row[1] = static_cast<uint8_t>(px_rgba[1]);
  row[2] = static_cast<uint8_t>(px_rgba[2]);
  row[3] = static_cast<uint8_t>(px_rgba[3]);
}

void pattern_raster_placeholder(char32_t cp, uint32_t cell_w, uint32_t cell_h, uint8_t *rgba,
                                uint32_t atlas_stride_px) noexcept {
  if (cell_w == 0U || cell_h == 0U || !rgba) {
    return;
  }
  const uint32_t transparent[4] = {0U, 0U, 0U, 0U};
  const uint32_t ink[4] = {255U, 255U, 255U, 255U};
  for (uint32_t py = 0; py < cell_h; ++py) {
    for (uint32_t px = 0; px < cell_w; ++px) {
      write_rgba(rgba, atlas_stride_px, px, py, transparent);
    }
  }
  if (cp == U' ') {
    return;
  }
  int pat_idx = 0;
  if (cp >= 32 && cp < 127) {
    pat_idx = static_cast<int>((static_cast<unsigned>(cp - U' ') % 96U) % 8U);
  } else {
    pat_idx =
        static_cast<int>((static_cast<unsigned>(cp ^ (cp >> 8U)) % 8191U) % 8U);
  }

  for (uint32_t py = 0; py < cell_h; ++py) {
    const int sy = static_cast<int>(
        (static_cast<int64_t>(py) * static_cast<int64_t>(kPatternRows)) /
        static_cast<int64_t>((std::max)(1u, cell_h)));
    for (uint32_t px = 0; px < cell_w; ++px) {
      const int sx = static_cast<int>(
          (static_cast<int64_t>(px) * static_cast<int64_t>(kPatternCols)) /
          static_cast<int64_t>((std::max)(1u, cell_w)));
      if (pattern_samples_ink(pat_idx, sx, sy)) {
        write_rgba(rgba, atlas_stride_px, px, py, ink);
      }
    }
  }
}

void raster_glyph_cell(char32_t cp, uint32_t font_px, uint32_t cell_w, uint32_t cell_h,
                       uint8_t *rgba, uint32_t atlas_stride_px) noexcept {
  BuiltinTruetypeFace &f = BuiltinTruetypeFace::instance();
  if (f.ensure_init()) {
    f.raster_glyph_white(cp, font_px, cell_w, cell_h, rgba, atlas_stride_px);
    return;
  }
  pattern_raster_placeholder(cp, cell_w, cell_h, rgba, atlas_stride_px);
}

struct CpuGlyphAtlasPass {
  std::vector<uint8_t> rgba{};
  uint32_t w{512};
  uint32_t h{256};
  uint32_t pen_x{0};
  uint32_t pen_y{0};
  uint32_t row_h{0};

  CpuGlyphAtlasPass() {
    rgba.resize(static_cast<size_t>(w) * static_cast<size_t>(h) * 4U, 0);
  }

  cgfx_result grow_height(uint32_t need_bottom_y) noexcept {
    if (need_bottom_y <= h) {
      return CGFX_OK;
    }
    uint32_t nh = h;
    while (nh < need_bottom_y && nh < 4096U) {
      nh = nh < 2048U ? nh * 2U : nh + 1024U;
    }
    if (nh > 4096U) {
      return CGFX_ERROR_OUT_OF_MEMORY;
    }
    try {
      rgba.resize(static_cast<size_t>(w) * static_cast<size_t>(nh) * 4U, 0);
      h = nh;
      return CGFX_OK;
    } catch (...) {
      return CGFX_ERROR_OUT_OF_MEMORY;
    }
  }

  cgfx_result widen(uint32_t need_w) noexcept {
    if (need_w <= w) {
      return CGFX_OK;
    }
    uint32_t nw = w;
    while (nw < need_w && nw < 4096U) {
      nw *= 2U;
    }
    if (nw > 4096U) {
      return CGFX_ERROR_OUT_OF_MEMORY;
    }
    try {
      std::vector<uint8_t> next(static_cast<size_t>(nw) * static_cast<size_t>(h) * 4U, 0);
      for (uint32_t y = 0; y < h; ++y) {
        std::memcpy(next.data() + static_cast<size_t>(y) * static_cast<size_t>(nw) * 4U,
                    rgba.data() + static_cast<size_t>(y) * static_cast<size_t>(w) * 4U,
                    static_cast<size_t>(w) * 4U);
      }
      rgba.swap(next);
      w = nw;
      return CGFX_OK;
    } catch (...) {
      return CGFX_ERROR_OUT_OF_MEMORY;
    }
  }

  cgfx_result allocate_cell_glyph(char32_t cp, uint32_t font_px, uint32_t cell_w,
                                  uint32_t cell_h, float &u0, float &v0, float &u1,
                                  float &v1) noexcept {
    if (cell_w == 0U || cell_h == 0U) {
      return CGFX_ERROR_INVALID_ARGUMENT;
    }
    if (cell_w > w) {
      const cgfx_result rw = widen(cell_w);
      if (rw != CGFX_OK) {
        return rw;
      }
    }
    if (pen_x + cell_w > w) {
      pen_x = 0U;
      pen_y += row_h;
      row_h = 0U;
    }
    cgfx_result rc = grow_height(pen_y + cell_h);
    if (rc != CGFX_OK) {
      return rc;
    }
    if (pen_x + cell_w > w) {
      const cgfx_result rw = widen(pen_x + cell_w);
      if (rw != CGFX_OK) {
        return rw;
      }
    }

    if (cell_h > row_h) {
      row_h = cell_h;
    }
    const uint32_t ox = pen_x;
    const uint32_t oy = pen_y;

    raster_glyph_cell(cp, font_px, cell_w, cell_h,
                      rgba.data() + (static_cast<size_t>(oy) * w + ox) * 4U, w);

    u0 = static_cast<float>(ox) / static_cast<float>(w);
    v0 = static_cast<float>(oy) / static_cast<float>(h);
    u1 = static_cast<float>(ox + cell_w) / static_cast<float>(w);
    v1 = static_cast<float>(oy + cell_h) / static_cast<float>(h);

    pen_x += cell_w;
    return CGFX_OK;
  }
};

struct GlyphBuildCtx {
  CpuGlyphAtlasPass *at{};
  std::vector<RenderGlyphQuadItem> quads{};
  int32_t pen_x_logical{0};
  int32_t pen_y_logical{0};
  uint32_t quad_h_px{1};
  uint32_t font_px{1};
};

void glyph_visitor(char32_t cp, uint32_t advance_px, void *user_data) noexcept {
  auto *gx = reinterpret_cast<GlyphBuildCtx *>(user_data);
  if (!gx || !gx->at || advance_px == 0U || gx->quad_h_px == 0U) {
    return;
  }
  const int32_t dst_x_start = gx->pen_x_logical;
  gx->pen_x_logical += static_cast<int32_t>(advance_px);

  float u0{};
  float v0{};
  float u1{};
  float v1{};
  const cgfx_result rc =
      gx->at->allocate_cell_glyph(cp, gx->font_px, advance_px, gx->quad_h_px, u0, v0, u1, v1);
  if (rc != CGFX_OK) {
    return;
  }

  RenderGlyphQuadItem q{};
  q.dst_x_px = dst_x_start;
  q.dst_y_px = gx->pen_y_logical;
  q.dst_w_px = advance_px;
  q.dst_h_px = gx->quad_h_px;
  q.u0 = u0;
  q.v0 = v0;
  q.u1 = u1;
  q.v1 = v1;
  q.red = 1.f;
  q.green = 1.f;
  q.blue = 1.f;
  q.alpha = 1.f;
  try {
    gx->quads.push_back(q);
  } catch (...) {
  }
}

} // namespace

cgfx_result submit_utf8_line_glyphs(RenderCommandList &cmds, int32_t line_origin_x,
                                    int32_t line_origin_y, uint32_t plan_width_px,
                                    uint32_t plan_height_px,
                                    const TextLineMetrics &metrics, uint32_t font_px,
                                    const char *utf8_bytes, size_t utf8_byte_len,
                                    float tint_r, float tint_g, float tint_b,
                                    float tint_a) noexcept {
  if (plan_width_px == 0U || plan_height_px == 0U || font_px == 0U) {
    return CGFX_OK;
  }
  if (!utf8_bytes) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  if (utf8_byte_len == 0U) {
    utf8_byte_len = std::strlen(utf8_bytes);
  }
  if (metrics.line_height_px == 0U) {
    return CGFX_OK;
  }

  const uint32_t line_h = metrics.line_height_px;
  const uint32_t dst_h_visible = std::min<uint32_t>(line_h, plan_height_px);
  if (dst_h_visible == 0U) {
    return CGFX_OK;
  }

  CpuGlyphAtlasPass atlas{};
  GlyphBuildCtx ctx{};
  ctx.at = &atlas;

  const uint32_t clipped_w =
      metrics.width_px > plan_width_px ? plan_width_px : metrics.width_px;
  const int64_t bx = static_cast<int64_t>(line_origin_x);
  const int64_t start_x_i64 =
      bx + (static_cast<int64_t>(plan_width_px) - static_cast<int64_t>(clipped_w)) /
               INT64_C(2);

  ctx.pen_x_logical = static_cast<int32_t>(
      std::clamp(start_x_i64, static_cast<int64_t>(INT32_MIN),
                 static_cast<int64_t>(INT32_MAX)));

  const int64_t by = static_cast<int64_t>(line_origin_y);
  const int64_t y_pad =
      (static_cast<int64_t>(plan_height_px) - static_cast<int64_t>(dst_h_visible)) /
      INT64_C(2);

  ctx.pen_y_logical = static_cast<int32_t>(
      std::clamp(by + y_pad, static_cast<int64_t>(INT32_MIN),
                 static_cast<int64_t>(INT32_MAX)));

  ctx.quad_h_px = dst_h_visible;
  ctx.font_px = font_px;

  text_stub_utf8_line_foreach_glyph(font_px, utf8_bytes, utf8_byte_len, glyph_visitor,
                                    &ctx);

  if (ctx.quads.empty()) {
    return CGFX_OK;
  }

  for (RenderGlyphQuadItem &q : ctx.quads) {
    q.red = tint_r;
    q.green = tint_g;
    q.blue = tint_b;
    q.alpha = tint_a;
  }

  return cmds.append_glyph_atlas_pass(atlas.w, atlas.h, atlas.rgba.data(),
                                      static_cast<size_t>(atlas.w) * atlas.h * 4U,
                                      ctx.quads.data(), ctx.quads.size());
}

} // namespace text_raster_backend
} // namespace cgfx

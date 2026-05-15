#pragma once

#include <cgfx/cgfx_api.h>

#include <cstdint>
#include <vector>

namespace cgfx {

enum class RenderCommandType {
  ClearColor = 1,
  FillRect = 2,
  FillRectBatch = 3,
  GlyphAtlasPass = 4,
};

struct RenderClearColorCommand {
  float red{0.0f};
  float green{0.0f};
  float blue{0.0f};
  float alpha{1.0f};
};

struct RenderFillRectItem {
  int32_t x_px{0};
  int32_t y_px{0};
  uint32_t width_px{0};
  uint32_t height_px{0};
  float red{0.0f};
  float green{0.0f};
  float blue{0.0f};
  float alpha{1.0f};
};

/** UVs normalized with origin top-left of atlas image (v grows downward); fragment sampling
 *  flips to OpenGL conventions inside the backend. */
struct RenderGlyphQuadItem {
  int32_t dst_x_px{0};
  int32_t dst_y_px{0};
  uint32_t dst_w_px{0};
  uint32_t dst_h_px{0};
  float u0{0.f};
  float v0{0.f};
  float u1{0.f};
  float v1{0.f};
  float red{1.0f};
  float green{1.0f};
  float blue{1.0f};
  float alpha{1.0f};
};

struct RenderCommand {
  RenderCommandType type{RenderCommandType::ClearColor};
  RenderClearColorCommand clear{};
  struct {
    int32_t x_px{0};
    int32_t y_px{0};
    uint32_t width_px{0};
    uint32_t height_px{0};
    float red{0.0f};
    float green{0.0f};
    float blue{0.0f};
    float alpha{1.0f};
  } fill_rect{};
  struct {
    size_t storage_offset{0};
    size_t item_count{0};
  } fill_rect_batch{};
  struct {
    uint32_t atlas_width_px{0};
    uint32_t atlas_height_px{0};
    size_t rgba_byte_offset{0};
    size_t rgba_byte_count{0};
    size_t quad_storage_offset{0};
    size_t quad_count{0};
  } glyph_atlas_pass{};
};

class RenderCommandList {
public:
  RenderCommandList() = default;

  void reset();
  cgfx_result append_clear_color(float red, float green, float blue, float alpha);
  cgfx_result append_fill_rect(int32_t x_px, int32_t y_px, uint32_t width_px,
                               uint32_t height_px, float red, float green,
                               float blue, float alpha);
  cgfx_result append_fill_rect_batch(const RenderFillRectItem *items,
                                     size_t item_count);
  cgfx_result append_fill_rect_batch_interleaved(const void *items,
                                                 size_t item_count,
                                                 size_t stride_bytes);

  /** Uploads @p rgba as GL_RGBA8 (`atlas_w * atlas_h * 4` bytes) and draws textured quads with
   *  modulation colors per quad. Intended for per-pass small glyph atlases. */
  cgfx_result append_glyph_atlas_pass(uint32_t atlas_w, uint32_t atlas_h,
                                      const uint8_t *rgba_pixels,
                                      size_t rgba_byte_count,
                                      const RenderGlyphQuadItem *quads,
                                      size_t quad_count);

  const std::vector<RenderCommand> &commands() const { return commands_; }
  const std::vector<RenderFillRectItem> &rect_batch_storage() const {
    return rect_batch_items_;
  }
  const std::vector<RenderGlyphQuadItem> &glyph_quad_storage() const {
    return glyph_quad_items_;
  }
  const std::vector<uint8_t> &glyph_atlas_pixel_blob() const {
    return glyph_atlas_blob_;
  }
  bool empty() const { return commands_.empty(); }

private:
  std::vector<RenderCommand> commands_{};
  std::vector<RenderFillRectItem> rect_batch_items_{};
  std::vector<uint8_t> glyph_atlas_blob_{};
  std::vector<RenderGlyphQuadItem> glyph_quad_items_{};
};

} // namespace cgfx

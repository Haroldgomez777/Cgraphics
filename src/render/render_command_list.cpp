#include "render/render_command_list.hpp"

#include <cstddef>
#include <cstring>
#include <limits>
#include <new>
#include <type_traits>

namespace cgfx {

namespace {

using CAbiRect = cgfx_surface_fill_rect_item;

static_assert(std::is_standard_layout<CAbiRect>::value);
static_assert(std::is_standard_layout<RenderFillRectItem>::value);
static_assert(sizeof(CAbiRect) == sizeof(RenderFillRectItem));
static_assert(alignof(CAbiRect) == alignof(RenderFillRectItem));
static_assert(offsetof(CAbiRect, x_px) == offsetof(RenderFillRectItem, x_px));
static_assert(offsetof(CAbiRect, y_px) == offsetof(RenderFillRectItem, y_px));
static_assert(offsetof(CAbiRect, width_px) == offsetof(RenderFillRectItem, width_px));
static_assert(offsetof(CAbiRect, height_px) == offsetof(RenderFillRectItem, height_px));
static_assert(offsetof(CAbiRect, r) == offsetof(RenderFillRectItem, red));
static_assert(offsetof(CAbiRect, g) == offsetof(RenderFillRectItem, green));
static_assert(offsetof(CAbiRect, b) == offsetof(RenderFillRectItem, blue));
static_assert(offsetof(CAbiRect, a) == offsetof(RenderFillRectItem, alpha));

bool rect_item_valid(const RenderFillRectItem &item) noexcept {
  return item.width_px != 0U && item.height_px != 0U;
}

cgfx_result validate_batch_increment(size_t existing, size_t add,
                                     size_t &out_total) noexcept {
  if (add == 0U) {
    return CGFX_OK;
  }
  if (existing > std::numeric_limits<size_t>::max() - add) {
    return CGFX_ERROR_OUT_OF_MEMORY;
  }
  out_total = existing + add;
  return CGFX_OK;
}

} // namespace

void RenderCommandList::reset() {
  commands_.clear();
  rect_batch_items_.clear();
  glyph_atlas_blob_.clear();
  glyph_quad_items_.clear();
}

cgfx_result RenderCommandList::append_clear_color(float red, float green,
                                                  float blue, float alpha) {
  try {
    RenderCommand cmd{};
    cmd.type = RenderCommandType::ClearColor;
    cmd.clear.red = red;
    cmd.clear.green = green;
    cmd.clear.blue = blue;
    cmd.clear.alpha = alpha;
    commands_.push_back(cmd);
    return CGFX_OK;
  } catch (const std::bad_alloc &) {
    return CGFX_ERROR_OUT_OF_MEMORY;
  } catch (...) {
    return CGFX_ERROR_PLATFORM;
  }
}

cgfx_result RenderCommandList::append_fill_rect(int32_t x_px, int32_t y_px,
                                                uint32_t width_px,
                                                uint32_t height_px, float red,
                                                float green, float blue,
                                                float alpha) {
  try {
    RenderCommand cmd{};
    cmd.type = RenderCommandType::FillRect;
    cmd.fill_rect.x_px = x_px;
    cmd.fill_rect.y_px = y_px;
    cmd.fill_rect.width_px = width_px;
    cmd.fill_rect.height_px = height_px;
    cmd.fill_rect.red = red;
    cmd.fill_rect.green = green;
    cmd.fill_rect.blue = blue;
    cmd.fill_rect.alpha = alpha;
    commands_.push_back(cmd);
    return CGFX_OK;
  } catch (const std::bad_alloc &) {
    return CGFX_ERROR_OUT_OF_MEMORY;
  } catch (...) {
    return CGFX_ERROR_PLATFORM;
  }
}

cgfx_result RenderCommandList::append_fill_rect_batch(
    const RenderFillRectItem *items, size_t item_count) {
  if (!items && item_count > 0U) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  if (item_count == 0U) {
    return CGFX_OK;
  }

  if (sizeof(RenderFillRectItem) > 0U &&
      item_count > std::numeric_limits<size_t>::max() / sizeof(RenderFillRectItem)) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  try {
    for (size_t i = 0; i < item_count; ++i) {
      if (!rect_item_valid(items[i])) {
        return CGFX_ERROR_INVALID_ARGUMENT;
      }
    }

    size_t new_size{};
    cgfx_result v =
        validate_batch_increment(rect_batch_items_.size(), item_count, new_size);
    if (v != CGFX_OK) {
      return v;
    }

    const size_t offset = rect_batch_items_.size();
    rect_batch_items_.resize(new_size);

    RenderFillRectItem *dst = rect_batch_items_.data() + offset;
    std::memcpy(dst, items, item_count * sizeof(RenderFillRectItem));

    RenderCommand cmd{};
    cmd.type = RenderCommandType::FillRectBatch;
    cmd.fill_rect_batch.storage_offset = offset;
    cmd.fill_rect_batch.item_count = item_count;

    commands_.emplace_back(cmd);

    return CGFX_OK;
  } catch (const std::bad_alloc &) {
    return CGFX_ERROR_OUT_OF_MEMORY;
  } catch (...) {
    return CGFX_ERROR_PLATFORM;
  }
}

cgfx_result RenderCommandList::append_fill_rect_batch_interleaved(
    const void *items, size_t item_count, size_t stride_bytes) {
  if (!items && item_count > 0U) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  if (item_count == 0U) {
    return CGFX_OK;
  }

  constexpr size_t k_layout_span = sizeof(RenderFillRectItem);
  const size_t stride = stride_bytes == 0U ? k_layout_span : stride_bytes;

  if (stride < k_layout_span) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  if (item_count > 1U &&
      stride >
          (std::numeric_limits<size_t>::max() /
           static_cast<size_t>(item_count - 1U))) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  const size_t last_row_offset =
      item_count <= 1U ? 0U : (item_count - 1U) * stride;
  if (last_row_offset > std::numeric_limits<size_t>::max() - k_layout_span) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  try {
    const unsigned char *row =
        reinterpret_cast<const unsigned char *>(items);

    for (size_t i = 0; i < item_count; ++i) {
      RenderFillRectItem tmp{};
      std::memcpy(&tmp, row, k_layout_span);
      row += stride;
      if (!rect_item_valid(tmp)) {
        return CGFX_ERROR_INVALID_ARGUMENT;
      }
    }

    size_t new_size{};
    cgfx_result v =
        validate_batch_increment(rect_batch_items_.size(), item_count, new_size);
    if (v != CGFX_OK) {
      return v;
    }

    const size_t offset = rect_batch_items_.size();
    rect_batch_items_.resize(new_size);

    RenderFillRectItem *dst = rect_batch_items_.data() + offset;

    row = reinterpret_cast<const unsigned char *>(items);
    for (size_t i = 0; i < item_count; ++i) {
      std::memcpy(dst + i, row, k_layout_span);
      row += stride;
    }

    RenderCommand cmd{};
    cmd.type = RenderCommandType::FillRectBatch;
    cmd.fill_rect_batch.storage_offset = offset;
    cmd.fill_rect_batch.item_count = item_count;
    commands_.emplace_back(cmd);

    return CGFX_OK;
  } catch (const std::bad_alloc &) {
    return CGFX_ERROR_OUT_OF_MEMORY;
  } catch (...) {
    return CGFX_ERROR_PLATFORM;
  }
}

cgfx_result RenderCommandList::append_glyph_atlas_pass(
    uint32_t atlas_w, uint32_t atlas_h, const uint8_t *rgba_pixels,
    size_t rgba_byte_count, const RenderGlyphQuadItem *quads,
    size_t quad_count) {
  if (atlas_w == 0U || atlas_h == 0U) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  const size_t expected = static_cast<size_t>(atlas_w) * static_cast<size_t>(atlas_h) * 4U;
  if (rgba_byte_count != expected || !rgba_pixels) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  if (quad_count > 0U && !quads) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  try {
    const size_t blob_off = glyph_atlas_blob_.size();
    if (rgba_byte_count > std::numeric_limits<size_t>::max() - blob_off) {
      return CGFX_ERROR_OUT_OF_MEMORY;
    }
    glyph_atlas_blob_.resize(blob_off + rgba_byte_count);
    std::memcpy(glyph_atlas_blob_.data() + blob_off, rgba_pixels, rgba_byte_count);

    const size_t qoff = glyph_quad_items_.size();
    if (quad_count > std::numeric_limits<size_t>::max() - qoff) {
      return CGFX_ERROR_OUT_OF_MEMORY;
    }
    glyph_quad_items_.resize(qoff + quad_count);
    if (quad_count > 0U) {
      std::memcpy(glyph_quad_items_.data() + qoff, quads,
                  quad_count * sizeof(RenderGlyphQuadItem));
    }

    RenderCommand cmd{};
    cmd.type = RenderCommandType::GlyphAtlasPass;
    cmd.glyph_atlas_pass.atlas_width_px = atlas_w;
    cmd.glyph_atlas_pass.atlas_height_px = atlas_h;
    cmd.glyph_atlas_pass.rgba_byte_offset = blob_off;
    cmd.glyph_atlas_pass.rgba_byte_count = rgba_byte_count;
    cmd.glyph_atlas_pass.quad_storage_offset = qoff;
    cmd.glyph_atlas_pass.quad_count = quad_count;
    commands_.push_back(cmd);
    return CGFX_OK;
  } catch (const std::bad_alloc &) {
    return CGFX_ERROR_OUT_OF_MEMORY;
  } catch (...) {
    return CGFX_ERROR_PLATFORM;
  }
}

} // namespace cgfx

#include "render/render_command_list.hpp"

#include <cstring>
#include <limits>
#include <new>

namespace cgfx {

namespace {

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

} // namespace cgfx

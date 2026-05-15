#pragma once

#include <cgfx/cgfx_api.h>

#include <cstdint>
#include <vector>

namespace cgfx {

enum class RenderCommandType {
  ClearColor = 1,
  FillRect = 2,
  FillRectBatch = 3,
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

  const std::vector<RenderCommand> &commands() const { return commands_; }
  const std::vector<RenderFillRectItem> &rect_batch_storage() const {
    return rect_batch_items_;
  }
  bool empty() const { return commands_.empty(); }

private:
  std::vector<RenderCommand> commands_{};
  std::vector<RenderFillRectItem> rect_batch_items_{};
};

} // namespace cgfx

#pragma once

#include <cgfx/cgfx_api.h>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace cgfx {

struct WidgetStyle {
  enum class Axis : uint8_t { Row = 0, Column = 1 };
  enum class SizeKind : uint8_t { Auto = 0, Fixed = 1 };

  Axis flex_axis = Axis::Column;
  SizeKind width_kind = SizeKind::Auto;
  SizeKind height_kind = SizeKind::Auto;
  uint32_t width_fixed_px = 0;
  uint32_t height_fixed_px = 0;
  float flex_grow = 0.f;
  float flex_shrink = 1.f;
};

struct WidgetNode {
  static constexpr size_t kNoParent = static_cast<size_t>(-1);

  uint64_t id = 0;
  bool alive = true;
  size_t parent = kNoParent;
  std::vector<size_t> children;

  WidgetStyle style{};
  /** Final bounds in window logical pixels updated each layout pass */
  cgfx_layout_rect bounds{};
};

class WidgetTree {
public:
  WidgetTree();

  WidgetTree(const WidgetTree &) = delete;
  WidgetTree &operator=(const WidgetTree &) = delete;

  uint64_t root_id() const noexcept {
    return nodes_.empty() ? static_cast<uint64_t>(0) : nodes_[0].id;
  }

  cgfx_result create_child(uint64_t parent_id, uint64_t *out_child_id);
  cgfx_result destroy_subtree(uint64_t widget_id);
  cgfx_result set_parent(uint64_t widget_id, uint64_t new_parent_id);

  cgfx_result set_layout_axis(uint64_t widget_id,
                              cgfx_layout_axis axis) noexcept;
  cgfx_result set_width(uint64_t widget_id, cgfx_layout_size_kind kind,
                        uint32_t fixed_px) noexcept;
  cgfx_result set_height(uint64_t widget_id, cgfx_layout_size_kind kind,
                         uint32_t fixed_px) noexcept;
  cgfx_result set_flex_grow(uint64_t widget_id, float flex_grow) noexcept;
  cgfx_result set_flex_shrink(uint64_t widget_id,
                              float flex_shrink) noexcept;

  cgfx_result get_bounds(uint64_t widget_id, cgfx_layout_rect *out) const;

  /** If @p candidate is alive, return it; otherwise return ``root_id()``. */
  uint64_t validated_widget_id_or_root(uint64_t candidate) const noexcept;

  /** Deepest widget whose bounds contain (x,y) in logical client space.
   *  Among siblings drawn on top equals **last** in children order (“last sibling on top”). */
  uint64_t hit_test_logical(int32_t x, int32_t y) const noexcept;

  const std::vector<WidgetNode> &nodes() const noexcept { return nodes_; }
  WidgetNode &node_at_mut(size_t index) noexcept { return nodes_[index]; }

  cgfx_result index_of_widget(uint64_t id, size_t &out_index) const;

private:
  bool is_alive_index_(size_t idx) const noexcept;
  cgfx_result try_widget_index(uint64_t id, size_t &out_index) const noexcept;

  bool is_direct_descendant_index_(size_t ancestor_idx,
                                    size_t node_idx) const noexcept;

  void remove_child_pointer_(size_t parent_idx,
                             size_t child_idx_value) noexcept;
  bool has_alive_children_(size_t idx) const noexcept;

  uint64_t allocate_id_();

  std::vector<WidgetNode> nodes_;
  std::unordered_map<uint64_t, size_t> id_to_index_;
  uint64_t next_id_ = 1;
};

} // namespace cgfx

#include "core/widget_tree.hpp"

#include <algorithm>
#include <vector>

namespace cgfx {

namespace {

constexpr float kEpsilon = 1.0e-5f;

bool logical_point_in_rect(int32_t x, int32_t y,
                           const cgfx_layout_rect &r) noexcept {
  if (r.width == 0U || r.height == 0U) {
    return false;
  }
  const int64_t ix = static_cast<int64_t>(x);
  const int64_t iy = static_cast<int64_t>(y);
  const int64_t bx0 = static_cast<int64_t>(r.x);
  const int64_t by0 = static_cast<int64_t>(r.y);
  const int64_t bx1 = bx0 + static_cast<int64_t>(r.width);
  const int64_t by1 = by0 + static_cast<int64_t>(r.height);
  return ix >= bx0 && iy >= by0 && ix < bx1 && iy < by1;
}

uint64_t hit_test_recursive_idx(const WidgetTree &tree, size_t node_index,
                               int32_t x, int32_t y) noexcept {
  const std::vector<WidgetNode> &nodes = tree.nodes();
  if (node_index >= nodes.size()) {
    return CGFX_WIDGET_ID_NONE;
  }
  const WidgetNode &n = nodes[node_index];
  if (!n.alive) {
    return CGFX_WIDGET_ID_NONE;
  }
  if (!logical_point_in_rect(x, y, n.bounds)) {
    return CGFX_WIDGET_ID_NONE;
  }

  /** Sibling overlap: children are probed from back to front of the storage order so the
   *  last child is “on top” among siblings. */
  for (auto it = n.children.rbegin(); it != n.children.rend(); ++it) {
    const size_t c = *it;
    const uint64_t deep = hit_test_recursive_idx(tree, c, x, y);
    if (deep != CGFX_WIDGET_ID_NONE) {
      return deep;
    }
  }
  return n.id;
}

} // namespace

WidgetTree::WidgetTree() {
  WidgetNode root{};
  root.id = allocate_id_();
  root.parent = WidgetNode::kNoParent;
  root.alive = true;
  root.style.flex_axis = WidgetStyle::Axis::Column;
  nodes_.push_back(std::move(root));
  id_to_index_[nodes_[0].id] = 0;
}

uint64_t WidgetTree::allocate_id_() { return next_id_++; }

bool WidgetTree::is_alive_index_(size_t idx) const noexcept {
  return idx < nodes_.size() && nodes_[idx].alive;
}

cgfx_result WidgetTree::try_widget_index(uint64_t id,
                                         size_t &out_index) const noexcept {
  auto it = id_to_index_.find(id);
  if (it == id_to_index_.end()) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  out_index = it->second;
  if (!is_alive_index_(out_index)) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  return CGFX_OK;
}

cgfx_result WidgetTree::index_of_widget(uint64_t id,
                                        size_t &out_index) const {
  return try_widget_index(id, out_index);
}

void WidgetTree::remove_child_pointer_(size_t parent_idx,
                                       size_t child_idx_value) noexcept {
  if (parent_idx >= nodes_.size()) {
    return;
  }
  auto &ch = nodes_[parent_idx].children;
  ch.erase(std::remove(ch.begin(), ch.end(), child_idx_value), ch.end());
}

bool WidgetTree::is_direct_descendant_index_(size_t ancestor_idx,
                                             size_t node_idx) const noexcept {
  size_t walk = node_idx;
  while (walk != WidgetNode::kNoParent && walk < nodes_.size()) {
    if (walk == ancestor_idx) {
      return true;
    }
    walk = nodes_[walk].parent;
  }
  return false;
}

cgfx_result WidgetTree::create_child(uint64_t parent_id,
                                     uint64_t *out_child_id) {
  if (!out_child_id) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  *out_child_id = 0;

  size_t parent{};
  cgfx_result r = try_widget_index(parent_id, parent);
  if (r != CGFX_OK) {
    return r;
  }

  try {
    WidgetNode node{};
    node.id = allocate_id_();
    node.parent = parent;
    node.alive = true;

    const size_t child_index = nodes_.size();
    nodes_.push_back(std::move(node));
    id_to_index_[nodes_[child_index].id] = child_index;
    nodes_[parent].children.push_back(child_index);
    *out_child_id = nodes_[child_index].id;
    return CGFX_OK;
  } catch (const std::bad_alloc &) {
    return CGFX_ERROR_OUT_OF_MEMORY;
  }
}

static void mark_subtree_dead_iter_(std::vector<WidgetNode> &nodes,
                                     size_t root_idx) noexcept {
  std::vector<size_t> stack;
  stack.push_back(root_idx);
  while (!stack.empty()) {
    const size_t cur = stack.back();
    stack.pop_back();
    if (cur >= nodes.size() || !nodes[cur].alive) {
      continue;
    }
    nodes[cur].alive = false;
    for (size_t c : nodes[cur].children) {
      if (c < nodes.size()) {
        stack.push_back(c);
      }
    }
  }
}

cgfx_result WidgetTree::destroy_subtree(uint64_t widget_id) {
  size_t idx{};
  cgfx_result r = try_widget_index(widget_id, idx);
  if (r != CGFX_OK) {
    return r;
  }
  if (idx == 0) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  WidgetNode &node = nodes_[idx];
  if (node.parent != WidgetNode::kNoParent && node.parent < nodes_.size()) {
    remove_child_pointer_(node.parent, idx);
  }
  mark_subtree_dead_iter_(nodes_, idx);

  /** Keep nodes allocated for trivial Phase 4 handle stability policy; lookups
      reject dead widgets. Indices remain stable across app lifetime window. */

  node.children.clear();

  node.bounds = cgfx_layout_rect{};
  node.style = WidgetStyle{};

  return CGFX_OK;
}

cgfx_result WidgetTree::set_parent(uint64_t widget_id,
                                   uint64_t new_parent_id) {
  size_t idx{};
  if (try_widget_index(widget_id, idx) != CGFX_OK) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  if (idx == 0) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  size_t new_parent{};
  if (try_widget_index(new_parent_id, new_parent) != CGFX_OK) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  if (new_parent == idx) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  /** Would create cycle: new_parent cannot sit under widget subtree */
  if (is_direct_descendant_index_(idx, new_parent)) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  const size_t old_parent = nodes_[idx].parent;
  if (old_parent != WidgetNode::kNoParent && old_parent < nodes_.size()) {
    remove_child_pointer_(old_parent, idx);
  }

  nodes_[idx].parent = new_parent;
  nodes_[new_parent].children.push_back(idx);

  return CGFX_OK;
}

cgfx_result WidgetTree::set_layout_axis(uint64_t widget_id,
                                        cgfx_layout_axis axis) noexcept {
  size_t idx{};
  if (try_widget_index(widget_id, idx) != CGFX_OK) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  WidgetStyle::Axis mapped = WidgetStyle::Axis::Column;
  switch (axis) {
  case CGFX_LAYOUT_AXIS_ROW:
    mapped = WidgetStyle::Axis::Row;
    break;
  case CGFX_LAYOUT_AXIS_COLUMN:
  default:
    mapped = WidgetStyle::Axis::Column;
    break;
  }
  nodes_[idx].style.flex_axis = mapped;
  return CGFX_OK;
}

cgfx_result WidgetTree::set_width(uint64_t widget_id,
                                   cgfx_layout_size_kind kind,
                                   uint32_t fixed_px) noexcept {
  size_t idx{};
  if (try_widget_index(widget_id, idx) != CGFX_OK) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  if (kind == CGFX_LAYOUT_SIZE_FIXED && fixed_px == 0U) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  WidgetStyle &st = nodes_[idx].style;
  if (kind == CGFX_LAYOUT_SIZE_FIXED) {
    st.width_kind = WidgetStyle::SizeKind::Fixed;
    st.width_fixed_px = fixed_px;
  } else {
    st.width_kind = WidgetStyle::SizeKind::Auto;
    st.width_fixed_px = 0;
  }
  return CGFX_OK;
}

cgfx_result WidgetTree::set_height(uint64_t widget_id,
                                     cgfx_layout_size_kind kind,
                                     uint32_t fixed_px) noexcept {
  size_t idx{};
  if (try_widget_index(widget_id, idx) != CGFX_OK) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  if (kind == CGFX_LAYOUT_SIZE_FIXED && fixed_px == 0U) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  WidgetStyle &st = nodes_[idx].style;
  if (kind == CGFX_LAYOUT_SIZE_FIXED) {
    st.height_kind = WidgetStyle::SizeKind::Fixed;
    st.height_fixed_px = fixed_px;
  } else {
    st.height_kind = WidgetStyle::SizeKind::Auto;
    st.height_fixed_px = 0;
  }
  return CGFX_OK;
}

cgfx_result WidgetTree::set_flex_grow(uint64_t widget_id,
                                      float flex_grow) noexcept {
  size_t idx{};
  if (try_widget_index(widget_id, idx) != CGFX_OK) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  if (flex_grow < -kEpsilon) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  nodes_[idx].style.flex_grow = flex_grow;
  return CGFX_OK;
}

cgfx_result WidgetTree::set_flex_shrink(uint64_t widget_id,
                                      float flex_shrink) noexcept {
  size_t idx{};
  if (try_widget_index(widget_id, idx) != CGFX_OK) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  if (flex_shrink <= 0.f) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  nodes_[idx].style.flex_shrink = flex_shrink;
  return CGFX_OK;
}

uint64_t WidgetTree::validated_widget_id_or_root(
    uint64_t candidate) const noexcept {
  size_t ix{};
  if (try_widget_index(candidate, ix) == CGFX_OK) {
    return candidate;
  }
  return root_id();
}

uint64_t WidgetTree::hit_test_logical(int32_t x, int32_t y) const noexcept {
  if (nodes_.empty()) {
    return CGFX_WIDGET_ID_NONE;
  }
  return hit_test_recursive_idx(*this, /*root_index=*/0, x, y);
}

cgfx_result WidgetTree::get_bounds(uint64_t widget_id,
                                    cgfx_layout_rect *out) const {
  if (!out) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  size_t idx{};
  if (try_widget_index(widget_id, idx) != CGFX_OK) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  *out = nodes_[idx].bounds;
  return CGFX_OK;
}

} // namespace cgfx

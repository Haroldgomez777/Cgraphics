#include "widgets/basic_widgets.hpp"

#include "core/widget_tree.hpp"

#include "core/window_impl.hpp"



#include <algorithm>

#include <cmath>

#include <cstring>

#include <vector>



namespace cgfx {



namespace {



void collect_alive_subtree_widget_ids(const WidgetTree &tree,

                                     size_t subtree_index,

                                     std::vector<uint64_t> &out) {

  const auto &nodes = tree.nodes();

  if (subtree_index >= nodes.size() || !nodes[subtree_index].alive) {

    return;

  }

  std::vector<size_t> stack;

  stack.push_back(subtree_index);

  while (!stack.empty()) {

    const size_t idx = stack.back();

    stack.pop_back();

    if (idx >= nodes.size() || !nodes[idx].alive) {

      continue;

    }

    out.push_back(nodes[idx].id);

    /** Preorder forward: natural paint / DOM-like order. */

    for (size_t child : nodes[idx].children) {

      if (child < nodes.size()) {

        stack.push_back(child);

      }

    }

  }

}



/**

 * Inserts children in reverse push order for stack — we used forward children

 * enumeration but stack LIFO reverses last child first. Rebuild: use explicit

 * deque or push children in reverse onto stack.

 *

 * collect order: parent first, then depth-first with first child earliest.

 */

void collect_alive_subtree_ordered_preorder(const WidgetTree &tree,

                                            size_t subtree_index,

                                            std::vector<uint64_t> &out) {

  const auto &nodes = tree.nodes();

  if (subtree_index >= nodes.size() || !nodes[subtree_index].alive) {

    return;

  }

  std::vector<size_t> stack;

  stack.push_back(subtree_index);

  while (!stack.empty()) {

    const size_t idx = stack.back();

    stack.pop_back();

    if (idx >= nodes.size() || !nodes[idx].alive) {

      continue;

    }

    out.push_back(nodes[idx].id);

    /** Push children right-to-left so left processes first. */

    const auto &ch = nodes[idx].children;

    for (auto it = ch.rbegin(); it != ch.rend(); ++it) {

      const size_t c = *it;

      if (c < nodes.size()) {

        stack.push_back(c);

      }

    }

  }

}



uint32_t placeholder_width_px(const std::string &utf8_text) noexcept {

  const size_t n = utf8_text.size();

  constexpr uint32_t kPerRun = 6U;

  constexpr uint32_t kMax = 1200U;

  const uint64_t scaled = static_cast<uint64_t>(n) * kPerRun;

  if (scaled > kMax) {

    return kMax;

  }

  return static_cast<uint32_t>(std::max(scaled, static_cast<uint64_t>(24U)));

}



} // namespace



bool BasicWidgets::input_visibility_filterThunk(cgfx_widget_id wid,

                                               void *user_data) noexcept {

  auto *w = reinterpret_cast<CgfxWindow *>(user_data);

  if (!w || wid == CGFX_WIDGET_ID_NONE) {

    return false;

  }

  return w->basic_widgets().chain_visible_for_input_hit(w->widget_tree(), wid);

}



cgfx_result BasicWidgets::create_panel(WidgetTree &tree,
                                       cgfx_widget_id parent_id,
                                       cgfx_widget_id *out_id) {
  BasicFacetUnion facet{};
  facet.kind = BasicWidgetKind::Panel;
  return allocate_typed_leaf_(tree, parent_id, facet, out_id);
}

cgfx_result BasicWidgets::create_label(WidgetTree &tree,
                                      cgfx_widget_id parent_id,
                                      cgfx_widget_id *out_id) {
  BasicFacetUnion facet{};
  facet.kind = BasicWidgetKind::Label;
  return allocate_typed_leaf_(tree, parent_id, facet, out_id);
}

cgfx_result BasicWidgets::create_button(WidgetTree &tree,
                                       cgfx_widget_id parent_id,
                                       cgfx_widget_id *out_id) {
  BasicFacetUnion facet{};
  facet.kind = BasicWidgetKind::Button;
  return allocate_typed_leaf_(tree, parent_id, facet, out_id);
}

cgfx_result BasicWidgets::allocate_typed_leaf_(WidgetTree &tree,
                                              cgfx_widget_id parent_id,
                                              BasicFacetUnion facet,
                                              cgfx_widget_id *out_id) {
  if (!out_id) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  *out_id = CGFX_WIDGET_ID_NONE;

  uint64_t child = 0U;
  const cgfx_result rc = tree.create_child(parent_id, &child);
  if (rc != CGFX_OK || child == 0U) {
    return rc != CGFX_OK ? rc : CGFX_ERROR_PLATFORM;
  }

  try {
    facets_.emplace(child, std::move(facet));
  } catch (...) {
    (void)tree.destroy_subtree(static_cast<cgfx_widget_id>(child));
    return CGFX_ERROR_OUT_OF_MEMORY;
  }
  *out_id = static_cast<cgfx_widget_id>(child);
  return CGFX_OK;
}



void BasicWidgets::purge_subtree(const WidgetTree &tree,

                                 cgfx_widget_id root_subtree_id) {

  size_t ri{};

  if (tree.index_of_widget(root_subtree_id, ri) != CGFX_OK) {

    return;

  }

  std::vector<uint64_t> ids;

  collect_alive_subtree_widget_ids(tree, ri, ids);

  const auto id_was_hovered =

      std::find(ids.begin(), ids.end(), hovered_button_logical_) != ids.end();

  const auto capture_broken =

      std::find(ids.begin(), ids.end(), capture_button_logical_) != ids.end();

  if (id_was_hovered) {

    hovered_button_logical_ = CGFX_WIDGET_ID_NONE;

  }

  if (capture_broken) {

    capture_button_logical_ = CGFX_WIDGET_ID_NONE;

    left_pressed_on_capture_ = false;

  }



  for (uint64_t id : ids) {

    facets_.erase(id);

  }

}



bool BasicWidgets::try_get_kind(cgfx_widget_id id,

                                BasicWidgetKind *out_kind) const noexcept {

  if (!out_kind) {

    return false;

  }

  const auto it = facets_.find(id);

  if (it == facets_.end()) {

    return false;

  }

  *out_kind = it->second.kind;

  return true;

}



bool BasicWidgets::chain_visible_for_input_hit(

    const WidgetTree &tree, cgfx_widget_id widget_id) const noexcept {

  if (widget_id == CGFX_WIDGET_ID_NONE || tree.nodes().empty()) {

    return false;

  }

  const size_t guard = tree.nodes().size() + 8U;

  cgfx_widget_id cur = widget_id;

  for (size_t hop = 0; hop < guard; ++hop) {

    size_t ix{};

    if (tree.index_of_widget(cur, ix) != CGFX_OK || !tree.nodes()[ix].alive) {

      return false;

    }

    const auto it = facets_.find(cur);

    if (it != facets_.end() && !it->second.visible) {

      return false;

    }

    if (cur == tree.root_id()) {

      return true;

    }

    const size_t p = tree.nodes()[ix].parent;

    if (p == WidgetNode::kNoParent || p >= tree.nodes().size()) {

      return false;

    }

    cur = tree.nodes()[p].id;

  }

  return false;

}



cgfx_widget_id BasicWidgets::resolve_button_logical_from_leaf_hit(

    const WidgetTree &tree, cgfx_widget_id filtered_hit_leaf) const noexcept {

  cgfx_widget_id cur = filtered_hit_leaf;

  const size_t guard = tree.nodes().size() + 8U;

  for (size_t hop = 0; hop < guard; ++hop) {

    if (cur == CGFX_WIDGET_ID_NONE) {

      break;

    }

    const auto it = facets_.find(cur);

    if (it != facets_.end() && it->second.kind == BasicWidgetKind::Button) {

      return cur;

    }

    size_t ix{};

    if (tree.index_of_widget(cur, ix) != CGFX_OK || !tree.nodes()[ix].alive) {

      break;

    }

    if (cur == tree.root_id()) {

      break;

    }

    const size_t p = tree.nodes()[ix].parent;

    if (p == WidgetNode::kNoParent || p >= tree.nodes().size()) {

      break;

    }

    cur = tree.nodes()[p].id;

  }

  return CGFX_WIDGET_ID_NONE;

}



void BasicWidgets::on_mouse_move_logical(const WidgetTree &tree,

                                        cgfx_widget_id filtered_hit_leaf_id,

                                        int32_t x, int32_t y) noexcept {

  last_pointer_x_ = x;

  last_pointer_y_ = y;

  const cgfx_widget_id button_id =

      resolve_button_logical_from_leaf_hit(tree, filtered_hit_leaf_id);

  if (button_id == CGFX_WIDGET_ID_NONE) {

    hovered_button_logical_ = CGFX_WIDGET_ID_NONE;

    return;

  }

  if (!chain_visible_for_input_hit(tree, button_id)) {

    hovered_button_logical_ = CGFX_WIDGET_ID_NONE;

    return;

  }

  const auto it = facets_.find(button_id);

  if (it == facets_.end() || it->second.kind != BasicWidgetKind::Button) {

    hovered_button_logical_ = CGFX_WIDGET_ID_NONE;

    return;

  }

  if (!it->second.button.enabled) {

    hovered_button_logical_ = CGFX_WIDGET_ID_NONE;

    return;

  }

  hovered_button_logical_ = button_id;

}



void BasicWidgets::on_mouse_button_logical(

    CgfxWindow *window,

    const cgfx_event_mouse_button_payload &p,

    WidgetClickSynthesisResult *out_click) noexcept {

  if (!window) {

    return;

  }

  if (out_click) {

    out_click->should_emit = false;

  }



  /** Keep hover aligned with the latest hit even if the platform omits moves. */

  on_mouse_move_logical(window->widget_tree(), p.target_widget, p.x, p.y);



  if (p.button != CGFX_MOUSE_LEFT) {

    return;

  }



  if (p.action == CGFX_PRESS) {

    const cgfx_widget_id button_hit =

        resolve_button_logical_from_leaf_hit(window->widget_tree(),

                                             p.target_widget);

    if (button_hit != CGFX_WIDGET_ID_NONE &&

        chain_visible_for_input_hit(window->widget_tree(), button_hit)) {

      const auto it = facets_.find(button_hit);

      if (it != facets_.end() && it->second.kind == BasicWidgetKind::Button &&

          it->second.button.enabled) {

        capture_button_logical_ = button_hit;

        left_pressed_on_capture_ = true;

        return;

      }

    }

    capture_button_logical_ = CGFX_WIDGET_ID_NONE;

    left_pressed_on_capture_ = false;

    return;

  }



  if (p.action == CGFX_RELEASE) {

    if (left_pressed_on_capture_ && capture_button_logical_ != CGFX_WIDGET_ID_NONE) {

      const cgfx_widget_id release_button =

          resolve_button_logical_from_leaf_hit(window->widget_tree(),

                                               p.target_widget);

      if (release_button == capture_button_logical_ && out_click &&

          chain_visible_for_input_hit(window->widget_tree(),
                                      capture_button_logical_)) {

        const auto it = facets_.find(capture_button_logical_);

        if (it != facets_.end() && it->second.kind == BasicWidgetKind::Button &&

            it->second.button.enabled) {

          out_click->should_emit = true;

          out_click->payload.widget_id = capture_button_logical_;

          out_click->payload.button = p.button;

          out_click->payload.x = p.x;

          out_click->payload.y = p.y;

        }

      }

    }

    capture_button_logical_ = CGFX_WIDGET_ID_NONE;

    left_pressed_on_capture_ = false;

  }

}



cgfx_result BasicWidgets::paint(const WidgetTree &tree,

                                RenderCommandList &cmds) {

  if (tree.nodes().empty()) {

    return CGFX_OK;

  }

  std::vector<uint64_t> order;

  collect_alive_subtree_ordered_preorder(tree, /*root_index=*/0, order);



  for (uint64_t raw_id : order) {

    const cgfx_widget_id id = static_cast<cgfx_widget_id>(raw_id);

    const auto it = facets_.find(id);

    if (it == facets_.end()) {

      continue;

    }

    /** Match hit-testing: descendants do not paint when any ancestor facet is `visible=false`. */
    if (!chain_visible_for_input_hit(tree, id)) {

      continue;

    }

    cgfx_layout_rect r{};

    if (tree.get_bounds(id, &r) != CGFX_OK || r.width == 0U || r.height == 0U) {

      continue;

    }



    switch (it->second.kind) {

    case BasicWidgetKind::Panel: {

      const PanelFacet &pn = it->second.panel;

      (void)cmds.append_fill_rect(r.x, r.y, r.width, r.height, pn.bg_r, pn.bg_g,

                                  pn.bg_b, pn.bg_a);

      break;

    }

    case BasicWidgetKind::Label: {

      const LabelFacet &lb = it->second.label;

      if (lb.text_utf8.empty()) {

        break;

      }

      const uint32_t strip_w =

          std::min(placeholder_width_px(lb.text_utf8), r.width);

      const uint32_t strip_h = std::max(2U, r.height / 10U);

      const int32_t sx =

          r.x + static_cast<int32_t>((r.width - strip_w) / 2U);

      const int32_t sy =

          r.y + static_cast<int32_t>(r.height - (r.height / 4U)) -

          static_cast<int32_t>(strip_h);

      (void)cmds.append_fill_rect(sx, sy, strip_w, strip_h, lb.marker_r,

                                  lb.marker_g, lb.marker_b, lb.marker_a);

      break;

    }

    case BasicWidgetKind::Button: {

      const ButtonFacet &bt = it->second.button;

      float cr = bt.bg_normal_r;

      float cg = bt.bg_normal_g;

      float cb = bt.bg_normal_b;

      float ca = 1.f;

      if (!bt.enabled) {

        cr = bt.bg_disabled_r;

        cg = bt.bg_disabled_g;

        cb = bt.bg_disabled_b;

      } else if (capture_button_logical_ == id && left_pressed_on_capture_) {

        cr = bt.bg_pressed_r;

        cg = bt.bg_pressed_g;

        cb = bt.bg_pressed_b;

      } else if (hovered_button_logical_ == id) {

        cr = bt.bg_hover_r;

        cg = bt.bg_hover_g;

        cb = bt.bg_hover_b;

      }

      (void)cmds.append_fill_rect(r.x, r.y, r.width, r.height, cr, cg, cb, ca);

      if (!bt.caption_utf8.empty()) {

        const uint32_t inner_w = r.width > 8U ? r.width - 8U : r.width;

        const uint32_t strip_w =

            std::min(placeholder_width_px(bt.caption_utf8), inner_w);

        const uint32_t strip_h = std::max(2U, r.height / 10U);

        const int32_t sx =

            r.x + static_cast<int32_t>((r.width - strip_w) / 2U);

        const int32_t sy =

            r.y + static_cast<int32_t>((r.height - strip_h) / 2U);

        (void)cmds.append_fill_rect(sx, sy, strip_w, strip_h,

                                    bt.caption_marker_r, bt.caption_marker_g,

                                    bt.caption_marker_b, bt.caption_marker_a);

      }

      break;

    }

    default:

      break;

    }

  }

  return CGFX_OK;

}



cgfx_result BasicWidgets::set_visible(cgfx_widget_id id, bool visible) noexcept {

  const auto it = facets_.find(id);

  if (it == facets_.end()) {

    return CGFX_ERROR_INVALID_ARGUMENT;

  }

  it->second.visible = visible;

  /** Avoid stale hover/press bookkeeping when hiding the active button directly. */
  if (!visible) {
    if (hovered_button_logical_ == id) {
      hovered_button_logical_ = CGFX_WIDGET_ID_NONE;
    }
    if (capture_button_logical_ == id) {
      capture_button_logical_ = CGFX_WIDGET_ID_NONE;
      left_pressed_on_capture_ = false;
    }
  }

  return CGFX_OK;

}



cgfx_result BasicWidgets::get_visible(cgfx_widget_id id,

                                     bool *out) const noexcept {

  if (!out) {

    return CGFX_ERROR_INVALID_ARGUMENT;

  }

  const auto it = facets_.find(id);

  if (it == facets_.end()) {

    return CGFX_ERROR_INVALID_ARGUMENT;

  }

  *out = it->second.visible;

  return CGFX_OK;

}



cgfx_result BasicWidgets::set_panel_background_rgba(cgfx_widget_id id, float r,

                                                  float g, float b,

                                                  float a) noexcept {

  const auto it = facets_.find(id);

  if (it == facets_.end() || it->second.kind != BasicWidgetKind::Panel) {

    return CGFX_ERROR_INVALID_ARGUMENT;

  }

  it->second.panel.bg_r = r;

  it->second.panel.bg_g = g;

  it->second.panel.bg_b = b;

  it->second.panel.bg_a = a;

  return CGFX_OK;

}



cgfx_result BasicWidgets::set_utf8_text(cgfx_widget_id id,

                                        BasicWidgetKind expected_kind,

                                        const char *utf8_bytes,

                                        size_t utf8_byte_count_or_zero) {

  const auto it = facets_.find(id);

  if (it == facets_.end() || it->second.kind != expected_kind) {

    return CGFX_ERROR_INVALID_ARGUMENT;

  }

  std::string s;

  if (utf8_bytes && utf8_byte_count_or_zero > 0U) {

    s.assign(utf8_bytes, utf8_bytes + utf8_byte_count_or_zero);

  } else if (utf8_bytes) {

    s.assign(utf8_bytes);

  }

  if (expected_kind == BasicWidgetKind::Label) {

    it->second.label.text_utf8 = std::move(s);

  } else if (expected_kind == BasicWidgetKind::Button) {

    it->second.button.caption_utf8 = std::move(s);

  } else {

    return CGFX_ERROR_INVALID_ARGUMENT;

  }

  return CGFX_OK;

}



cgfx_result BasicWidgets::get_utf8_text(cgfx_widget_id id,

                                        BasicWidgetKind expected_kind,

                                        char *out_buf, size_t out_cap_bytes,

                                        size_t *out_written_including_null) const

    noexcept {

  if (out_written_including_null) {

    *out_written_including_null = 0;

  }

  const auto it = facets_.find(id);

  if (it == facets_.end() || it->second.kind != expected_kind) {

    return CGFX_ERROR_INVALID_ARGUMENT;

  }

  const std::string *src = nullptr;

  if (expected_kind == BasicWidgetKind::Label) {

    src = &it->second.label.text_utf8;

  } else if (expected_kind == BasicWidgetKind::Button) {

    src = &it->second.button.caption_utf8;

  } else {

    return CGFX_ERROR_INVALID_ARGUMENT;

  }

  const size_t need = src->size() + 1U;

  if (out_written_including_null) {

    *out_written_including_null = need;

  }

  if (!out_buf || out_cap_bytes == 0U) {

    return CGFX_ERROR_INVALID_ARGUMENT;

  }

  if (out_cap_bytes < need) {

    return CGFX_ERROR_INVALID_ARGUMENT;

  }

  if (!src->empty()) {

    std::memcpy(out_buf, src->data(), src->size());

  }

  out_buf[src->size()] = '\0';

  return CGFX_OK;

}



cgfx_result BasicWidgets::utf8_text_byte_length(

    cgfx_widget_id id, BasicWidgetKind expected_kind,

    size_t *out_bytes_excluding_null) const noexcept {

  if (!out_bytes_excluding_null) {

    return CGFX_ERROR_INVALID_ARGUMENT;

  }

  *out_bytes_excluding_null = 0;

  const auto it = facets_.find(id);

  if (it == facets_.end() || it->second.kind != expected_kind) {

    return CGFX_ERROR_INVALID_ARGUMENT;

  }

  const std::string *src = nullptr;

  if (expected_kind == BasicWidgetKind::Label) {

    src = &it->second.label.text_utf8;

  } else if (expected_kind == BasicWidgetKind::Button) {

    src = &it->second.button.caption_utf8;

  } else {

    return CGFX_ERROR_INVALID_ARGUMENT;

  }

  *out_bytes_excluding_null = src->size();

  return CGFX_OK;

}



cgfx_result BasicWidgets::set_button_enabled(cgfx_widget_id id,

                                             bool enabled) noexcept {

  const auto it = facets_.find(id);

  if (it == facets_.end() || it->second.kind != BasicWidgetKind::Button) {

    return CGFX_ERROR_INVALID_ARGUMENT;

  }

  it->second.button.enabled = enabled;

  if (!enabled && hovered_button_logical_ == id) {

    hovered_button_logical_ = CGFX_WIDGET_ID_NONE;

  }

  if (!enabled && capture_button_logical_ == id) {

    capture_button_logical_ = CGFX_WIDGET_ID_NONE;

    left_pressed_on_capture_ = false;

  }

  return CGFX_OK;

}



cgfx_result BasicWidgets::get_button_enabled(cgfx_widget_id id,

                                            bool *out) const noexcept {

  if (!out) {

    return CGFX_ERROR_INVALID_ARGUMENT;

  }

  const auto it = facets_.find(id);

  if (it == facets_.end() || it->second.kind != BasicWidgetKind::Button) {

    return CGFX_ERROR_INVALID_ARGUMENT;

  }

  *out = it->second.button.enabled;

  return CGFX_OK;

}



} // namespace cgfx


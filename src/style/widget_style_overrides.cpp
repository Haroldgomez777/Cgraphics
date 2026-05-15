#include "style/widget_style_overrides.hpp"

#include <vector>

namespace cgfx {

namespace {

void collect_alive_subtree_widget_ids(const WidgetTree &tree, size_t subtree_index,
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
    for (size_t child : nodes[idx].children) {
      if (child < nodes.size()) {
        stack.push_back(child);
      }
    }
  }
}

} // namespace

const WidgetStyleOverrideRecord *
WidgetStyleOverrides::try_get(cgfx_widget_id id) const noexcept {
  const auto it = map_.find(static_cast<uint64_t>(id));
  if (it == map_.end()) {
    return nullptr;
  }
  return &it->second;
}

WidgetStyleOverrideRecord &WidgetStyleOverrides::ensure_(uint64_t id) {
  return map_[id];
}

cgfx_result WidgetStyleOverrides::set_panel_background(cgfx_widget_id id, float r,
                                                       float g, float b,
                                                       float a) noexcept {
  WidgetStyleOverrideRecord &rec = ensure_(static_cast<uint64_t>(id));
  rec.panel_background = {r, g, b, a};
  rec.mask |= static_cast<uint32_t>(WidgetStyleOverrideMask::PanelBackground);
  return CGFX_OK;
}

cgfx_result WidgetStyleOverrides::set_label_placeholder_color(
    cgfx_widget_id id, float r, float g, float b, float a) noexcept {
  WidgetStyleOverrideRecord &rec = ensure_(static_cast<uint64_t>(id));
  rec.label_placeholder = {r, g, b, a};
  rec.mask |= static_cast<uint32_t>(WidgetStyleOverrideMask::LabelPlaceholder);
  return CGFX_OK;
}

cgfx_result WidgetStyleOverrides::set_label_text_placeholder(
    cgfx_widget_id id, float r, float g, float b, float a) noexcept {
  WidgetStyleOverrideRecord &rec = ensure_(static_cast<uint64_t>(id));
  rec.label_text_color = {r, g, b, a};
  rec.mask |= static_cast<uint32_t>(WidgetStyleOverrideMask::LabelTextColor);
  return CGFX_OK;
}

cgfx_result WidgetStyleOverrides::set_label_font_size_sp(cgfx_widget_id id,
                                                         float sp) noexcept {
  WidgetStyleOverrideRecord &rec = ensure_(static_cast<uint64_t>(id));
  rec.label_font_size_sp = sp;
  rec.mask |= static_cast<uint32_t>(WidgetStyleOverrideMask::LabelFontSizeSp);
  return CGFX_OK;
}

cgfx_result WidgetStyleOverrides::set_button_background_normal(
    cgfx_widget_id id, float r, float g, float b, float a) noexcept {
  WidgetStyleOverrideRecord &rec = ensure_(static_cast<uint64_t>(id));
  rec.button_bg_normal = {r, g, b, a};
  rec.mask |= static_cast<uint32_t>(WidgetStyleOverrideMask::ButtonBackgroundNormal);
  return CGFX_OK;
}

cgfx_result WidgetStyleOverrides::set_button_background_hover(
    cgfx_widget_id id, float r, float g, float b, float a) noexcept {
  WidgetStyleOverrideRecord &rec = ensure_(static_cast<uint64_t>(id));
  rec.button_bg_hover = {r, g, b, a};
  rec.mask |= static_cast<uint32_t>(WidgetStyleOverrideMask::ButtonBackgroundHover);
  return CGFX_OK;
}

cgfx_result WidgetStyleOverrides::set_button_background_pressed(
    cgfx_widget_id id, float r, float g, float b, float a) noexcept {
  WidgetStyleOverrideRecord &rec = ensure_(static_cast<uint64_t>(id));
  rec.button_bg_pressed = {r, g, b, a};
  rec.mask |= static_cast<uint32_t>(WidgetStyleOverrideMask::ButtonBackgroundPressed);
  return CGFX_OK;
}

cgfx_result WidgetStyleOverrides::set_button_background_disabled(
    cgfx_widget_id id, float r, float g, float b, float a) noexcept {
  WidgetStyleOverrideRecord &rec = ensure_(static_cast<uint64_t>(id));
  rec.button_bg_disabled = {r, g, b, a};
  rec.mask |= static_cast<uint32_t>(WidgetStyleOverrideMask::ButtonBackgroundDisabled);
  return CGFX_OK;
}

cgfx_result WidgetStyleOverrides::set_button_caption_placeholder(
    cgfx_widget_id id, float r, float g, float b, float a) noexcept {
  WidgetStyleOverrideRecord &rec = ensure_(static_cast<uint64_t>(id));
  rec.button_caption = {r, g, b, a};
  rec.mask |= static_cast<uint32_t>(WidgetStyleOverrideMask::ButtonCaption);
  return CGFX_OK;
}

cgfx_result WidgetStyleOverrides::set_button_text_placeholder(
    cgfx_widget_id id, float r, float g, float b, float a) noexcept {
  WidgetStyleOverrideRecord &rec = ensure_(static_cast<uint64_t>(id));
  rec.button_text_color = {r, g, b, a};
  rec.mask |= static_cast<uint32_t>(WidgetStyleOverrideMask::ButtonTextColor);
  return CGFX_OK;
}

cgfx_result WidgetStyleOverrides::set_button_font_size_sp(cgfx_widget_id id,
                                                          float sp) noexcept {
  WidgetStyleOverrideRecord &rec = ensure_(static_cast<uint64_t>(id));
  rec.button_font_size_sp = sp;
  rec.mask |= static_cast<uint32_t>(WidgetStyleOverrideMask::ButtonFontSizeSp);
  return CGFX_OK;
}

void WidgetStyleOverrides::clear(cgfx_widget_id id,
                                 uint32_t mask_bits) noexcept {
  auto it = map_.find(static_cast<uint64_t>(id));
  if (it == map_.end()) {
    return;
  }
  WidgetStyleOverrideRecord &rec = it->second;
  rec.mask &= ~mask_bits;
  if (rec.mask == 0U) {
    map_.erase(it);
  }
}

void WidgetStyleOverrides::clear_all(cgfx_widget_id id) noexcept {
  map_.erase(static_cast<uint64_t>(id));
}

void WidgetStyleOverrides::purge_subtree(const WidgetTree &tree,
                                         cgfx_widget_id root_subtree_id) {
  size_t ri{};
  if (tree.index_of_widget(root_subtree_id, ri) != CGFX_OK) {
    return;
  }
  std::vector<uint64_t> ids;
  collect_alive_subtree_widget_ids(tree, ri, ids);
  for (uint64_t raw : ids) {
    map_.erase(raw);
  }
}

} // namespace cgfx

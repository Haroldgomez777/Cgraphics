#pragma once

#include "style/ui_theme.hpp"

#include <cgfx/cgfx_api.h>

#include "core/widget_tree.hpp"

#include <cstdint>
#include <unordered_map>

namespace cgfx {

/** Bitmask mirrored in public C header `cgfx_widget_style_override_mask`. */
enum class WidgetStyleOverrideMask : uint32_t {
  None = 0,
  PanelBackground = 1u << 0,
  LabelPlaceholder = 1u << 1,
  LabelTextColor = 1u << 2,
  LabelFontSizeSp = 1u << 3,
  ButtonBackgroundNormal = 1u << 4,
  ButtonBackgroundHover = 1u << 5,
  ButtonBackgroundPressed = 1u << 6,
  ButtonBackgroundDisabled = 1u << 7,
  ButtonCaption = 1u << 8,
  ButtonTextColor = 1u << 9,
  ButtonFontSizeSp = 1u << 10,
};

struct WidgetStyleOverrideRecord {
  uint32_t mask{0};
  RgbaNormalized panel_background{};
  RgbaNormalized label_placeholder{};
  RgbaNormalized label_text_color{};
  float label_font_size_sp{13.f};
  RgbaNormalized button_bg_normal{};
  RgbaNormalized button_bg_hover{};
  RgbaNormalized button_bg_pressed{};
  RgbaNormalized button_bg_disabled{};
  RgbaNormalized button_caption{};
  RgbaNormalized button_text_color{};
  float button_font_size_sp{13.f};
};

class WidgetStyleOverrides {
public:
  const WidgetStyleOverrideRecord *try_get(cgfx_widget_id id) const noexcept;

  cgfx_result set_panel_background(cgfx_widget_id id, float r, float g, float b,
                                   float a) noexcept;
  cgfx_result set_label_placeholder_color(cgfx_widget_id id, float r, float g,
                                          float b, float a) noexcept;
  cgfx_result set_label_text_placeholder(cgfx_widget_id id, float r, float g,
                                         float b, float a) noexcept;
  cgfx_result set_label_font_size_sp(cgfx_widget_id id, float sp) noexcept;
  cgfx_result set_button_background_normal(cgfx_widget_id id, float r, float g,
                                           float b, float a) noexcept;
  cgfx_result set_button_background_hover(cgfx_widget_id id, float r, float g,
                                          float b, float a) noexcept;
  cgfx_result set_button_background_pressed(cgfx_widget_id id, float r, float g,
                                            float b, float a) noexcept;
  cgfx_result set_button_background_disabled(cgfx_widget_id id, float r, float g,
                                             float b, float a) noexcept;
  cgfx_result set_button_caption_placeholder(cgfx_widget_id id, float r, float g,
                                           float b, float a) noexcept;
  cgfx_result set_button_text_placeholder(cgfx_widget_id id, float r, float g,
                                          float b, float a) noexcept;
  cgfx_result set_button_font_size_sp(cgfx_widget_id id, float sp) noexcept;

  void clear(cgfx_widget_id id, uint32_t mask_bits) noexcept;
  void clear_all(cgfx_widget_id id) noexcept;

  void purge_subtree(const WidgetTree &tree, cgfx_widget_id root_subtree_id);

private:
  using Map = std::unordered_map<uint64_t, WidgetStyleOverrideRecord>;
  Map map_{};

  WidgetStyleOverrideRecord &ensure_(uint64_t id);
};

} // namespace cgfx

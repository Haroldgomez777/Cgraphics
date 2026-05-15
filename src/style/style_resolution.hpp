#pragma once

#include "style/ui_theme.hpp"
#include "style/widget_style_overrides.hpp"
#include "widgets/basic_widgets.hpp"

namespace cgfx {
namespace style_resolution {

inline RgbaNormalized resolve_panel_background(const UiTheme &theme,
                                                 const WidgetStyleOverrideRecord *ov,
                                                 const PanelFacet &facet) noexcept {
  const uint32_t m = static_cast<uint32_t>(WidgetStyleOverrideMask::PanelBackground);
  if (ov && (ov->mask & m) != 0U) {
    return ov->panel_background;
  }
  if (facet.bg_explicit) {
    return RgbaNormalized{facet.bg_r, facet.bg_g, facet.bg_b, facet.bg_a};
  }
  return theme.panel_background;
}

inline RgbaNormalized resolve_label_placeholder(
    const UiTheme &theme, const WidgetStyleOverrideRecord *ov,
    const LabelFacet &facet) noexcept {
  const uint32_t m = static_cast<uint32_t>(WidgetStyleOverrideMask::LabelPlaceholder);
  if (ov && (ov->mask & m) != 0U) {
    return ov->label_placeholder;
  }
  if (facet.marker_explicit) {
    return RgbaNormalized{facet.marker_r, facet.marker_g, facet.marker_b, facet.marker_a};
  }
  return theme.label_placeholder;
}

inline RgbaNormalized resolve_label_text_color(const UiTheme &theme,
                                               const WidgetStyleOverrideRecord *ov) noexcept {
  const uint32_t m = static_cast<uint32_t>(WidgetStyleOverrideMask::LabelTextColor);
  if (ov && (ov->mask & m) != 0U) {
    return ov->label_text_color;
  }
  return theme.label_text_color;
}

inline float resolve_label_font_size_sp(const UiTheme &theme,
                                         const WidgetStyleOverrideRecord *ov) noexcept {
  const uint32_t m = static_cast<uint32_t>(WidgetStyleOverrideMask::LabelFontSizeSp);
  if (ov && (ov->mask & m) != 0U) {
    return ov->label_font_size_sp;
  }
  return theme.label_font_size_sp;
}

inline RgbaNormalized resolve_button_bg(const UiTheme & /*theme*/,
                                          const WidgetStyleOverrideRecord *ov,
                                          WidgetStyleOverrideMask state_bit,
                                          const RgbaNormalized &token) noexcept {
  const uint32_t m = static_cast<uint32_t>(state_bit);
  if (ov && (ov->mask & m) != 0U) {
    switch (state_bit) {
    case WidgetStyleOverrideMask::ButtonBackgroundNormal:
      return ov->button_bg_normal;
    case WidgetStyleOverrideMask::ButtonBackgroundHover:
      return ov->button_bg_hover;
    case WidgetStyleOverrideMask::ButtonBackgroundPressed:
      return ov->button_bg_pressed;
    case WidgetStyleOverrideMask::ButtonBackgroundDisabled:
      return ov->button_bg_disabled;
    default:
      break;
    }
  }
  return token;
}

inline RgbaNormalized resolve_button_caption_placeholder(
    const UiTheme &theme, const WidgetStyleOverrideRecord *ov) noexcept {
  const uint32_t m = static_cast<uint32_t>(WidgetStyleOverrideMask::ButtonCaption);
  if (ov && (ov->mask & m) != 0U) {
    return ov->button_caption;
  }
  return theme.button_caption_placeholder;
}

inline RgbaNormalized resolve_button_text_color_placeholder(
    const UiTheme &theme, const WidgetStyleOverrideRecord *ov) noexcept {
  const uint32_t m = static_cast<uint32_t>(WidgetStyleOverrideMask::ButtonTextColor);
  if (ov && (ov->mask & m) != 0U) {
    return ov->button_text_color;
  }
  return theme.button_text_color;
}

inline float resolve_button_font_size_sp(const UiTheme &theme,
                                        const WidgetStyleOverrideRecord *ov) noexcept {
  const uint32_t m = static_cast<uint32_t>(WidgetStyleOverrideMask::ButtonFontSizeSp);
  if (ov && (ov->mask & m) != 0U) {
    return ov->button_font_size_sp;
  }
  return theme.button_font_size_sp;
}

/** Matches `BasicWidgets::paint` background selection for buttons. */
inline RgbaNormalized resolve_button_face_background(
    const UiTheme &theme, const WidgetStyleOverrideRecord *ov, bool enabled,
    bool hovered, bool pressed_capture) noexcept {
  if (!enabled) {
    return resolve_button_bg(theme, ov, WidgetStyleOverrideMask::ButtonBackgroundDisabled,
                             theme.button_background_disabled);
  }
  if (pressed_capture) {
    return resolve_button_bg(theme, ov, WidgetStyleOverrideMask::ButtonBackgroundPressed,
                             theme.button_background_pressed);
  }
  if (hovered) {
    return resolve_button_bg(theme, ov, WidgetStyleOverrideMask::ButtonBackgroundHover,
                             theme.button_background_hover);
  }
  return resolve_button_bg(theme, ov, WidgetStyleOverrideMask::ButtonBackgroundNormal,
                           theme.button_background_normal);
}

} // namespace style_resolution
} // namespace cgfx

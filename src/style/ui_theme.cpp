#include "style/ui_theme.hpp"

namespace cgfx {

UiTheme UiTheme::make_phase5_builtin() noexcept {
  UiTheme t{};
  t.panel_background = {0.18f, 0.18f, 0.2f, 1.f};
  t.label_text_color = {0.92f, 0.93f, 0.96f, 1.f};
  t.label_placeholder = {0.85f, 0.85f, 0.9f, 1.f};
  t.button_background_normal = {0.35f, 0.38f, 0.45f, 1.f};
  t.button_background_hover = {0.45f, 0.5f, 0.58f, 1.f};
  t.button_background_pressed = {0.28f, 0.3f, 0.36f, 1.f};
  t.button_background_disabled = {0.25f, 0.25f, 0.27f, 1.f};
  t.button_caption_placeholder = {0.92f, 0.92f, 0.96f, 1.f};
  t.button_text_color = t.button_caption_placeholder;
  return t;
}

} // namespace cgfx

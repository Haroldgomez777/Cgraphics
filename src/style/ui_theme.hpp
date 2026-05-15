#pragma once

namespace cgfx {

/** Normalized RGBA in linear framebuffer space — matches existing rect commands. */
struct RgbaNormalized {
  float r{1.f};
  float g{1.f};
  float b{1.f};
  float a{1.f};
};

/** Per-window Phase 6 token bundle (palette + typography placeholders). */
struct UiTheme {
  RgbaNormalized panel_background{};

  RgbaNormalized label_text_color{};
  /** Phase 7: actual glyphs; retained as token for forwards compatibility. */
  float label_font_size_sp{13.f};
  RgbaNormalized label_placeholder{};

  RgbaNormalized button_background_normal{};
  RgbaNormalized button_background_hover{};
  RgbaNormalized button_background_pressed{};
  RgbaNormalized button_background_disabled{};
  RgbaNormalized button_caption_placeholder{};
  RgbaNormalized button_text_color{};
  float button_font_size_sp{13.f};

  float spacing_unit_px{8.f};
  float padding_sm_px{8.f};
  float padding_md_px{16.f};

  /** Mirrors historical Phase 5 hardcoded literals so visuals stay parity by default. */
  static UiTheme make_phase5_builtin() noexcept;
};

} // namespace cgfx

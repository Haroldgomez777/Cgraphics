/**
 * Headless API smoke: exercises font selection, text measurement, theme bundle,
 * and animation clock accessors used by the Phase 9 showcase (no GPU window).
 */
#include <cgfx/cgfx_api.h>

#include <stddef.h>
#include <string.h>

int main(void) {
  cgfx_context *ctx = NULL;
  if (cgfx_context_create(&ctx) != CGFX_OK || ctx == NULL) {
    return 1;
  }

  cgfx_font_id fid = CGFX_FONT_ID_INVALID;
  if (cgfx_font_builtin_acquire_mono_stub(ctx, &fid) != CGFX_OK) {
    cgfx_context_destroy(ctx);
    return 2;
  }
  if (cgfx_context_set_text_font(ctx, fid) != CGFX_OK) {
    cgfx_context_destroy(ctx);
    return 3;
  }

  cgfx_text_line_metrics m = {0};
  if (cgfx_text_measure_utf8_line_cstr_pixels(
          ctx, CGFX_FONT_ID_INVALID, "Phase 9 showcase", 15.f, 1.75f,
          &m) != CGFX_OK) {
    cgfx_context_destroy(ctx);
    return 4;
  }
  if (m.line_height_px == 0U) {
    cgfx_context_destroy(ctx);
    return 5;
  }

  cgfx_theme *preset = NULL;
  if (cgfx_theme_create(&preset) != CGFX_OK || preset == NULL) {
    cgfx_context_destroy(ctx);
    return 6;
  }
  (void)cgfx_theme_reset_to_defaults(preset);
  cgfx_color_rgba tweak = {0.08f, 0.10f, 0.14f, 1.f};
  (void)cgfx_theme_set_color_rgba(preset, CGFX_THEME_COLOR_PANEL_BACKGROUND,
                                  &tweak);
  cgfx_theme_destroy(preset);

  cgfx_context_set_animation_speed_scale(ctx, 1.f);
  (void)cgfx_context_animation_clock_get_seconds(ctx);

  cgfx_context_destroy(ctx);
  return 0;
}

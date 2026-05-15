#include "theme_setup.h"

static cgfx_result set_rgba(cgfx_window *window, cgfx_theme_color_token token,
                           float r, float g, float b, float a) {
  cgfx_color_rgba c;
  c.r = r;
  c.g = g;
  c.b = b;
  c.a = a;
  return cgfx_window_theme_set_color_rgba(window, token, &c);
}

void prof_app_font_setup(cgfx_context *ctx) {
  if (!ctx) {
    return;
  }
  cgfx_font_id fid = CGFX_FONT_ID_INVALID;
  if (cgfx_font_builtin_acquire_mono_stub(ctx, &fid) != CGFX_OK) {
    return;
  }
  (void)cgfx_context_set_text_font(ctx, fid);
}

cgfx_result prof_app_theme_apply(cgfx_window *window,
                                 const prof_app_handles *handles) {
  if (!window || !handles) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  cgfx_result rc = CGFX_OK;

  if ((rc = cgfx_window_theme_reset_to_defaults(window)) != CGFX_OK) {
    return rc;
  }

  (void)cgfx_window_theme_set_metric(window, CGFX_THEME_METRIC_SPACING_UNIT_PX, 8.f);
  (void)cgfx_window_theme_set_metric(window, CGFX_THEME_METRIC_PADDING_SM_PX, 10.f);
  (void)cgfx_window_theme_set_metric(window, CGFX_THEME_METRIC_PADDING_MD_PX, 16.f);
  (void)cgfx_window_theme_set_metric(window, CGFX_THEME_METRIC_LABEL_FONT_SIZE_SP, 15.f);
  (void)cgfx_window_theme_set_metric(window, CGFX_THEME_METRIC_BUTTON_FONT_SIZE_SP, 14.f);

  (void)set_rgba(window, CGFX_THEME_COLOR_PANEL_BACKGROUND, 0.07f, 0.085f, 0.11f, 1.f);
  (void)set_rgba(window, CGFX_THEME_COLOR_LABEL_TEXT, 0.92f, 0.93f, 0.95f, 1.f);
  (void)set_rgba(window, CGFX_THEME_COLOR_LABEL_PLACEHOLDER, 0.55f, 0.62f, 0.74f, 1.f);

  (void)set_rgba(window, CGFX_THEME_COLOR_BUTTON_BACKGROUND_NORMAL, 0.16f, 0.41f,
                 0.84f, 1.f);
  (void)set_rgba(window, CGFX_THEME_COLOR_BUTTON_BACKGROUND_HOVER, 0.24f, 0.52f,
                 0.98f, 1.f);
  (void)set_rgba(window, CGFX_THEME_COLOR_BUTTON_BACKGROUND_PRESSED, 0.10f, 0.28f,
                 0.68f, 1.f);
  (void)set_rgba(window, CGFX_THEME_COLOR_BUTTON_BACKGROUND_DISABLED, 0.25f, 0.27f,
                 0.30f, 1.f);

  (void)set_rgba(window, CGFX_THEME_COLOR_BUTTON_CAPTION_PLACEHOLDER, 0.80f, 0.86f,
                 1.0f, 1.f);
  (void)set_rgba(window, CGFX_THEME_COLOR_BUTTON_TEXT, 0.98f, 0.99f, 1.0f, 1.f);

  if (handles->sidebar_panel != CGFX_WIDGET_ID_NONE) {
    (void)cgfx_widget_style_set_panel_background_rgba_normalized(
        window, handles->sidebar_panel, 0.05f, 0.065f, 0.085f, 1.f);
  }
  if (handles->hero_panel != CGFX_WIDGET_ID_NONE) {
    (void)cgfx_widget_style_set_panel_background_rgba_normalized(
        window, handles->hero_panel, 0.10f, 0.13f, 0.17f, 1.f);
  }
  if (handles->primary_button != CGFX_WIDGET_ID_NONE) {
    (void)cgfx_widget_style_set_button_background_normal_rgba_normalized(
        window, handles->primary_button, 0.20f, 0.55f, 0.95f, 1.f);
    (void)cgfx_widget_style_set_button_background_hover_rgba_normalized(
        window, handles->primary_button, 0.28f, 0.62f, 1.0f, 1.f);
    (void)cgfx_widget_style_set_button_background_pressed_rgba_normalized(
        window, handles->primary_button, 0.12f, 0.34f, 0.78f, 1.f);
  }
  if (handles->secondary_button != CGFX_WIDGET_ID_NONE) {
    (void)cgfx_widget_style_set_button_background_normal_rgba_normalized(
        window, handles->secondary_button, 0.14f, 0.16f, 0.19f, 1.f);
    (void)cgfx_widget_style_set_button_background_hover_rgba_normalized(
        window, handles->secondary_button, 0.20f, 0.24f, 0.30f, 1.f);
    (void)cgfx_widget_style_set_button_background_pressed_rgba_normalized(
        window, handles->secondary_button, 0.10f, 0.12f, 0.15f, 1.f);
  }

  return CGFX_OK;
}

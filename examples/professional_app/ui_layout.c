#include "ui_layout.h"

#include <stddef.h>
#include <string.h>

static cgfx_result set_text(cgfx_window *window, cgfx_widget_id id,
                            const char *utf8) {
  return cgfx_basic_widget_utf8_text_set(window, id, utf8,
                                         utf8 ? strlen(utf8) : 0U);
}

cgfx_result prof_app_ui_build(cgfx_window *window, prof_app_handles *out_handles) {
  if (!window || !out_handles) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }

  memset(out_handles, 0, sizeof(*out_handles));

  cgfx_widget_id root = cgfx_window_widget_root(window);
  cgfx_result rc;

  if ((rc = cgfx_basic_widget_panel_create(window, root, &out_handles->chrome_row)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_layout_axis(window, out_handles->chrome_row,
                                         CGFX_LAYOUT_AXIS_ROW)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->chrome_row,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->chrome_row,
                                   CGFX_LAYOUT_SIZE_FIXED, 56U)) != CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_label_create(window, out_handles->chrome_row,
                                            &out_handles->title_label)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->title_label,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->title_label,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_flex_grow(window, out_handles->title_label, 1.f)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_style_set_label_font_size_sp_placeholder(
           window, out_handles->title_label, 17.f)) != CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_label_create(window, out_handles->chrome_row,
                                            &out_handles->badge_label)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->badge_label,
                                   CGFX_LAYOUT_SIZE_FIXED, 148U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->badge_label,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_panel_create(window, root, &out_handles->body_row)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_layout_axis(window, out_handles->body_row,
                                        CGFX_LAYOUT_AXIS_ROW)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->body_row,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->body_row,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_flex_grow(window, out_handles->body_row, 1.f)) !=
      CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_panel_create(window, out_handles->body_row,
                                            &out_handles->sidebar_panel)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_layout_axis(window, out_handles->sidebar_panel,
                                        CGFX_LAYOUT_AXIS_COLUMN)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->sidebar_panel,
                                   CGFX_LAYOUT_SIZE_FIXED, 228U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->sidebar_panel,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_label_create(window, out_handles->sidebar_panel,
                                            &out_handles->nav_title)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->nav_title,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->nav_title,
                                   CGFX_LAYOUT_SIZE_FIXED, 32U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_style_set_label_text_placeholder_rgba_normalized(
           window, out_handles->nav_title, 0.72f, 0.78f, 0.92f, 1.f)) !=
      CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_label_create(
           window, out_handles->sidebar_panel, &out_handles->nav_item_a)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->nav_item_a,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->nav_item_a,
                                   CGFX_LAYOUT_SIZE_FIXED, 28U)) != CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_label_create(
           window, out_handles->sidebar_panel, &out_handles->nav_item_b)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->nav_item_b,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->nav_item_b,
                                   CGFX_LAYOUT_SIZE_FIXED, 28U)) != CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_label_create(
           window, out_handles->sidebar_panel, &out_handles->nav_item_c)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->nav_item_c,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->nav_item_c,
                                   CGFX_LAYOUT_SIZE_FIXED, 34U)) != CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_panel_create(window, out_handles->body_row,
                                            &out_handles->content_column)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_layout_axis(window, out_handles->content_column,
                                        CGFX_LAYOUT_AXIS_COLUMN)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->content_column,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->content_column,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_flex_grow(window, out_handles->content_column, 1.f)) !=
      CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_label_create(window, out_handles->content_column,
                                            &out_handles->status_label)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->status_label,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->status_label,
                                   CGFX_LAYOUT_SIZE_FIXED, 36U)) != CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_panel_create(window, out_handles->content_column,
                                            &out_handles->hero_panel)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_layout_axis(window, out_handles->hero_panel,
                                        CGFX_LAYOUT_AXIS_COLUMN)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->hero_panel,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->hero_panel,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_flex_grow(window, out_handles->hero_panel, 1.f)) !=
      CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_label_create(window, out_handles->hero_panel,
                                            &out_handles->hero_caption)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->hero_caption,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->hero_caption,
                                   CGFX_LAYOUT_SIZE_FIXED, 26U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_style_set_label_text_placeholder_rgba_normalized(
           window, out_handles->hero_caption, 0.70f, 0.76f, 0.88f, 1.f)) !=
      CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_label_create(window, out_handles->hero_panel,
                                            &out_handles->hero_value)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->hero_value,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->hero_value,
                                   CGFX_LAYOUT_SIZE_FIXED, 44U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_style_set_label_font_size_sp_placeholder(
           window, out_handles->hero_value, 22.f)) != CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_label_create(window, out_handles->hero_panel,
                                            &out_handles->hero_blurb)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->hero_blurb,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->hero_blurb,
                                   CGFX_LAYOUT_SIZE_FIXED, 72U)) != CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_panel_create(window, out_handles->content_column,
                                            &out_handles->action_row)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_layout_axis(window, out_handles->action_row,
                                        CGFX_LAYOUT_AXIS_ROW)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->action_row,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->action_row,
                                   CGFX_LAYOUT_SIZE_FIXED, 52U)) != CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_button_create(window, out_handles->action_row,
                                            &out_handles->primary_button)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->primary_button,
                                   CGFX_LAYOUT_SIZE_FIXED, 200U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->primary_button,
                                   CGFX_LAYOUT_SIZE_FIXED, 42U)) != CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_panel_create(window, out_handles->action_row,
                                            &out_handles->gap_after_primary)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->gap_after_primary,
                                   CGFX_LAYOUT_SIZE_FIXED, 16U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->gap_after_primary,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_button_create(window, out_handles->action_row,
                                             &out_handles->secondary_button)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->secondary_button,
                                   CGFX_LAYOUT_SIZE_FIXED, 160U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->secondary_button,
                                   CGFX_LAYOUT_SIZE_FIXED, 42U)) != CGFX_OK) {
    return rc;
  }

  if ((rc = cgfx_basic_widget_label_create(window, out_handles->content_column,
                                            &out_handles->footer_label)) !=
      CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_width(window, out_handles->footer_label,
                                   CGFX_LAYOUT_SIZE_AUTO, 0U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_set_height(window, out_handles->footer_label,
                                   CGFX_LAYOUT_SIZE_FIXED, 40U)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_style_set_label_font_size_sp_placeholder(
           window, out_handles->footer_label, 12.f)) != CGFX_OK) {
    return rc;
  }
  if ((rc = cgfx_widget_style_set_label_text_placeholder_rgba_normalized(
           window, out_handles->footer_label, 0.55f, 0.58f, 0.64f, 1.f)) !=
      CGFX_OK) {
    return rc;
  }

  (void)set_text(window, out_handles->title_label, "Workspace / Operations");
  (void)set_text(window, out_handles->badge_label, "cgfx Phase 9");
  (void)set_text(window, out_handles->nav_title, "CAPABILITIES");
  (void)set_text(window, out_handles->nav_item_a, "• Flex layout & tree");
  (void)set_text(window, out_handles->nav_item_b, "• Styles + theme tokens");
  (void)set_text(window, out_handles->nav_item_c, "• Inputs, text measure, motion");
  (void)set_text(window, out_handles->hero_caption, "Engagement counter");
  (void)set_text(window, out_handles->hero_value, "0");
  (void)set_text(
      window, out_handles->hero_blurb,
      "Primary button records a retained-mode interaction: UTF-8 captions resize via "
      "cgfx_text_measure_* (stub font), hits synthesize CGFX_EVENT_WIDGET_CLICK, "
      "and Phase 8 translate clips add motion feedback on the card.");
  (void)set_text(window, out_handles->primary_button, "Record interaction");
  (void)set_text(window, out_handles->secondary_button, "Reset demo");
  (void)set_text(window, out_handles->footer_label,
                 "Pointer propagation: BUBBLE_TO_PARENT — "
                 "cgfx_context_set_input_routing_trace_enabled(ctx, true) logs stops.");

  return CGFX_OK;
}

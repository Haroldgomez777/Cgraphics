#ifndef CGFX_EXAMPLES_PROFESSIONAL_APP_APP_TYPES_H
#define CGFX_EXAMPLES_PROFESSIONAL_APP_APP_TYPES_H

#include <cgfx/cgfx_api.h>

/** Stable widget handles for the Phase 9 showcase layout. */
typedef struct prof_app_handles {
  cgfx_widget_id chrome_row;
  cgfx_widget_id title_label;
  cgfx_widget_id badge_label;
  cgfx_widget_id body_row;
  cgfx_widget_id sidebar_panel;
  cgfx_widget_id nav_title;
  cgfx_widget_id nav_item_a;
  cgfx_widget_id nav_item_b;
  cgfx_widget_id nav_item_c;
  cgfx_widget_id content_column;
  cgfx_widget_id status_label;
  cgfx_widget_id hero_panel;
  cgfx_widget_id hero_caption;
  cgfx_widget_id hero_value;
  cgfx_widget_id hero_blurb;
  cgfx_widget_id action_row;
  cgfx_widget_id primary_button;
  cgfx_widget_id gap_after_primary;
  cgfx_widget_id secondary_button;
  cgfx_widget_id footer_label;
} prof_app_handles;

#endif

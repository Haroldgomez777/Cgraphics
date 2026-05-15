#include <cgfx/cgfx_api.h>

#include <stdio.h>
#include <string.h>

#include "app_types.h"
#include "theme_setup.h"
#include "ui_layout.h"

static void set_status(cgfx_window *window, cgfx_widget_id label,
                       const char *text) {
  (void)cgfx_basic_widget_utf8_text_set(window, label, text,
                                        text ? strlen(text) : 0U);
}

static void set_hero_value(cgfx_window *window, cgfx_widget_id label,
                           unsigned long long n) {
  char buf[48];
  (void)snprintf(buf, sizeof buf, "%llu", n);
  set_status(window, label, buf);
}

static void update_status_line(cgfx_window *window, cgfx_context *ctx,
                               const prof_app_handles *h, float dpi,
                               unsigned long long count) {
  char line[192];
  cgfx_text_line_metrics m = {0};
  const char *sample = "Phase 9 / UTF-8 measure";
  if (count == 0ULL) {
    if (cgfx_text_measure_utf8_line_cstr_pixels(ctx, CGFX_FONT_ID_INVALID, sample,
                                                15.f, dpi, &m) == CGFX_OK) {
      (void)snprintf(
          line, sizeof line,
          "Ready — sample line \"%s\" measures %u x %u px (stub font, dpi=%.2f).",
          sample, m.width_px, m.line_height_px, (double)dpi);
    } else {
      (void)snprintf(line, sizeof line,
                     "Ready — text measure API error while probing \"%s\".", sample);
    }
    set_status(window, h->status_label, line);
    return;
  }
  if (cgfx_text_measure_utf8_line_cstr_pixels(ctx, CGFX_FONT_ID_INVALID, sample,
                                              15.f, dpi, &m) == CGFX_OK) {
    (void)snprintf(line, sizeof line,
                   "Recorded %llu — line measure for \"%s\": %u x %u px "
                   "(stub font, dpi=%.2f).",
                   count, sample, m.width_px, m.line_height_px, (double)dpi);
  } else {
    (void)snprintf(line, sizeof line,
                   "Recorded %llu — text measure API returned an error.", count);
  }
  set_status(window, h->status_label, line);
}

static void play_hero_nudge(cgfx_window *window, cgfx_widget_id hero,
                            unsigned int *flip) {
  if (hero == CGFX_WIDGET_ID_NONE) {
    return;
  }
  (void)cgfx_animation_stop_widget_property(
      window, hero, CGFX_ANIM_PROPERTY_TRANSLATE_LOGICAL_PX);
  const float dir = (*flip & 1U) ? -1.f : 1.f;
  *flip = (*flip + 1U) & 7U;
  cgfx_animation_id aid = CGFX_ANIMATION_ID_NONE;
  (void)cgfx_animation_start_translate_logical_px(
      window, hero, 0.f, 0.f, dir * 18.f, 0.f, 0.26f, CGFX_ANIM_EASE_OUT_QUAD,
      &aid);
}

static void reset_demo(cgfx_window *window, cgfx_context *ctx,
                       const prof_app_handles *h, unsigned long long *counter,
                       unsigned int *nudge_flip, float dpi) {
  *counter = 0ULL;
  *nudge_flip = 0U;
  if (h->hero_panel != CGFX_WIDGET_ID_NONE) {
    (void)cgfx_animation_stop_widget_property(
        window, h->hero_panel, CGFX_ANIM_PROPERTY_TRANSLATE_LOGICAL_PX);
  }
  set_hero_value(window, h->hero_value, 0ULL);
  update_status_line(window, ctx, h, dpi, 0ULL);
}

int main(void) {
  cgfx_context *ctx = NULL;
  cgfx_window *win = NULL;
  prof_app_handles ui = {0};
  unsigned long long counter = 0ULL;
  unsigned int nudge_flip = 0U;
  int running = 1;

  if (cgfx_context_create(&ctx) != CGFX_OK || ctx == NULL) {
    fprintf(stderr, "cgfx_context_create failed\n");
    return 1;
  }

  cgfx_window_desc desc;
  desc.title = "cgfx — Phase 9 professional showcase";
  desc.width = 1100U;
  desc.height = 720U;

  if (cgfx_window_create(ctx, &desc, &win) != CGFX_OK || win == NULL) {
    fprintf(stderr, "cgfx_window_create failed\n");
    cgfx_context_destroy(ctx);
    return 1;
  }

  /** Demonstrate bubble expansion on pointer events (widget clicks stay single-shot). */
  cgfx_window_set_input_propagation_policy(
      win, CGFX_INPUT_PROPAGATION_BUBBLE_TO_PARENT);

  prof_app_font_setup(ctx);

  if (prof_app_ui_build(win, &ui) != CGFX_OK) {
    fprintf(stderr, "prof_app_ui_build failed\n");
    cgfx_window_destroy(win);
    cgfx_context_destroy(ctx);
    return 1;
  }

  if (prof_app_theme_apply(win, &ui) != CGFX_OK) {
    fprintf(stderr, "prof_app_theme_apply failed\n");
    cgfx_window_destroy(win);
    cgfx_context_destroy(ctx);
    return 1;
  }

  float dpi_for_status = 1.f;
  (void)cgfx_window_get_dpi_scale(win, &dpi_for_status);
  update_status_line(win, ctx, &ui, dpi_for_status, counter);

  while (running) {
    if (cgfx_poll_events(ctx) != CGFX_OK) {
      fprintf(stderr, "cgfx_poll_events failed\n");
      break;
    }

    cgfx_event ev;
    while (cgfx_next_event_into(ctx, &ev)) {
      if (ev.window != win) {
        continue;
      }
      if (ev.type == CGFX_EVENT_CLOSE_REQUEST) {
        running = 0;
        break;
      }
      if (ev.type == CGFX_EVENT_KEY && ev.payload.key.action == CGFX_PRESS &&
          ev.payload.key.key == CGFX_KEY_ESCAPE) {
        running = 0;
        break;
      }
      if (ev.type == CGFX_EVENT_WIDGET_CLICK &&
          ev.payload.widget_click.button == CGFX_MOUSE_LEFT) {
        if (ev.payload.widget_click.widget_id == ui.primary_button) {
          counter++;
          set_hero_value(win, ui.hero_value, counter);
          float dpi = 1.f;
          (void)cgfx_window_get_dpi_scale(win, &dpi);
          update_status_line(win, ctx, &ui, dpi, counter);
          play_hero_nudge(win, ui.hero_panel, &nudge_flip);
        } else if (ev.payload.widget_click.widget_id == ui.secondary_button) {
          float dpi_now = 1.f;
          (void)cgfx_window_get_dpi_scale(win, &dpi_now);
          reset_demo(win, ctx, &ui, &counter, &nudge_flip, dpi_now);
        }
      }
    }

    if (!running) {
      break;
    }

    uint32_t ww = 0U;
    uint32_t hh = 0U;
    float dpi_scale = 1.f;

    if (cgfx_window_begin_present_pass(win, &ww, &hh, &dpi_scale) != CGFX_OK) {
      fprintf(stderr, "cgfx_window_begin_present_pass failed\n");
      break;
    }

    if (cgfx_surface_clear_normalized_rgba(win, 0.04f, 0.045f, 0.055f, 1.f) !=
        CGFX_OK) {
      fprintf(stderr, "clear failed\n");
    }

    if (cgfx_window_draw_basic_widgets(win) != CGFX_OK) {
      fprintf(stderr, "cgfx_window_draw_basic_widgets failed\n");
    }

    cgfx_window_end_present_pass(win);
  }

  cgfx_window_destroy(win);
  cgfx_context_destroy(ctx);
  return 0;
}

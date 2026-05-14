#include <cgfx/cgfx_api.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
  cgfx_context *ctx = NULL;
  cgfx_window *win = NULL;

  if (cgfx_context_create(&ctx) != CGFX_OK || ctx == NULL) {
    fprintf(stderr, "cgfx_context_create failed\n");
    return 1;
  }

  cgfx_window_desc desc;
  desc.title = "cgfx phase1 demo";
  desc.width = 960U;
  desc.height = 540U;

  if (cgfx_window_create(ctx, &desc, &win) != CGFX_OK || win == NULL) {
    fprintf(stderr, "cgfx_window_create failed\n");
    cgfx_context_destroy(ctx);
    return 1;
  }

  int running = 1;
  while (running) {
    if (cgfx_poll_events(ctx) != CGFX_OK) {
      fprintf(stderr, "cgfx_poll_events failed\n");
      break;
    }

    cgfx_event_type type;
    cgfx_window *event_wnd = NULL;

    unsigned char payload[128];
    while (cgfx_next_event(ctx, &type, &event_wnd, payload, sizeof(payload),
                          NULL)) {

      if ((type == CGFX_EVENT_CLOSE_REQUEST) && (event_wnd == win)) {
        running = 0;
      }
    }

    uint32_t ww = 0U;
    uint32_t hh = 0U;
    float dpi_scale = 1.0f;

    if (cgfx_window_begin_present_pass(win, &ww, &hh, &dpi_scale) != CGFX_OK) {
      fprintf(stderr, "begin_present failed\n");
      break;
    }

    float g = 0.45f + (0.03f * dpi_scale);
    if (g > 0.95f) {
      g = 0.95f;
    }

    if (cgfx_surface_clear_normalized_rgba(win, 0.12f, g, 0.85f, 1.0f) !=
        CGFX_OK) {

      fprintf(stderr, "clear failed\n");
    }

    cgfx_window_end_present_pass(win);
  }

  cgfx_window_destroy(win);
  cgfx_context_destroy(ctx);
  return 0;
}

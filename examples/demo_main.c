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
  desc.title = "cgfx phase2 demo";
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

#define GRID_MARGIN 48
#define CELL 28

    const uint32_t grid_w =
        (ww > (GRID_MARGIN * 2U)) ? (ww - GRID_MARGIN * 2U) : 0U;
    const uint32_t grid_h =
        (hh > (GRID_MARGIN * 2U)) ? (hh - GRID_MARGIN * 2U) : 0U;

    unsigned cols =
        (grid_w >= CELL && CELL > 0) ? ((unsigned)(grid_w / CELL)) : 1U;
    unsigned rows =
        (grid_h >= CELL && CELL > 0) ? ((unsigned)(grid_h / CELL)) : 1U;

    if (cols > 32U) {
      cols = 32U;
    }
    if (rows > 24U) {
      rows = 24U;
    }

    cgfx_surface_fill_rect_item cells[32U * 24U];
    size_t idx = 0U;

    for (unsigned yi = 0U; yi < rows; yi++) {
      for (unsigned xi = 0U; xi < cols; xi++) {
        cells[idx].x_px = (int32_t)((int32_t)GRID_MARGIN +
                                    (int32_t)(xi * CELL));
        cells[idx].y_px = (int32_t)((int32_t)GRID_MARGIN +
                                    (int32_t)(yi * CELL));
        cells[idx].width_px = CELL;
        cells[idx].height_px = CELL;

        const int checker = (((int)(xi ^ yi)) & 1) != 0;
        cells[idx].r = checker ? 0.94f : 0.22f;
        cells[idx].g = checker ? 0.96f : 0.74f;
        cells[idx].b = checker ? 0.96f : 0.94f;
        cells[idx].a = 1.0f;

        idx++;
      }
    }

    if (idx > 0U) {
      if (cgfx_surface_fill_rect_batch_pixels(win, cells, idx, 0U) != CGFX_OK) {
        fprintf(stderr, "fill_rect_batch failed\n");
      }
    }

#undef CELL
#undef GRID_MARGIN

    const uint32_t rw = (ww > 220U) ? (ww / 4U) : 80U;
    const uint32_t rh = (hh > 180U) ? (hh / 5U) : 60U;
    const int32_t rx = (int32_t)((ww > rw) ? ((ww - rw) / 2U) : 0U);
    const int32_t ry = (int32_t)((hh > rh) ? ((hh - rh) / 2U) : 0U);

    /* Single-fill path sanity check alongside Phase 2.3 batched rects. */
    if (cgfx_surface_fill_rect_pixels(win, rx, ry, rw, rh, 0.95f, 0.90f, 0.20f,
                                      1.0f) != CGFX_OK) {
      fprintf(stderr, "fill_rect failed\n");
    }

    cgfx_window_end_present_pass(win);
  }

  cgfx_window_destroy(win);
  cgfx_context_destroy(ctx);
  return 0;
}

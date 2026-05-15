#include <cgfx/cgfx_api.h>

#include <math.h>
#include <stdint.h>

static int ck(int cond, const char *msg, int code) {
  if (!cond) {
    /* stderr optional for minimal test portability */
    (void)msg;
    return code;
  }
  return 0;
}

/** Expected narrow advance stub for latin @ 13px (matches `narrow_advance_px` rounding). */
static uint32_t narrow_13(void) {
  const uint64_t n = ((uint64_t)13 * 62ULL + 99ULL) / 100ULL;
  return (uint32_t)((n > 0ULL ? n : 1ULL));
}

int main(void) {
  cgfx_context *ctx = NULL;
  if (ck(cgfx_context_create(&ctx) == CGFX_OK && ctx != NULL, "ctx_create", 1)) {
    return 1;
  }

  cgfx_font_id fid = CGFX_FONT_ID_INVALID;
  if (ck(cgfx_font_builtin_acquire(ctx, CGFX_FONT_BUILTIN_MONO_STUB, &fid) == CGFX_OK &&
             fid == CGFX_FONT_ID_BUILTIN_DEFAULT,
         "builtin_acquire", 2)) {
    cgfx_context_destroy(ctx);
    return 2;
  }

  cgfx_font_id sel = CGFX_FONT_ID_INVALID;
  if (ck(cgfx_context_text_font_selected(ctx, &sel) == CGFX_OK &&
             sel == CGFX_FONT_ID_BUILTIN_DEFAULT,
         "default_selected_font", 3)) {
    cgfx_context_destroy(ctx);
    return 3;
  }

  if (ck(cgfx_context_text_font_select(ctx, fid) == CGFX_OK, "select_font", 4)) {
    cgfx_context_destroy(ctx);
    return 4;
  }

  cgfx_text_line_metrics m_hi = {};
  if (ck(cgfx_text_measure_utf8_line_pixels(ctx, fid, "Hi", 2, 13.f, 1.f,
                                           &m_hi) == CGFX_OK &&
             m_hi.logical_font_px == 13U &&
             m_hi.line_height_px == m_hi.ascent_px + m_hi.descent_px &&
             m_hi.width_px == narrow_13() * 2U,
         "measure_hi", 5)) {
    cgfx_context_destroy(ctx);
    return 5;
  }

  cgfx_text_line_metrics m_nl = {};
  const char nl[] = "Hi\r\nWide";
  if (ck(cgfx_text_measure_utf8_line_pixels(ctx, CGFX_FONT_ID_INVALID /*context default*/,
                                            nl, sizeof(nl) - 1U, 13.f, 1.f,
                                            &m_nl) == CGFX_OK &&
             m_nl.width_px == narrow_13() * 2U,
         "measure_newline_trunc", 6)) {
    cgfx_context_destroy(ctx);
    return 6;
  }

  /** U+3042 HIRAGANA A — wide deterministic cell (advance == font_px at 13). */
  cgfx_text_line_metrics m_wide = {};
  static const unsigned char hiragana_a_utf8[] = {0xE3, 0x81, 0x82};
  if (ck(cgfx_text_measure_utf8_line_pixels(ctx, fid, (const char *)hiragana_a_utf8,
                                            sizeof hiragana_a_utf8, 13.f, 1.f,
                                            &m_wide) == CGFX_OK &&
             m_wide.width_px == 13U && m_wide.width_px > narrow_13(),
         "measure_wide", 7)) {
    cgfx_context_destroy(ctx);
    return 7;
  }

  /** dpi_scale drives logical pixels (2x → 26px body). */
  cgfx_text_line_metrics m_scaled = {};
  if (ck(cgfx_text_measure_utf8_line_pixels(ctx, fid, "A", 1, 13.f, 2.f,
                                            &m_scaled) == CGFX_OK &&
             m_scaled.logical_font_px == 26U,
         "measure_dpi_scale", 8)) {
    cgfx_context_destroy(ctx);
    return 8;
  }

  cgfx_text_line_metrics bogus = {};
  if (ck(cgfx_text_measure_utf8_line_pixels(NULL, fid, "a", 1, 13.f, 1.f, &bogus) ==
             CGFX_ERROR_INVALID_ARGUMENT,
         "null_context_invalid", 9)) {
    cgfx_context_destroy(ctx);
    return 9;
  }

  cgfx_font_id bad = CGFX_FONT_ID_BUILTIN_DEFAULT + UINT64_C(910);
  if (ck(cgfx_text_measure_utf8_line_pixels(ctx, bad, "a", 1, 13.f, 1.f, &bogus) ==
             CGFX_ERROR_INVALID_ARGUMENT,
         "bad_font_invalid", 10)) {
    cgfx_context_destroy(ctx);
    return 10;
  }

  if (ck(cgfx_text_measure_utf8_line_pixels(ctx, fid, "", 1, NAN, 1.f, &bogus) ==
             CGFX_ERROR_INVALID_ARGUMENT,
         "nan_size_invalid", 11)) {
    cgfx_context_destroy(ctx);
    return 11;
  }

  cgfx_context_destroy(ctx);
  return 0;
}

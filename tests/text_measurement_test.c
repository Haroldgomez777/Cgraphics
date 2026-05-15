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

/** Expected width for "Hi" at 13px from built-in scaled NotoSans via the same path as widgets. */
static uint32_t hi_width_13(cgfx_context *ctx, cgfx_font_id fid) {
  cgfx_text_line_metrics m_h = {};
  cgfx_text_line_metrics m_i = {};
  if (cgfx_text_measure_utf8_line_pixels(ctx, fid, "H", 1, 13.f, 1.f, &m_h) != CGFX_OK) {
    return 0U;
  }
  if (cgfx_text_measure_utf8_line_pixels(ctx, fid, "i", 1, 13.f, 1.f, &m_i) != CGFX_OK) {
    return 0U;
  }
  return m_h.width_px + m_i.width_px;
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
             m_hi.line_height_px >= m_hi.ascent_px + m_hi.descent_px &&
             m_hi.width_px == hi_width_13(ctx, fid),
         "measure_hi", 5)) {
    cgfx_context_destroy(ctx);
    return 5;
  }

  cgfx_text_line_metrics m_nl = {};
  const char nl[] = "Hi\r\nWide";
  if (ck(cgfx_text_measure_utf8_line_pixels(ctx, CGFX_FONT_ID_INVALID /*context default*/,
                                            nl, sizeof(nl) - 1U, 13.f, 1.f,
                                            &m_nl) == CGFX_OK &&
             m_nl.width_px == hi_width_13(ctx, fid),
         "measure_newline_trunc", 6)) {
    cgfx_context_destroy(ctx);
    return 6;
  }

  /** Hiragana glyph should be at least as wide as a typical Latin letter at the same px. */
  cgfx_text_line_metrics m_narrow = {};
  if (cgfx_text_measure_utf8_line_pixels(ctx, fid, "A", 1, 13.f, 1.f, &m_narrow) != CGFX_OK ||
      m_narrow.width_px == 0U) {
    cgfx_context_destroy(ctx);
    return 7;
  }
  cgfx_text_line_metrics m_wide = {};
  static const unsigned char hiragana_a_utf8[] = {0xE3, 0x81, 0x82};
  if (ck(cgfx_text_measure_utf8_line_pixels(ctx, fid, (const char *)hiragana_a_utf8,
                                            sizeof hiragana_a_utf8, 13.f, 1.f,
                                            &m_wide) == CGFX_OK &&
             m_wide.width_px >= m_narrow.width_px,
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

  cgfx_font_id mono = CGFX_FONT_ID_INVALID;
  if (ck(cgfx_font_builtin_acquire_mono_stub(ctx, &mono) == CGFX_OK &&
             mono == CGFX_FONT_ID_BUILTIN_DEFAULT,
         "mono_stub_acquire_alias", 12)) {
    cgfx_context_destroy(ctx);
    return 12;
  }

  if (ck(cgfx_context_set_text_font(ctx, fid) == CGFX_OK &&
             cgfx_context_get_text_font(ctx, &sel) == CGFX_OK && sel == fid,
         "set_get_text_font_aliases", 13)) {
    cgfx_context_destroy(ctx);
    return 13;
  }

  cgfx_text_line_metrics m_cstr = {0};
  if (ck(cgfx_text_measure_utf8_line_cstr_pixels(ctx, fid, "Hi", 13.f, 1.f,
                                                 &m_cstr) == CGFX_OK &&
             m_cstr.width_px == m_hi.width_px,
         "measure_cstr_matches_explicit_len", 14)) {
    cgfx_context_destroy(ctx);
    return 14;
  }

  cgfx_context_destroy(ctx);
  return 0;
}

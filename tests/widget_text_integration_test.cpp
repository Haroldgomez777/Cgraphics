/** Phase 7: offline layout + BasicWidgets text command list matches standalone measures. */

#include "layout/flex_layout.hpp"
#include "render/render_command_list.hpp"
#include "style/style_resolution.hpp"
#include "style/ui_theme.hpp"
#include "style/widget_style_overrides.hpp"
#include "text/text_layout_stub.hpp"
#include "widgets/basic_widgets.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>

#include <algorithm>
#include <climits>

#include <vector>

#include "core/widget_tree.hpp"

namespace {

struct FillGeom {
  int32_t x{};
  int32_t y{};
  uint32_t w{};
  uint32_t h{};
};

FillGeom nth_fill_rect(const cgfx::RenderCommandList &cmds, size_t want_index) {
  size_t ix = 0;
  for (const auto &c : cmds.commands()) {
    if (c.type == cgfx::RenderCommandType::FillRect) {
      if (ix == want_index) {
        return FillGeom{c.fill_rect.x_px, c.fill_rect.y_px, c.fill_rect.width_px,
                        c.fill_rect.height_px};
      }
      ++ix;
    }
  }
  return FillGeom{};
}

void sum_glyph_pass_by_index(const cgfx::RenderCommandList &cmds, size_t want_pass,
                             uint32_t *out_sum_w, size_t *out_quad_count,
                             int32_t *first_dst_x) noexcept {
  *out_sum_w = 0U;
  *out_quad_count = 0U;
  if (first_dst_x) {
    *first_dst_x = 0;
  }
  size_t n = 0;
  const std::vector<cgfx::RenderGlyphQuadItem> &q = cmds.glyph_quad_storage();
  for (const auto &c : cmds.commands()) {
    if (c.type != cgfx::RenderCommandType::GlyphAtlasPass) {
      continue;
    }
    if (n != want_pass) {
      ++n;
      continue;
    }
    const size_t off = c.glyph_atlas_pass.quad_storage_offset;
    const size_t qc = c.glyph_atlas_pass.quad_count;
    *out_quad_count = qc;
    uint32_t acc = 0U;
    for (size_t i = 0; i < qc; ++i) {
      acc += q[off + i].dst_w_px;
    }
    *out_sum_w = acc;
    if (qc > 0U && first_dst_x) {
      *first_dst_x = q[off].dst_x_px;
    }
    return;
  }
}

bool glyph_atlas_pass_has_visible_mask(const cgfx::RenderCommandList &cmds,
                                       size_t want_pass_idx) noexcept {
  size_t n = 0;
  const std::vector<uint8_t> &blob = cmds.glyph_atlas_pixel_blob();
  for (const auto &c : cmds.commands()) {
    if (c.type != cgfx::RenderCommandType::GlyphAtlasPass) {
      continue;
    }
    if (n != want_pass_idx) {
      ++n;
      continue;
    }
    const auto &gap = c.glyph_atlas_pass;
    const size_t rgba_off = gap.rgba_byte_offset;
    const size_t rgba_len = gap.rgba_byte_count;
    if (rgba_off > blob.size() || rgba_len > blob.size() - rgba_off ||
        rgba_len < 4U) {
      return false;
    }
    for (size_t i = rgba_off + 3U; i < rgba_off + rgba_len; i += 4U) {
      if (blob[i] != 0U) {
        return true;
      }
    }
    return false;
  }
  return false;
}

} // namespace

int main() {
  using cgfx::BasicWidgets;
  using cgfx::RenderCommandList;
  using cgfx::UiTheme;
  using cgfx::WidgetStyleOverrides;
  using cgfx::WidgetTree;
  using cgfx::run_flex_layout;

  WidgetTree tree{};
  BasicWidgets widgets{};
  const cgfx_widget_id root = tree.root_id();

  cgfx_widget_id panel = CGFX_WIDGET_ID_NONE;
  assert(widgets.create_panel(tree, root, &panel) == CGFX_OK);
  cgfx_widget_id lbl = CGFX_WIDGET_ID_NONE;
  assert(widgets.create_label(tree, panel, &lbl) == CGFX_OK);
  cgfx_widget_id btn = CGFX_WIDGET_ID_NONE;
  assert(widgets.create_button(tree, panel, &btn) == CGFX_OK);

  constexpr uint32_t kW = 320U;
  constexpr uint32_t kH = 240U;

  assert(tree.set_width(root, CGFX_LAYOUT_SIZE_FIXED, kW) == CGFX_OK);
  assert(tree.set_height(root, CGFX_LAYOUT_SIZE_FIXED, kH) == CGFX_OK);
  assert(tree.set_layout_axis(root, CGFX_LAYOUT_AXIS_COLUMN) == CGFX_OK);

  assert(tree.set_width(panel, CGFX_LAYOUT_SIZE_AUTO, 0) == CGFX_OK);
  assert(tree.set_height(panel, CGFX_LAYOUT_SIZE_FIXED, 120) == CGFX_OK);
  assert(tree.set_layout_axis(panel, CGFX_LAYOUT_AXIS_COLUMN) == CGFX_OK);

  assert(tree.set_width(lbl, CGFX_LAYOUT_SIZE_AUTO, 0) == CGFX_OK);
  assert(tree.set_height(lbl, CGFX_LAYOUT_SIZE_FIXED, 32) == CGFX_OK);

  assert(tree.set_width(btn, CGFX_LAYOUT_SIZE_AUTO, 0) == CGFX_OK);
  assert(tree.set_height(btn, CGFX_LAYOUT_SIZE_FIXED, 40) == CGFX_OK);

  assert(widgets.set_utf8_text(lbl, cgfx::BasicWidgetKind::Label, "Pi", 2) == CGFX_OK);
  assert(widgets.set_utf8_text(btn, cgfx::BasicWidgetKind::Button, "Ok", 2) == CGFX_OK);

  run_flex_layout(tree, kW, kH);

  const UiTheme theme = UiTheme::make_phase5_builtin();
  WidgetStyleOverrides ovs{};
  const cgfx::WidgetStyleOverrideRecord *norec = nullptr;

  const float sp_label =
      cgfx::style_resolution::resolve_label_font_size_sp(theme, norec);
  const uint32_t fpx_label = cgfx::text_logical_font_px_round(sp_label, 1.f);
  const cgfx::TextLineMetrics m_label =
      cgfx::text_measure_utf8_line_stub(fpx_label, "Pi", 2);
  cgfx_layout_rect lb_bounds{};
  assert(tree.get_bounds(lbl, &lb_bounds) == CGFX_OK);
  cgfx::TextPlaceholderBox plan_label{};
  cgfx::text_layout_placeholder_centered(lb_bounds, m_label, lb_bounds.width,
                                         &plan_label);

  const float sp_btn =
      cgfx::style_resolution::resolve_button_font_size_sp(theme, norec);
  const uint32_t fpx_btn = cgfx::text_logical_font_px_round(sp_btn, 1.f);
  const cgfx::TextLineMetrics m_bt =
      cgfx::text_measure_utf8_line_stub(fpx_btn, "Ok", 2);
  cgfx_layout_rect btn_bounds{};
  assert(tree.get_bounds(btn, &btn_bounds) == CGFX_OK);
  const uint32_t pad = static_cast<uint32_t>(std::floor(theme.padding_sm_px));
  const uint32_t pad_clamped = pad < 1U ? 1U : pad;
  const uint32_t inner_w =
      btn_bounds.width > pad_clamped * 2U ? btn_bounds.width - pad_clamped * 2U
                                        : btn_bounds.width;
  cgfx::TextPlaceholderBox plan_bt{};
  cgfx::text_layout_placeholder_centered(btn_bounds, m_bt, inner_w, &plan_bt);

  RenderCommandList cmds{};
  assert(widgets.paint(tree, cmds, theme, ovs, 1.0f) == CGFX_OK);

  /** preorder: panel face, label glyph pass, button face, caption glyph pass */
  const FillGeom g_panel = nth_fill_rect(cmds, 0);
  cgfx_layout_rect panel_bounds{};
  assert(tree.get_bounds(panel, &panel_bounds) == CGFX_OK);
  assert(g_panel.w == panel_bounds.width);
  assert(g_panel.h == panel_bounds.height);
  assert(g_panel.x == panel_bounds.x);
  assert(g_panel.y == panel_bounds.y);

  const FillGeom g_btn_face = nth_fill_rect(cmds, 1);
  assert(g_btn_face.w == btn_bounds.width);
  assert(g_btn_face.h == btn_bounds.height);
  assert(g_btn_face.x == btn_bounds.x);
  assert(g_btn_face.y == btn_bounds.y);

  uint32_t sum_lbl{};
  size_t qc_lbl{};
  int32_t first_lbl_x{};
  sum_glyph_pass_by_index(cmds, 0U, &sum_lbl, &qc_lbl, &first_lbl_x);
  assert(sum_lbl == m_label.width_px);
  assert(qc_lbl == 2U);
  const uint64_t clipped_lbl =
      m_label.width_px > plan_label.width_px ? plan_label.width_px : m_label.width_px;
  const int64_t expect_lbl_x64 =
      static_cast<int64_t>(plan_label.origin_x) +
      (static_cast<int64_t>(plan_label.width_px) - static_cast<int64_t>(clipped_lbl)) /
          INT64_C(2);
  const int32_t expect_lbl_x = static_cast<int32_t>(std::clamp(
      expect_lbl_x64, static_cast<int64_t>(INT32_MIN),
      static_cast<int64_t>(INT32_MAX)));
  assert(first_lbl_x == expect_lbl_x);

  uint32_t sum_bt{};
  size_t qc_bt{};
  sum_glyph_pass_by_index(cmds, 1U, &sum_bt, &qc_bt, nullptr);
  assert(sum_bt == m_bt.width_px);
  assert(qc_bt == 2U);

  assert(glyph_atlas_pass_has_visible_mask(cmds, 0U));
  assert(glyph_atlas_pass_has_visible_mask(cmds, 1U));

  return 0;
}

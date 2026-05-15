/** Phase 7: offline layout + BasicWidgets placeholder geometry matches standalone measures. */

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
  const cgfx::TextLineMetrics m_btn =
      cgfx::text_measure_utf8_line_stub(fpx_btn, "Ok", 2);
  cgfx_layout_rect btn_bounds{};
  assert(tree.get_bounds(btn, &btn_bounds) == CGFX_OK);
  const uint32_t pad = static_cast<uint32_t>(std::floor(theme.padding_sm_px));
  const uint32_t pad_clamped = pad < 1U ? 1U : pad;
  const uint32_t inner_w =
      btn_bounds.width > pad_clamped * 2U ? btn_bounds.width - pad_clamped * 2U
                                        : btn_bounds.width;
  cgfx::TextPlaceholderBox plan_btn{};
  cgfx::text_layout_placeholder_centered(btn_bounds, m_btn, inner_w, &plan_btn);

  RenderCommandList cmds{};
  assert(widgets.paint(tree, cmds, theme, ovs, 1.0f) == CGFX_OK);

  const FillGeom g_label = nth_fill_rect(cmds, 1);
  assert(g_label.w == plan_label.width_px);
  assert(g_label.h == plan_label.height_px);
  assert(g_label.x == plan_label.origin_x);
  assert(g_label.y == plan_label.origin_y);

  const FillGeom g_btn = nth_fill_rect(cmds, 3);
  assert(g_btn.w == plan_btn.width_px);
  assert(g_btn.h == plan_btn.height_px);
  assert(g_btn.x == plan_btn.origin_x);
  assert(g_btn.y == plan_btn.origin_y);

  return 0;
}

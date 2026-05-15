/** Phase 6: theme tokens, per-widget overrides, panel resolution order (override > legacy facet >
 *  theme), button states (offline).
 */

#include "core/widget_tree.hpp"
#include "layout/flex_layout.hpp"
#include "render/render_command_list.hpp"
#include "style/ui_theme.hpp"
#include "style/widget_style_overrides.hpp"
#include "widgets/basic_widgets.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>

namespace {

bool near_f(float a, float b) { return std::fabs(a - b) < 1e-4f; }

struct FillRectView {
  float r{};
  float g{};
  float b{};
  float a{};
};

FillRectView nth_fill_rect(const cgfx::RenderCommandList &cmds, size_t want_index) {
  size_t ix = 0;
  for (const auto &c : cmds.commands()) {
    if (c.type == cgfx::RenderCommandType::FillRect) {
      if (ix == want_index) {
        FillRectView v{};
        v.r = c.fill_rect.red;
        v.g = c.fill_rect.green;
        v.b = c.fill_rect.blue;
        v.a = c.fill_rect.alpha;
        return v;
      }
      ++ix;
    }
  }
  FillRectView bad{};
  return bad;
}

size_t fill_rect_count(const cgfx::RenderCommandList &cmds) {
  size_t n = 0;
  for (const auto &c : cmds.commands()) {
    if (c.type == cgfx::RenderCommandType::FillRect) {
      ++n;
    }
  }
  return n;
}

} // namespace

int main() {
  using cgfx::BasicWidgets;
  using cgfx::ButtonFaceQueryScenario;
  using cgfx::RenderCommandList;
  using cgfx::UiTheme;
  using cgfx::WidgetStyleOverrides;
  using cgfx::WidgetTree;
  using cgfx::run_flex_layout;

  WidgetTree tree{};
  BasicWidgets widgets{};
  WidgetStyleOverrides ovs{};
  const cgfx_widget_id root = tree.root_id();

  constexpr uint32_t kW = 240U;
  constexpr uint32_t kH = 200U;

  assert(tree.set_width(root, CGFX_LAYOUT_SIZE_FIXED, kW) == CGFX_OK);
  assert(tree.set_height(root, CGFX_LAYOUT_SIZE_FIXED, kH) == CGFX_OK);
  assert(tree.set_layout_axis(root, CGFX_LAYOUT_AXIS_COLUMN) == CGFX_OK);

  cgfx_widget_id panel = CGFX_WIDGET_ID_NONE;
  assert(widgets.create_panel(tree, root, &panel) == CGFX_OK);
  assert(tree.set_width(panel, CGFX_LAYOUT_SIZE_AUTO, 0) == CGFX_OK);
  assert(tree.set_height(panel, CGFX_LAYOUT_SIZE_FIXED, 80) == CGFX_OK);

  /** --- Theme token drives panel when facet is not legacy-touched --- */
  UiTheme warm = UiTheme::make_phase5_builtin();
  warm.panel_background = {0.11f, 0.22f, 0.33f, 1.f};

  run_flex_layout(tree, kW, kH);
  RenderCommandList cmds{};
  assert(widgets.paint(tree, cmds, warm, ovs, 1.0f) == CGFX_OK);
  assert(fill_rect_count(cmds) == 1U);
  {
    FillRectView v = nth_fill_rect(cmds, 0);
    assert(near_f(v.r, 0.11f) && near_f(v.g, 0.22f) && near_f(v.b, 0.33f));
  }

  /** --- Per-widget override wins over theme --- */
  assert(ovs.set_panel_background(panel, 0.01f, 0.02f, 0.03f, 1.f) == CGFX_OK);
  cmds.reset();
  assert(widgets.paint(tree, cmds, warm, ovs, 1.0f) == CGFX_OK);
  {
    FillRectView v = nth_fill_rect(cmds, 0);
    assert(near_f(v.r, 0.01f) && near_f(v.g, 0.02f) && near_f(v.b, 0.03f));
  }

  /** --- Legacy facet sits below Phase 6 panel override, above theme --- */
  assert(widgets.set_panel_background_rgba(panel, 0.5f, 0.5f, 0.5f, 1.f) == CGFX_OK);
  cmds.reset();
  assert(widgets.paint(tree, cmds, warm, ovs, 1.0f) == CGFX_OK);
  {
    FillRectView v = nth_fill_rect(cmds, 0);
    assert(near_f(v.r, 0.01f)); /* override still dominates */
    (void)v;
  }

  /** --- Clearing only CGFX_WIDGET_STYLE_OVERRIDE_PANEL_BACKGROUND exposes legacy facet --- */
  ovs.clear(panel, static_cast<uint32_t>(
                      cgfx::WidgetStyleOverrideMask::PanelBackground));
  float qr{}, qg{}, qb{}, qa{};
  assert(widgets.query_resolved_panel_background_rgba_normalized(
             panel, warm, ovs, &qr, &qg, &qb, &qa) == CGFX_OK);
  assert(near_f(qr, 0.5f) && near_f(qg, 0.5f) && near_f(qb, 0.5f));
  cmds.reset();
  assert(widgets.paint(tree, cmds, warm, ovs, 1.0f) == CGFX_OK);
  {
    FillRectView v = nth_fill_rect(cmds, 0);
    assert(near_f(v.r, qr) && near_f(v.g, qg) && near_f(v.b, qb));
    assert(near_f(v.r, 0.5f) && near_f(v.g, 0.5f) && near_f(v.b, 0.5f));
    (void)v;
  }

  ovs.clear_all(panel);
  cmds.reset();
  assert(widgets.paint(tree, cmds, warm, ovs, 1.0f) == CGFX_OK);
  {
    FillRectView v = nth_fill_rect(cmds, 0);
    assert(near_f(v.r, 0.5f) && near_f(v.g, 0.5f) && near_f(v.b, 0.5f));
  }

  /** --- Hover paint path (no platform window needed) --- */
  cgfx_widget_id btn_only = CGFX_WIDGET_ID_NONE;
  assert(widgets.create_button(tree, root, &btn_only) == CGFX_OK);
  assert(tree.set_width(btn_only, CGFX_LAYOUT_SIZE_AUTO, 0) == CGFX_OK);
  assert(tree.set_height(btn_only, CGFX_LAYOUT_SIZE_FIXED, 48) == CGFX_OK);

  UiTheme ui2 = UiTheme::make_phase5_builtin();
  ui2.button_background_normal = {0.1f, 0.2f, 0.3f, 1.f};
  ui2.button_background_hover = {0.9f, 0.1f, 0.1f, 1.f};

  cmds.reset();
  run_flex_layout(tree, kW, kH);
  ovs.clear_all(panel); /* leftover cleared */
  assert(widgets.paint(tree, cmds, ui2, ovs, 1.0f) == CGFX_OK);

  cgfx_layout_rect btn_bounds{};
  assert(tree.get_bounds(btn_only, &btn_bounds) == CGFX_OK);
  const int32_t hx =
      btn_bounds.x + static_cast<int32_t>(btn_bounds.width / 2);
  const int32_t hy =
      btn_bounds.y + static_cast<int32_t>(btn_bounds.height / 2);
  widgets.on_mouse_move_logical(tree, btn_only, hx, hy);
  cmds.reset();
  assert(widgets.paint(tree, cmds, ui2, ovs, 1.0f) == CGFX_OK);

  assert(fill_rect_count(cmds) >= 2U);
  {
    const FillRectView panel_paint = nth_fill_rect(cmds, 0);
    (void)panel_paint;
    const FillRectView button_face = nth_fill_rect(cmds, 1);
    assert(near_f(button_face.r, 0.9f) && near_f(button_face.g, 0.1f) &&
           near_f(button_face.b, 0.1f));
  }

  /** --- Button state resolution (query API mirrors paint combiner) --- */
  float pr{}, pg{}, pb{}, pa{};
  assert(widgets.query_resolved_button_face_rgba_normalized(
             btn_only, ui2, ovs, ButtonFaceQueryScenario::Normal, &pr, &pg, &pb,
             &pa) == CGFX_OK);
  assert(near_f(pr, 0.1f) && near_f(pg, 0.2f) && near_f(pb, 0.3f));

  assert(widgets.query_resolved_button_face_rgba_normalized(
             btn_only, ui2, ovs, ButtonFaceQueryScenario::Pressed, &pr, &pg, &pb,
             &pa) == CGFX_OK);
  UiTheme builtins = UiTheme::make_phase5_builtin();
  assert(near_f(pr, builtins.button_background_pressed.r));

  UiTheme ui_ov = builtins;
  assert(ovs.set_button_background_pressed(btn_only, 0.05f, 0.06f, 0.07f,
                                         1.f) == CGFX_OK);
  assert(widgets.query_resolved_button_face_rgba_normalized(
             btn_only, ui_ov, ovs, ButtonFaceQueryScenario::Pressed, &pr, &pg, &pb,
             &pa) == CGFX_OK);
  assert(near_f(pr, 0.05f) && near_f(pg, 0.06f) && near_f(pb, 0.07f));

  return 0;
}

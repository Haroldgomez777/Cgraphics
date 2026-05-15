/** Phase 8: easing math, deterministic timeline stepping, paint integration. */

#include <cgfx/cgfx_api.h>

#include "animation/easing.hpp"
#include "animation/widget_animation_system.hpp"
#include "core/widget_tree.hpp"
#include "layout/flex_layout.hpp"
#include "render/render_command_list.hpp"
#include "style/ui_theme.hpp"
#include "style/widget_style_overrides.hpp"
#include "widgets/basic_widgets.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace {

bool near_f(float a, float b) { return std::fabs(a - b) < 1e-4f; }

struct FillRectView {
  int32_t x{};
  int32_t y{};
  float r{};
  float g{};
  float b{};
  float a{};
};

FillRectView first_fill_rect(const cgfx::RenderCommandList &cmds) {
  for (const auto &c : cmds.commands()) {
    if (c.type == cgfx::RenderCommandType::FillRect) {
      return FillRectView{c.fill_rect.x_px, c.fill_rect.y_px, c.fill_rect.red,
                          c.fill_rect.green, c.fill_rect.blue, c.fill_rect.alpha};
    }
  }
  return {};
}

} // namespace

int main() {
  assert(near_f(cgfx::animation_easing::linear(0.37f), 0.37f));
  assert(near_f(cgfx::animation_easing::in_out_quad(0.5f), 0.5f));
  assert(near_f(cgfx::animation_easing::apply(CGFX_ANIM_EASE_LINEAR, 0.25f), 0.25f));
  /** Ease-out quad: shallow start, fast finish — must exceed linear at mid-upper t. */
  const float oq = cgfx::animation_easing::apply(CGFX_ANIM_EASE_OUT_QUAD, 0.75f);
  assert(oq > 0.75f);

  cgfx::WidgetAnimationSystem sys{};
  cgfx_widget_id w1 = 42;
  cgfx_animation_id id{};
  assert(sys.start_translate(w1, 0.f, 0.f, 100.f, -50.f, 10.f, CGFX_ANIM_EASE_LINEAR,
                             &id) == CGFX_OK);
  sys.advance(2.5); /** 25% */
  cgfx::AnimPaintMod m{};
  assert(sys.try_get_mod(w1, &m));
  assert(m.has_translate);
  assert(near_f(m.translate_x_px, 25.f));
  assert(near_f(m.translate_y_px, -12.5f));
  sys.advance(100.); /** finish hold */
  assert(sys.try_get_mod(w1, &m));
  assert(near_f(m.translate_x_px, 100.f));
  assert(near_f(m.translate_y_px, -50.f));

  /** ---- Paint path: fill + opacity multiply ---- */
  cgfx::WidgetTree tree{};
  cgfx::BasicWidgets widgets{};
  cgfx::WidgetStyleOverrides ovs{};
  const cgfx_widget_id root = tree.root_id();

  constexpr uint32_t kW = 120U;
  constexpr uint32_t kH = 100U;
  assert(tree.set_width(root, CGFX_LAYOUT_SIZE_FIXED, kW) == CGFX_OK);
  assert(tree.set_height(root, CGFX_LAYOUT_SIZE_FIXED, kH) == CGFX_OK);
  assert(tree.set_layout_axis(root, CGFX_LAYOUT_AXIS_COLUMN) == CGFX_OK);

  cgfx_widget_id panel = CGFX_WIDGET_ID_NONE;
  assert(widgets.create_panel(tree, root, &panel) == CGFX_OK);
  assert(tree.set_width(panel, CGFX_LAYOUT_SIZE_AUTO, 0) == CGFX_OK);
  assert(tree.set_height(panel, CGFX_LAYOUT_SIZE_FIXED, 40) == CGFX_OK);

  run_flex_layout(tree, kW, kH);

  cgfx_layout_rect bounds{};
  assert(tree.get_bounds(panel, &bounds) == CGFX_OK);

  cgfx::WidgetAnimationSystem paint_anim{};
  cgfx_animation_id oid{};
  assert(paint_anim.start_opacity(panel, 1.f, 0.5f, 2.f, CGFX_ANIM_EASE_LINEAR,
                                  &oid) == CGFX_OK);
  paint_anim.advance(1.0); /** 50% */
  cgfx::UiTheme theme = cgfx::UiTheme::make_phase5_builtin();

  cgfx::RenderCommandList cmds{};
  assert(widgets.paint(tree, cmds, theme, ovs, 1.f, &paint_anim) == CGFX_OK);
  FillRectView fr = first_fill_rect(cmds);
  assert(fr.x == bounds.x);
  assert(fr.y == bounds.y);
  assert(near_f(fr.a, 0.75f)); /** resolved theme alpha ~1 * 0.75 lerp midpoint */

  /** Fill override replaces tint; linear halfway between red and blue. */
  cmds.reset();
  cgfx_animation_id fid{};
  cgfx_animation_id dummy{};
  paint_anim.stop(oid); /** clear opacity clip */
  (void)dummy;
  assert(paint_anim.start_fill_rgba(panel, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f,
                                    10.f, CGFX_ANIM_EASE_LINEAR, &fid) == CGFX_OK);
  paint_anim.advance(5.f);
  assert(widgets.paint(tree, cmds, theme, ovs, 1.f, &paint_anim) == CGFX_OK);
  fr = first_fill_rect(cmds);
  assert(near_f(fr.r, 0.5f) && near_f(fr.g, 0.f) && near_f(fr.b, 0.5f));

  /** Translate shifts emitted rect vs layout bounds */
  cmds.reset();
  paint_anim.stop(fid);
  assert(paint_anim.start_translate(panel, 0.f, 0.f, 4.f, -2.f, 10.f,
                                    CGFX_ANIM_EASE_LINEAR, &fid) == CGFX_OK);
  paint_anim.advance(5.f); /** halfway */
  assert(widgets.paint(tree, cmds, theme, ovs, 1.f, &paint_anim) == CGFX_OK);
  fr = first_fill_rect(cmds);
  assert(fr.x == bounds.x + 2);
  assert(fr.y == bounds.y - 1);

  return 0;
}

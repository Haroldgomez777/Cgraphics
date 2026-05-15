/** Paint translate adjusts widget hit rectangles for routed pointer parity (offline). */

#include "animation/widget_animation_system.hpp"
#include "core/widget_tree.hpp"
#include "layout/flex_layout.hpp"
#include "widgets/basic_widgets.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>

#include <climits>

namespace {

bool nop_filter_thunk(cgfx_widget_id /*id*/, void * /*user*/) noexcept { return true; }

/** Half-open containment matching `logical_point_in_rect` in WidgetTree tests. */
bool point_in_logical_rect(int32_t x, int32_t y,
                           const cgfx_layout_rect &r) noexcept {
  if (r.width == 0U || r.height == 0U) {
    return false;
  }
  const int64_t ix = static_cast<int64_t>(x);
  const int64_t iy = static_cast<int64_t>(y);
  const int64_t bx0 = static_cast<int64_t>(r.x);
  const int64_t by0 = static_cast<int64_t>(r.y);
  const int64_t bx1 = bx0 + static_cast<int64_t>(r.width);
  const int64_t by1 = by0 + static_cast<int64_t>(r.height);
  return ix >= bx0 && iy >= by0 && ix < bx1 && iy < by1;
}

} // namespace

int main() {
  cgfx::WidgetTree tree{};
  cgfx::BasicWidgets widgets{};
  cgfx::WidgetAnimationSystem anim{};

  const cgfx_widget_id root = tree.root_id();
  /** Row root so a fixed-width panel keeps its declared main-axis width (column would stretch
   *  children to the full viewport width). */
  assert(tree.set_layout_axis(root, CGFX_LAYOUT_AXIS_ROW) == CGFX_OK);
  assert(tree.set_width(root, CGFX_LAYOUT_SIZE_FIXED, 900) == CGFX_OK);
  assert(tree.set_height(root, CGFX_LAYOUT_SIZE_FIXED, 300) == CGFX_OK);

  cgfx_widget_id panel = CGFX_WIDGET_ID_NONE;
  assert(widgets.create_panel(tree, root, &panel) == CGFX_OK);
  assert(tree.set_width(panel, CGFX_LAYOUT_SIZE_FIXED, 80) == CGFX_OK);
  assert(tree.set_height(panel, CGFX_LAYOUT_SIZE_FIXED, 40) == CGFX_OK);

  run_flex_layout(tree, 900U, 300U);

  cgfx_layout_rect p{};
  assert(tree.get_bounds(panel, &p) == CGFX_OK);
  assert(p.width > 2U);

  cgfx_animation_id tid{};
  assert(anim.start_translate(panel, 0.f, 0.f, static_cast<float>(p.width), 0.f, 10.f,
                              CGFX_ANIM_EASE_LINEAR, &tid) == CGFX_OK);
  anim.advance(10.f);

  /* Left interior of layout box; after +width translate the visual box starts at p.x+width. */
  const int32_t hit_x_inside_layout_only =
      static_cast<int32_t>(p.x + static_cast<int32_t>(p.width / 4));
  const int32_t cy =
      static_cast<int32_t>(p.y + static_cast<int64_t>(p.height) / 2);

  assert(tree.hit_test_logical(hit_x_inside_layout_only, cy) == panel);
  assert(tree.hit_test_logical_paint_visual(hit_x_inside_layout_only, cy, &anim) ==
         root);

  cgfx::AnimPaintMod samp{};
  assert(anim.try_get_mod(panel, &samp));
  assert(samp.has_translate);
  cgfx_layout_rect shifted = p;
  shifted.x += static_cast<int32_t>(
      std::llround(static_cast<double>(samp.translate_x_px)));

  const int64_t inset =
      std::max<int64_t>(2LL, static_cast<int64_t>(p.width / 4U));
  const int64_t hit_x_paint_i64 = static_cast<int64_t>(shifted.x) + inset;
  assert(hit_x_paint_i64 < static_cast<int64_t>(shifted.x) +
                             static_cast<int64_t>(p.width));
  const int32_t hit_x_paint_only = static_cast<int32_t>(
      std::clamp(hit_x_paint_i64, static_cast<int64_t>(INT32_MIN),
                 static_cast<int64_t>(INT32_MAX)));

  assert(!point_in_logical_rect(hit_x_paint_only, cy, p));
  assert(point_in_logical_rect(hit_x_paint_only, cy, shifted));

  const cgfx_widget_id layout_pick = tree.hit_test_logical(hit_x_paint_only, cy);
  assert(layout_pick != panel); /** Outside intrinsic bounds */

  assert(tree.hit_test_logical_paint_visual(hit_x_paint_only, cy, &anim) == panel);

  assert(tree.hit_test_logical_filtered_paint_visual(
             hit_x_paint_only, cy, &nop_filter_thunk, nullptr, &anim) == panel);

  return 0;
}

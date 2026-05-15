#include "core/widget_tree.hpp"
#include "layout/flex_layout.hpp"

#include <cassert>
#include <cstdint>

namespace {

static cgfx_layout_rect bounds_of(const cgfx::WidgetTree &t, uint64_t id) {
  cgfx_layout_rect r{};
  const cgfx_result rc = t.get_bounds(id, &r);
  assert(rc == CGFX_OK);
  return r;
}

} // namespace

int main() {
  cgfx::WidgetTree tree{};
  const uint64_t root = tree.root_id();
  assert(root != 0);

  assert(tree.destroy_subtree(root) == CGFX_ERROR_INVALID_ARGUMENT);

  uint64_t leaf_a{};
  uint64_t leaf_b{};
  assert(tree.create_child(root, &leaf_a) == CGFX_OK);
  assert(tree.create_child(root, &leaf_b) == CGFX_OK);

  uint64_t mid{};
  assert(tree.create_child(leaf_a, &mid) == CGFX_OK);
  uint64_t tip{};
  assert(tree.create_child(mid, &tip) == CGFX_OK);

  /* Reparent cycle detection — evaluated before reshaping subtree order */
  assert(tree.set_parent(leaf_a, tip) != CGFX_OK); /* leaf_a ancestor of tip */
  assert(tree.set_parent(mid, tip) != CGFX_OK); /* tip descendant of mid */
  assert(tree.set_parent(mid, leaf_b) ==
         CGFX_OK); /* subtree detaches leaf_a branch */

  /* Structural reparent preserving stable ids */
  assert(tree.set_parent(tip, root) == CGFX_OK);

  assert(tree.destroy_subtree(leaf_a) == CGFX_OK);

  cgfx_layout_rect ignored{};
  assert(tree.get_bounds(leaf_a, &ignored) == CGFX_ERROR_INVALID_ARGUMENT);
  cgfx_layout_rect tip_alive{};
  assert(tree.get_bounds(tip, &tip_alive) == CGFX_OK);

  /* Horizontal flex-grow consumes leftover width */
  cgfx::WidgetTree flex{};
  const uint64_t fr = flex.root_id();
  assert(flex.set_layout_axis(fr, CGFX_LAYOUT_AXIS_ROW) == CGFX_OK);

  uint64_t left{};
  uint64_t middle{};
  uint64_t greedy{};
  assert(flex.create_child(fr, &left) == CGFX_OK);
  assert(flex.create_child(fr, &middle) == CGFX_OK);
  assert(flex.create_child(fr, &greedy) == CGFX_OK);

  assert(flex.set_width(left, CGFX_LAYOUT_SIZE_FIXED, 120U) == CGFX_OK);
  assert(flex.set_height(left, CGFX_LAYOUT_SIZE_AUTO, 0U) == CGFX_OK);

  assert(flex.set_width(middle, CGFX_LAYOUT_SIZE_FIXED, 140U) == CGFX_OK);
  assert(flex.set_height(middle, CGFX_LAYOUT_SIZE_AUTO, 0U) == CGFX_OK);

  assert(flex.set_width(greedy, CGFX_LAYOUT_SIZE_AUTO, 0U) == CGFX_OK);
  assert(flex.set_height(greedy, CGFX_LAYOUT_SIZE_AUTO, 0U) == CGFX_OK);
  assert(flex.set_flex_grow(greedy, 1.f) == CGFX_OK);

  run_flex_layout(flex, 900U, 600U);

  const cgfx_layout_rect bl = bounds_of(flex, left);
  const cgfx_layout_rect bm = bounds_of(flex, middle);
  const cgfx_layout_rect bg = bounds_of(flex, greedy);

  assert(bl.x == 0);
  assert(bm.x == static_cast<int32_t>(bl.width));
  assert(bg.x ==
         static_cast<int32_t>(static_cast<uint32_t>(bm.x) + bm.width));

  assert(bl.width == 120U);
  assert(bm.width == 140U);
  assert(bg.width == 640U); /* remainder after 260px of fixed sizing */
  assert(bl.height == 600U);
  assert(bg.height == 600U);

  /* Overflow shrink clamps total allocated main axis */
  cgfx::WidgetTree tight{};
  const uint64_t tr = tight.root_id();
  assert(tight.set_layout_axis(tr, CGFX_LAYOUT_AXIS_ROW) == CGFX_OK);

  uint64_t s0{};
  uint64_t s1{};
  uint64_t s2{};
  assert(tight.create_child(tr, &s0) == CGFX_OK);
  assert(tight.create_child(tr, &s1) == CGFX_OK);
  assert(tight.create_child(tr, &s2) == CGFX_OK);

  for (cgfx_widget_id wid : {s0, s1, s2}) {
    assert(tight.set_width(wid, CGFX_LAYOUT_SIZE_FIXED, 140U) == CGFX_OK);
    assert(tight.set_height(wid, CGFX_LAYOUT_SIZE_AUTO, 0U) == CGFX_OK);
    assert(tight.set_flex_shrink(wid, 1.f) == CGFX_OK);
  }

  run_flex_layout(tight, 300U, 200U);

  const cgfx_layout_rect t0 = bounds_of(tight, s0);
  const cgfx_layout_rect t1 = bounds_of(tight, s1);
  const cgfx_layout_rect t2 = bounds_of(tight, s2);

  const uint64_t summed =
      static_cast<uint64_t>(t0.width) + static_cast<uint64_t>(t1.width) +
      static_cast<uint64_t>(t2.width);

  assert(summed <= 305U); /* tolerate rounding jitter around 300 */
  assert(summed >= 285U);

  return 0;
}

#include "core/widget_tree.hpp"
#include "layout/flex_layout.hpp"

#include <cassert>

int main() {
  /* Deepest widget along a chain receives the inner hit */
  {
    cgfx::WidgetTree nested{};
    const uint64_t root = nested.root_id();
    assert(nested.set_layout_axis(root, CGFX_LAYOUT_AXIS_COLUMN) == CGFX_OK);
    uint64_t outer{};
    uint64_t inner{};
    assert(nested.create_child(root, &outer) == CGFX_OK);
    assert(nested.create_child(outer, &inner) == CGFX_OK);
    for (cgfx_widget_id w : {outer, inner}) {
      assert(nested.set_width(w, CGFX_LAYOUT_SIZE_AUTO, 0U) == CGFX_OK);
      assert(nested.set_height(w, CGFX_LAYOUT_SIZE_AUTO, 0U) == CGFX_OK);
      assert(nested.set_flex_grow(w, 1.f) == CGFX_OK);
    }

    run_flex_layout(nested, 200U, 200U);

    cgfx_layout_rect inner_rect{};
    assert(nested.get_bounds(inner, &inner_rect) == CGFX_OK);
    const uint32_t wi =
        inner_rect.width == 0U ? 1U : inner_rect.width;
    const uint32_t hi =
        inner_rect.height == 0U ? 1U : inner_rect.height;
    const int32_t cx =
        inner_rect.x + static_cast<int32_t>(wi / 2U);
    const int32_t cy =
        inner_rect.y + static_cast<int32_t>(hi / 2U);

    assert(nested.hit_test_logical(cx, cy) == inner);
  }

  /** Overlapping siblings: last sibling in parent's children vector is visited first
      (last-on-top stacking). Bounds manually aligned for deterministic overlap. */
  {
    cgfx::WidgetTree ovl{};
    const uint64_t r = ovl.root_id();
    assert(ovl.set_layout_axis(r, CGFX_LAYOUT_AXIS_ROW) == CGFX_OK);
    uint64_t a{};
    uint64_t b{};
    assert(ovl.create_child(r, &a) == CGFX_OK);
    assert(ovl.create_child(r, &b) == CGFX_OK);
    for (cgfx_widget_id w : {a, b}) {
      assert(ovl.set_width(w, CGFX_LAYOUT_SIZE_FIXED, 100U) == CGFX_OK);
      assert(ovl.set_height(w, CGFX_LAYOUT_SIZE_AUTO, 0U) == CGFX_OK);
    }
    run_flex_layout(ovl, 500U, 200U);

    size_t ia{};
    size_t ib{};
    assert(ovl.index_of_widget(a, ia) == CGFX_OK);
    assert(ovl.index_of_widget(b, ib) == CGFX_OK);

    const cgfx_layout_rect shared{10, 20, 200U, 100U};
    ovl.node_at_mut(ia).bounds = shared;
    ovl.node_at_mut(ib).bounds = shared;

    const int32_t hx = shared.x + 5;
    const int32_t hy = shared.y + 5;
    assert(ovl.hit_test_logical(hx, hy) == b);

    /** Reparenting appends onto the parent's child list ⇒ new last sibling ⇒ top */
    assert(ovl.set_parent(a, r) == CGFX_OK);
    assert(ovl.hit_test_logical(hx, hy) == a);

    /** Outside viewport layout rect → no ancestor contains (root gets full flex rect) */
    assert(ovl.hit_test_logical(-5, -5) == CGFX_WIDGET_ID_NONE);
  }

  /* Destroyed-handle validation falls back to root */
  {
    cgfx::WidgetTree t{};
    const uint64_t root = t.root_id();
    uint64_t doomed{};
    assert(t.create_child(root, &doomed) == CGFX_OK);
    assert(t.destroy_subtree(doomed) == CGFX_OK);

    assert(t.validated_widget_id_or_root(doomed) == root);
    assert(t.validated_widget_id_or_root(CGFX_WIDGET_ID_NONE) == root);
  }

  return 0;
}

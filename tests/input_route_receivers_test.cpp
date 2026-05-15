#include "core/input_propagation.hpp"
#include "core/widget_tree.hpp"
#include "layout/flex_layout.hpp"

#include <cassert>

int main() {
  cgfx::WidgetTree t{};
  const uint64_t root = t.root_id();
  uint64_t a{};
  uint64_t b{};
  uint64_t c{};
  assert(t.create_child(root, &a) == CGFX_OK);
  assert(t.create_child(a, &b) == CGFX_OK);
  assert(t.create_child(b, &c) == CGFX_OK);
  for (cgfx_widget_id w : {a, b, c}) {
    assert(t.set_width(w, CGFX_LAYOUT_SIZE_FIXED, 20U) == CGFX_OK);
    assert(t.set_height(w, CGFX_LAYOUT_SIZE_FIXED, 20U) == CGFX_OK);
  }
  run_flex_layout(t, 128U, 128U);

  std::vector<cgfx_widget_id> receivers;

  /** Default / explicit target-only: single leaf retains Phase 4.1 cardinality. */
  build_input_route_receiver_ids(t, c, CGFX_INPUT_PROPAGATION_TARGET_ONLY,
                                 receivers);
  assert(receivers.size() == 1U);
  assert(receivers[0] == c);

  /** Bubble expands inner → … → root (deterministic parent walk). */
  build_input_route_receiver_ids(t, c, CGFX_INPUT_PROPAGATION_BUBBLE_TO_PARENT,
                                 receivers);
  assert(receivers.size() == 4U);
  assert(receivers[0] == c);
  assert(receivers[1] == b);
  assert(receivers[2] == a);
  assert(receivers[3] == root);

  /** Miss / outside hit: unified single NONE slot for both modes. */
  build_input_route_receiver_ids(t, CGFX_WIDGET_ID_NONE,
                                 CGFX_INPUT_PROPAGATION_BUBBLE_TO_PARENT,
                                 receivers);
  assert(receivers.size() == 1U);
  assert(receivers[0] == CGFX_WIDGET_ID_NONE);

  /** Keyboard focus analogue: bubbling from rooted focus emits root alone. */
  build_input_route_receiver_ids(t, root, CGFX_INPUT_PROPAGATION_BUBBLE_TO_PARENT,
                                 receivers);
  assert(receivers.size() == 1U);
  assert(receivers[0] == root);

  return 0;
}

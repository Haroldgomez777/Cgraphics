#include "core/input_propagation.hpp"
#include "core/widget_tree.hpp"

namespace cgfx {

void build_input_route_receiver_ids(const WidgetTree &tree,
                                   cgfx_widget_id origin_target,
                                   cgfx_input_propagation_policy policy,
                                   std::vector<cgfx_widget_id> &out_ordered) {
  out_ordered.clear();
  if (origin_target == CGFX_WIDGET_ID_NONE) {
    out_ordered.push_back(CGFX_WIDGET_ID_NONE);
    return;
  }

  switch (policy) {
  default:
  case CGFX_INPUT_PROPAGATION_TARGET_ONLY:
    out_ordered.push_back(origin_target);
    return;
  case CGFX_INPUT_PROPAGATION_BUBBLE_TO_PARENT:
    tree.append_ancestors_leaf_to_root(origin_target, out_ordered);
    if (out_ordered.empty()) {
      /** Defensive fallback if @p origin_target became stale vs. routing snapshot */
      out_ordered.push_back(CGFX_WIDGET_ID_NONE);
    }
    return;
  }
}

} // namespace cgfx

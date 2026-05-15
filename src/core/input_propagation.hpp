#pragma once

#include <cgfx/cgfx_api.h>

#include <vector>

namespace cgfx {

class WidgetTree;

/** Fills @p out_ordered with widget ids for input delivery: inner target first, then
 *  each ancestor up to and including the tree root. Order is deterministic (parent
 *  chain by index). Used for **bubble** mode; **target-only** uses a single receiver.
 *
 *  @param origin_target Deepest logical target (hit-tested pointer target or focus
 *                       widget for keys). `CGFX_WIDGET_ID_NONE` yields a single `NONE`
 *                       receiver (one coalesced missed event).
 */
void build_input_route_receiver_ids(const WidgetTree &tree,
                                   cgfx_widget_id origin_target,
                                   cgfx_input_propagation_policy policy,
                                   std::vector<cgfx_widget_id> &out_ordered);

} // namespace cgfx

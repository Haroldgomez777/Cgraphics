#pragma once

#include <cgfx/cgfx_api.h>

#include <cstdint>

namespace cgfx {

/** One-line stderr trace for expanded pointer/keyboard routed streams. */
void emit_input_routing_route_trace(cgfx_event_type type, uint64_t sequence,
                                    cgfx_widget_id target_widget,
                                    cgfx_widget_id routed_widget,
                                    cgfx_input_propagation_policy policy) noexcept;

} // namespace cgfx

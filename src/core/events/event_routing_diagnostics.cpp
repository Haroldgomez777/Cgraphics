#include "core/events/event_routing_diagnostics.hpp"

#include <cstdio>

namespace cgfx {

void emit_input_routing_route_trace(cgfx_event_type type, uint64_t sequence,
                                    cgfx_widget_id target_widget,
                                    cgfx_widget_id routed_widget,
                                    cgfx_input_propagation_policy policy) noexcept {
  const char *tag = "unknown";
  switch (type) {
  case CGFX_EVENT_MOUSE_BUTTON:
    tag = "mbtn";
    break;
  case CGFX_EVENT_KEY:
    tag = "key";
    break;
  default:
    break;
  }

  const int pol = static_cast<int>(policy);

  std::fprintf(stderr, "cgfx-route %s seq=%llu tgt=%llu rtd=%llu pol=%d\n",
               tag,
               static_cast<unsigned long long>(sequence),
               static_cast<unsigned long long>(target_widget),
               static_cast<unsigned long long>(routed_widget), pol);
}

} // namespace cgfx

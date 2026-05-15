#pragma once

#include <cstdint>

namespace cgfx {

class WidgetTree;

/** Runs a deterministic flex-inspired measure + allocate pass for Phase 4. */
void run_flex_layout(WidgetTree &tree, uint32_t viewport_w_u32,
                       uint32_t viewport_h_u32) noexcept;

} // namespace cgfx

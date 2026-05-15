#pragma once

#include "text/text_layout_stub.hpp"

#include "render/render_command_list.hpp"

namespace cgfx {
namespace text_raster_backend {

/** Phase 7 seam: glyphs are not rasterized yet; widgets issue a deterministic fill-rect aligned
 *  to measured line metrics (`TextLineMetrics` + `text_layout_placeholder_centered`).
 *
 *  Phase 7.1: implement atlas upload + textured quads or SDF draws from this hook without
 *  changing widget measurement/planning code paths.
 */
inline void submit_glyph_rasterization_placeholder_todo(
    const TextPlaceholderBox & /*plan*/, const TextLineMetrics & /*metrics*/,
    RenderCommandList & /*cmds*/) noexcept {
  /* Intentionally empty — visual placeholder is emitted by basic_widgets paint. */
}

} // namespace text_raster_backend
} // namespace cgfx

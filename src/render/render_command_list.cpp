#include "render/render_command_list.hpp"

#include <new>

namespace cgfx {

void RenderCommandList::reset() { commands_.clear(); }

cgfx_result RenderCommandList::append_clear_color(float red, float green,
                                                  float blue, float alpha) {
  try {
    RenderCommand cmd{};
    cmd.type = RenderCommandType::ClearColor;
    cmd.clear.red = red;
    cmd.clear.green = green;
    cmd.clear.blue = blue;
    cmd.clear.alpha = alpha;
    commands_.push_back(cmd);
    return CGFX_OK;
  } catch (const std::bad_alloc &) {
    return CGFX_ERROR_OUT_OF_MEMORY;
  } catch (...) {
    return CGFX_ERROR_PLATFORM;
  }
}

} // namespace cgfx

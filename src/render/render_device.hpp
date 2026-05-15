#pragma once

#include <cgfx/cgfx_api.h>

#include "render/render_command_list.hpp"

#include <cstdint>
#include <memory>

namespace cgfx {

struct RenderFrameInfo {
  uint32_t width_px{0};
  uint32_t height_px{0};
  float dpi_scale{1.0f};
};

class RenderDevice {
public:
  virtual ~RenderDevice() = default;

  RenderDevice(const RenderDevice &) = delete;
  RenderDevice &operator=(const RenderDevice &) = delete;

  virtual cgfx_result begin_frame(const RenderFrameInfo &frame) = 0;
  virtual cgfx_result submit(const RenderCommandList &commands) = 0;
  virtual void end_frame() = 0;

protected:
  RenderDevice() = default;
};

std::unique_ptr<RenderDevice> cgfx_make_opengl_render_device();

} // namespace cgfx

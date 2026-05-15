#include "render/render_device.hpp"

#include <algorithm>
#include <cmath>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#elif defined(__linux__)
#include <GL/gl.h>
#endif

namespace cgfx {

namespace {

float clamp_unit(float value) noexcept {
  if (std::isnan(value)) {
    return 0.0f;
  }
  if (value < 0.0f) {
    return 0.0f;
  }
  if (value > 1.0f) {
    return 1.0f;
  }
  return value;
}

struct ClippedScissorPx {
  int x{0};
  int gl_y_bottom{0};
  int width{0};
  int height{0};
  bool drawable{false};
};

ClippedScissorPx clip_fill_rect_px(int32_t x_px, int32_t y_px, uint32_t width_px,
                                   uint32_t height_px, int frame_width_px,
                                   int frame_height_px) noexcept {
  ClippedScissorPx out{};
  if (frame_width_px <= 0 || frame_height_px <= 0) {
    return out;
  }

  const int x0 = (std::max)(static_cast<int>(x_px), 0);
  const int y0 = (std::max)(static_cast<int>(y_px), 0);
  const int x1 =
      (std::min)(static_cast<int>(x_px) + static_cast<int>(width_px),
                 frame_width_px);
  const int y1 =
      (std::min)(static_cast<int>(y_px) + static_cast<int>(height_px),
                 frame_height_px);
  const int clipped_w = x1 - x0;
  const int clipped_h = y1 - y0;
  if (clipped_w <= 0 || clipped_h <= 0) {
    return out;
  }

  const int gl_y_bottom = frame_height_px - y1;
  out.x = x0;
  out.gl_y_bottom = gl_y_bottom;
  out.width = clipped_w;
  out.height = clipped_h;
  out.drawable = true;
  return out;
}

void apply_fill_rect_via_scissor_clear(const ClippedScissorPx &clip, float red,
                                       float green, float blue, float alpha) {
  glScissor(clip.x, clip.gl_y_bottom, clip.width, clip.height);
  glClearColor(clamp_unit(red), clamp_unit(green), clamp_unit(blue),
               clamp_unit(alpha));
  glClear(GL_COLOR_BUFFER_BIT);
}

class OpenGlRenderDevice final : public RenderDevice {
public:
  OpenGlRenderDevice() = default;
  ~OpenGlRenderDevice() override = default;

  cgfx_result begin_frame(const RenderFrameInfo &frame) override {
    frame_width_px_ = static_cast<int>(frame.width_px);
    frame_height_px_ = static_cast<int>(frame.height_px);
    if (frame_width_px_ <= 0 || frame_height_px_ <= 0) {
      return CGFX_ERROR_INVALID_ARGUMENT;
    }
    glViewport(0, 0, frame_width_px_, frame_height_px_);
    return CGFX_OK;
  }

  cgfx_result submit(const RenderCommandList &commands) override {
    for (const RenderCommand &cmd : commands.commands()) {
      switch (cmd.type) {
      case RenderCommandType::ClearColor:
        glClearColor(clamp_unit(cmd.clear.red), clamp_unit(cmd.clear.green),
                     clamp_unit(cmd.clear.blue), clamp_unit(cmd.clear.alpha));
        glClear(GL_COLOR_BUFFER_BIT);
        break;
      case RenderCommandType::FillRect: {
        if (frame_width_px_ <= 0 || frame_height_px_ <= 0) {
          return CGFX_ERROR_PLATFORM;
        }

        const ClippedScissorPx clip = clip_fill_rect_px(
            cmd.fill_rect.x_px, cmd.fill_rect.y_px, cmd.fill_rect.width_px,
            cmd.fill_rect.height_px, frame_width_px_, frame_height_px_);
        if (!clip.drawable) {
          break;
        }

        glEnable(GL_SCISSOR_TEST);
        apply_fill_rect_via_scissor_clear(clip, cmd.fill_rect.red,
                                          cmd.fill_rect.green,
                                          cmd.fill_rect.blue,
                                          cmd.fill_rect.alpha);
        glDisable(GL_SCISSOR_TEST);
        break;
      }
      case RenderCommandType::FillRectBatch: {
        if (frame_width_px_ <= 0 || frame_height_px_ <= 0) {
          return CGFX_ERROR_PLATFORM;
        }

        const size_t storage_offset = cmd.fill_rect_batch.storage_offset;
        const size_t item_count = cmd.fill_rect_batch.item_count;
        const std::vector<RenderFillRectItem> &items =
            commands.rect_batch_storage();

        const size_t end_excl = storage_offset + item_count;
        if (storage_offset > items.size() || end_excl > items.size() ||
            end_excl < storage_offset) {
          return CGFX_ERROR_PLATFORM;
        }

        glEnable(GL_SCISSOR_TEST);

        float last_red = NAN;
        float last_green = NAN;
        float last_blue = NAN;
        float last_alpha = NAN;

        auto apply_cached_clear_color = [&](float r, float g, float b, float a) {
          // `last_*` are initially NaNs so the first drawable rect always binds
          // colors.
          if (!(r == last_red && g == last_green && b == last_blue &&
                a == last_alpha)) {

            glClearColor(clamp_unit(r), clamp_unit(g), clamp_unit(b),
                         clamp_unit(a));
            last_red = r;
            last_green = g;
            last_blue = b;
            last_alpha = a;
          }
        };

        for (size_t i = 0; i < item_count; ++i) {
          const RenderFillRectItem &item = items[storage_offset + i];
          const ClippedScissorPx clip = clip_fill_rect_px(
              item.x_px, item.y_px, item.width_px, item.height_px,
              frame_width_px_, frame_height_px_);
          if (!clip.drawable) {
            continue;
          }

          apply_cached_clear_color(item.red, item.green, item.blue,
                                   item.alpha);
          glScissor(clip.x, clip.gl_y_bottom, clip.width, clip.height);
          glClear(GL_COLOR_BUFFER_BIT);
        }

        glDisable(GL_SCISSOR_TEST);
        break;
      }
      default:
        return CGFX_ERROR_UNSUPPORTED;
      }
    }
    return CGFX_OK;
  }

  void end_frame() override {}

private:
  int frame_width_px_{0};
  int frame_height_px_{0};
};

} // namespace

std::unique_ptr<RenderDevice> cgfx_make_opengl_render_device() {
  return std::make_unique<OpenGlRenderDevice>();
}

} // namespace cgfx

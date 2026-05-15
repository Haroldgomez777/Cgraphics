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

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
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
      case RenderCommandType::GlyphAtlasPass: {
        if (frame_width_px_ <= 0 || frame_height_px_ <= 0) {
          return CGFX_ERROR_PLATFORM;
        }
        const uint32_t aw = cmd.glyph_atlas_pass.atlas_width_px;
        const uint32_t ah = cmd.glyph_atlas_pass.atlas_height_px;
        const size_t rgba_off = cmd.glyph_atlas_pass.rgba_byte_offset;
        const size_t rgba_len = cmd.glyph_atlas_pass.rgba_byte_count;
        const size_t q_off = cmd.glyph_atlas_pass.quad_storage_offset;
        const size_t q_count = cmd.glyph_atlas_pass.quad_count;
        const std::vector<uint8_t> &blob = commands.glyph_atlas_pixel_blob();
        const std::vector<RenderGlyphQuadItem> &qstore =
            commands.glyph_quad_storage();
        if (rgba_len < static_cast<size_t>(aw) * static_cast<size_t>(ah) * 4U) {
          return CGFX_ERROR_PLATFORM;
        }
        if (rgba_off > blob.size() || rgba_len > blob.size() - rgba_off) {
          return CGFX_ERROR_PLATFORM;
        }
        const size_t q_end = q_off + q_count;
        if (q_off > qstore.size() || q_end > qstore.size() || q_end < q_off) {
          return CGFX_ERROR_PLATFORM;
        }

        if (glyph_atlas_texture_name_ == 0U) {
          glGenTextures(1, &glyph_atlas_texture_name_);
        }

        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glDisable(GL_SCISSOR_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, glyph_atlas_texture_name_);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(aw),
                     static_cast<GLsizei>(ah), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     blob.data() + rgba_off);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0, static_cast<GLdouble>(frame_width_px_),
                static_cast<GLdouble>(frame_height_px_), 0.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        for (size_t i = 0; i < q_count; ++i) {
          const RenderGlyphQuadItem &q = qstore[q_off + i];
          if (q.dst_w_px == 0U || q.dst_h_px == 0U) {
            continue;
          }
          const float x0 = static_cast<float>(q.dst_x_px);
          const float y0 = static_cast<float>(q.dst_y_px);
          const float x1 = x0 + static_cast<float>(q.dst_w_px);
          const float y1 = y0 + static_cast<float>(q.dst_h_px);
          const float tv0 = 1.0f - q.v0;
          const float tv1 = 1.0f - q.v1;
          glColor4f(clamp_unit(q.red), clamp_unit(q.green), clamp_unit(q.blue),
                    clamp_unit(q.alpha));
          glBegin(GL_QUADS);
          glTexCoord2f(q.u0, tv0);
          glVertex2f(x0, y0);
          glTexCoord2f(q.u1, tv0);
          glVertex2f(x1, y0);
          glTexCoord2f(q.u1, tv1);
          glVertex2f(x1, y1);
          glTexCoord2f(q.u0, tv1);
          glVertex2f(x0, y1);
          glEnd();
        }

        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glPopAttrib();
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
  unsigned int glyph_atlas_texture_name_{0};
};

} // namespace

std::unique_ptr<RenderDevice> cgfx_make_opengl_render_device() {
  return std::make_unique<OpenGlRenderDevice>();
}

} // namespace cgfx

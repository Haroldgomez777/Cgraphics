#pragma once

#include <cgfx/cgfx_api.h>

#include "core/widget_tree.hpp"
#include "render/render_command_list.hpp"
#include "render/render_device.hpp"

#include <memory>

namespace cgfx {

class CgfxContext;
class CgfxWindow;

class PlatformSurface {
public:
  explicit PlatformSurface(CgfxWindow *owner) : owner_(owner) {}
  virtual ~PlatformSurface() = default;

  PlatformSurface(const PlatformSurface &) = delete;
  PlatformSurface &operator=(const PlatformSurface &) = delete;

  CgfxWindow *owner() const { return owner_; }

  virtual void teardown() noexcept = 0;
  virtual cgfx_result begin_present(uint32_t *out_w, uint32_t *out_h,
                                    float *out_dpi) = 0;
  virtual void end_present() = 0;

  virtual cgfx_result query_size_px(uint32_t *out_w, uint32_t *out_h) const = 0;
  virtual cgfx_result query_dpi_scale(float *out_scale) const = 0;

private:
  CgfxWindow *owner_{};
};

class PlatformBackend {
public:
  virtual ~PlatformBackend() = default;

  virtual void poll_events(CgfxContext *ctx) = 0;

  virtual std::unique_ptr<PlatformSurface> create_surface(
      CgfxWindow *owner, const cgfx_window_desc *desc,
      cgfx_result &out_status) = 0;
};

class CgfxWindow {
public:
  static CgfxWindow *from_opaque(cgfx_window *p) {
    return reinterpret_cast<CgfxWindow *>(p);
  }

  cgfx_window *opaque() { return reinterpret_cast<cgfx_window *>(this); }

  explicit CgfxWindow(CgfxContext *ctx);
  ~CgfxWindow();

  CgfxWindow(const CgfxWindow &) = delete;
  CgfxWindow &operator=(const CgfxWindow &) = delete;

  CgfxContext &context() const { return *ctx_; }

  cgfx_result initialize(const cgfx_window_desc *desc);
  void shutdown();

  PlatformSurface *surface() { return surface_.get(); }
  RenderDevice *render_device() { return render_device_.get(); }

  cgfx_result begin_present_pass(uint32_t *out_width_px, uint32_t *out_height_px,
                                 float *out_dpi_scale);
  cgfx_result clear_present_surface(float red, float green, float blue,
                                    float alpha);
  cgfx_result fill_rect_present_surface(int32_t x_px, int32_t y_px,
                                        uint32_t width_px, uint32_t height_px,
                                        float red, float green, float blue,
                                        float alpha);
  cgfx_result fill_rect_batch_present_surface(const void *items,
                                              size_t item_count,
                                              size_t stride_bytes);
  void end_present_pass();

  WidgetTree &widget_tree_mut() noexcept { return widget_tree_; }
  const WidgetTree &widget_tree() const noexcept { return widget_tree_; }

private:
  CgfxContext *ctx_{};
  std::unique_ptr<PlatformSurface> surface_{};
  std::unique_ptr<RenderDevice> render_device_{};
  RenderCommandList command_list_{};
  WidgetTree widget_tree_{};
  bool presenting_{false};
};

} // namespace cgfx

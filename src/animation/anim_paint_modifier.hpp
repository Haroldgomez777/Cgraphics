#pragma once

#include <cgfx/cgfx_api.h>

namespace cgfx {

/** Composed visual modifiers sampled from active animation clips for one frame. */
struct AnimPaintMod {
  bool has_translate{false};
  float translate_x_px{0.f};
  float translate_y_px{0.f};
  bool has_opacity_factor{false};
  float opacity_factor{1.f};
  bool has_fill_rgba{false};
  float fill_r{0.f};
  float fill_g{0.f};
  float fill_b{0.f};
  float fill_a{1.f};
};

/** Read-only producer consumed by `BasicWidgets::paint` (keeps widgets decoupled from
 * concrete animation engine). */
class IAnimPaintCompositor {
public:
  virtual ~IAnimPaintCompositor() = default;
  virtual bool try_get_mod(cgfx_widget_id widget_id,
                           AnimPaintMod *out_mod) const noexcept = 0;
};

} // namespace cgfx

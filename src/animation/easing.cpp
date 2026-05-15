#include "animation/easing.hpp"

#include <cgfx/cgfx_api.h>

namespace cgfx {
namespace animation_easing {

float apply(int ease_enum, float t) noexcept {
  if (!std::isfinite(t)) {
    return 0.f;
  }
  if (t <= 0.f) {
    return 0.f;
  }
  if (t >= 1.f) {
    return 1.f;
  }
  switch (ease_enum) {
  case CGFX_ANIM_EASE_LINEAR:
    return linear(t);
  case CGFX_ANIM_EASE_IN_QUAD:
    return in_quad(t);
  case CGFX_ANIM_EASE_OUT_QUAD:
    return out_quad(t);
  case CGFX_ANIM_EASE_IN_OUT_QUAD:
    return in_out_quad(t);
  case CGFX_ANIM_EASE_IN_OUT_CUBIC:
    return in_out_cubic(t);
  default:
    return linear(t);
  }
}

} // namespace animation_easing
} // namespace cgfx

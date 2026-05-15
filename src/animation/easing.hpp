#pragma once

#include <cmath>

namespace cgfx {
namespace animation_easing {

/** @param t Clamp caller to [0,1] for stable output. */
inline float linear(float t) noexcept { return t; }

inline float in_quad(float t) noexcept { return t * t; }

inline float out_quad(float t) noexcept {
  const float inv = 1.f - t;
  return 1.f - inv * inv;
}

inline float in_out_quad(float t) noexcept {
  if (t < 0.5f) {
    return 2.f * t * t;
  }
  const float u = -2.f * t + 2.f;
  return 1.f - (u * u) / 2.f;
}

inline float in_out_cubic(float t) noexcept {
  if (t < 0.5f) {
    return 4.f * t * t * t;
  }
  const float u = -2.f * t + 2.f;
  return 1.f - (u * u * u) / 2.f;
}

/** Maps CGFX_ANIM_EASE_* enum values to easing (see cgfx_api). */
float apply(int ease_enum, float t) noexcept;

} // namespace animation_easing
} // namespace cgfx

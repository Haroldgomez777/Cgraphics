#include "animation/animation_clock.hpp"

#include <cmath>

namespace cgfx {

void AnimationClock::set_manual_mode(bool enabled) noexcept {
  if (manual_mode_ == enabled) {
    return;
  }
  if (enabled) {
    /** Capture wall head so toggling is approximately seamless. */
    if (!manual_mode_) {
      bootstrap_wall_if_needed();
      manual_t_s_ = wall_base_s_;
      if (wall_have_t0_) {
        using clock = std::chrono::steady_clock;
        manual_t_s_ += std::chrono::duration<double>(clock::now() - wall_t0_).count();
      }
    }
    manual_mode_ = true;
    return;
  }
  /** Wall mode: continue from the last manual head as the new wall offset. */
  manual_mode_ = false;
  wall_base_s_ = manual_t_s_;
  wall_t0_ = std::chrono::steady_clock::now();
  wall_have_t0_ = true;
}

void AnimationClock::manual_advance_seconds(double dt) noexcept {
  if (!manual_mode_) {
    return;
  }
  if (!std::isfinite(dt)) {
    return;
  }
  manual_t_s_ += dt;
}

void AnimationClock::bootstrap_wall_if_needed() noexcept {
  if (manual_mode_) {
    return;
  }
  if (wall_have_t0_) {
    return;
  }
  wall_base_s_ = 0.;
  wall_t0_ = std::chrono::steady_clock::now();
  wall_have_t0_ = true;
}

double AnimationClock::now_seconds() const noexcept {
  if (manual_mode_) {
    return manual_t_s_;
  }
  using clock = std::chrono::steady_clock;
  if (!wall_have_t0_) {
    return wall_base_s_;
  }
  return wall_base_s_ +
         std::chrono::duration<double>(clock::now() - wall_t0_).count();
}

} // namespace cgfx

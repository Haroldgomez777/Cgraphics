#pragma once

#include <chrono>
#include <cmath>
#include <cstdint>

namespace cgfx {

/** Lightweight monotonic clock for animation integration (no Win32/X11 coupling).
 *  Wall mode reports seconds since the first `bootstrap_wall_if_needed` after last mode switch.
 *  Manual mode exposes an explicit `manual_time_seconds` authoritative head. */
class AnimationClock {
public:
  bool manual_mode() const noexcept { return manual_mode_; }

  /** Toggling to manual captures the current wall-derived time as the starting manual head. */
  void set_manual_mode(bool enabled) noexcept;

  double manual_time_seconds() const noexcept { return manual_t_s_; }
  void set_manual_time_seconds(double t) noexcept {
    if (std::isfinite(t)) {
      manual_t_s_ = t;
    }
  }

  void manual_advance_seconds(double dt) noexcept;

  /** No-op in manual mode. In wall mode, pins @c t0 once so @c now_seconds() progresses. */
  void bootstrap_wall_if_needed() noexcept;

  double now_seconds() const noexcept;

private:
  bool manual_mode_{false};
  double manual_t_s_{0.};

  bool wall_have_t0_{false};
  std::chrono::steady_clock::time_point wall_t0_{};
  double wall_base_s_{0.};
};

} // namespace cgfx

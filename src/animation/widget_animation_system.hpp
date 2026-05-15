#pragma once

#include "animation/anim_paint_modifier.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace cgfx {

class WidgetTree;

/** Per-window timeline of property clips + ease sampling. Platform- and widget-kind-agnostic:
 * modifiers are applied during basic-widget painting when a compositor pointer is supplied. */
class WidgetAnimationSystem final : public IAnimPaintCompositor {
public:
  WidgetAnimationSystem() = default;

  void purge_subtree(const WidgetTree &tree, cgfx_widget_id subtree_root_id) noexcept;

  cgfx_result start_translate(cgfx_widget_id widget, float sx, float sy, float ex,
                              float ey, float duration_seconds, int ease,
                              cgfx_animation_id *out_id) noexcept;

  cgfx_result start_opacity(cgfx_widget_id widget, float start_alpha, float end_alpha,
                            float duration_seconds, int ease,
                            cgfx_animation_id *out_id) noexcept;

  cgfx_result start_fill_rgba(cgfx_widget_id widget, float sr, float sg, float sb,
                              float sa, float er, float eg, float eb, float ea,
                              float duration_seconds, int ease,
                              cgfx_animation_id *out_id) noexcept;

  void stop(cgfx_animation_id id) noexcept;
  void stop_widget_property(cgfx_widget_id widget, int property_enum) noexcept;

  bool is_active(cgfx_animation_id id) const noexcept;

  /** Advances every clip by @p dt_seconds (already scaled by context speed if desired). */
  void advance(double dt_seconds) noexcept;

  bool try_get_mod(cgfx_widget_id widget_id,
                   AnimPaintMod *out_mod) const noexcept override;

private:
  enum class Channel : std::uint8_t {
    Translate = 0,
    Opacity = 1,
    FillRgba = 2,
  };

  struct Clip {
    cgfx_animation_id id{};
    cgfx_widget_id widget{};
    Channel channel{};
    int ease{};
    float duration{};
    /** Total integrated local time ([0,duration]); eases saturate at endpoint. */
    float local_time{};
    bool finished{};
    float sx{}, sy{}, ex{}, ey{}; /** translate OR rgba packed as xyzw */
    float sa{}, ea{};              /** opacity endpoints */
    float sr{}, sg{}, sb{}, sca{}, er{}, eg{}, eb{}, eca{}; /** fill rgba */
  };

  std::vector<Clip> clips_{};
  cgfx_animation_id next_id_{1};
  mutable std::vector<const Clip *> compose_scratch_{};
};

} // namespace cgfx

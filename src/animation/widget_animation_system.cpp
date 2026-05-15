#include "animation/widget_animation_system.hpp"

#include "animation/easing.hpp"
#include "core/widget_tree.hpp"

#include <algorithm>
#include <cmath>

namespace cgfx {
namespace {

void collect_alive_subtree_widget_ids(const WidgetTree &tree, size_t subtree_index,
                                      std::vector<uint64_t> &out) {
  const auto &nodes = tree.nodes();
  if (subtree_index >= nodes.size() || !nodes[subtree_index].alive) {
    return;
  }
  std::vector<size_t> stack;
  stack.push_back(subtree_index);
  while (!stack.empty()) {
    const size_t idx = stack.back();
    stack.pop_back();
    if (idx >= nodes.size() || !nodes[idx].alive) {
      continue;
    }
    out.push_back(nodes[idx].id);
    for (size_t child : nodes[idx].children) {
      if (child < nodes.size()) {
        stack.push_back(child);
      }
    }
  }
}

inline float lerp_f(float a, float b, float t) noexcept {
  return a + (b - a) * t;
}

} // namespace

void WidgetAnimationSystem::purge_subtree(const WidgetTree &tree,
                                          cgfx_widget_id subtree_root_id) noexcept {
  size_t ri{};
  if (tree.index_of_widget(subtree_root_id, ri) != CGFX_OK) {
    return;
  }
  std::vector<uint64_t> ids;
  collect_alive_subtree_widget_ids(tree, ri, ids);
  clips_.erase(std::remove_if(clips_.begin(), clips_.end(),
                              [&ids](const Clip &c) {
                                return std::find(ids.begin(), ids.end(), c.widget) !=
                                       ids.end();
                              }),
               clips_.end());
}

cgfx_result WidgetAnimationSystem::start_translate(
    cgfx_widget_id widget, float sx, float sy, float ex, float ey,
    float duration_seconds, int ease, cgfx_animation_id *out_id) noexcept {
  if (!out_id || !std::isfinite(duration_seconds) || duration_seconds <= 0.f) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  Clip c{};
  c.id = next_id_++;
  if (c.id == CGFX_ANIMATION_ID_NONE) {
    c.id = next_id_++;
  }
  c.widget = widget;
  c.channel = Channel::Translate;
  c.ease = ease;
  c.duration = duration_seconds;
  c.sx = sx;
  c.sy = sy;
  c.ex = ex;
  c.ey = ey;
  clips_.push_back(c);
  *out_id = c.id;
  return CGFX_OK;
}

cgfx_result WidgetAnimationSystem::start_opacity(cgfx_widget_id widget,
                                                 float start_alpha, float end_alpha,
                                                 float duration_seconds, int ease,
                                                 cgfx_animation_id *out_id) noexcept {
  if (!out_id || !std::isfinite(duration_seconds) || duration_seconds <= 0.f) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  if (!std::isfinite(start_alpha) || !std::isfinite(end_alpha)) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  Clip c{};
  c.id = next_id_++;
  if (c.id == CGFX_ANIMATION_ID_NONE) {
    c.id = next_id_++;
  }
  c.widget = widget;
  c.channel = Channel::Opacity;
  c.ease = ease;
  c.duration = duration_seconds;
  c.sa = start_alpha;
  c.ea = end_alpha;
  clips_.push_back(c);
  *out_id = c.id;
  return CGFX_OK;
}

cgfx_result WidgetAnimationSystem::start_fill_rgba(
    cgfx_widget_id widget, float sr, float sg, float sb, float sa, float er, float eg,
    float eb, float ea, float duration_seconds, int ease,
    cgfx_animation_id *out_id) noexcept {
  if (!out_id || !std::isfinite(duration_seconds) || duration_seconds <= 0.f) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  Clip c{};
  c.id = next_id_++;
  if (c.id == CGFX_ANIMATION_ID_NONE) {
    c.id = next_id_++;
  }
  c.widget = widget;
  c.channel = Channel::FillRgba;
  c.ease = ease;
  c.duration = duration_seconds;
  c.sr = sr;
  c.sg = sg;
  c.sb = sb;
  c.sca = sa;
  c.er = er;
  c.eg = eg;
  c.eb = eb;
  c.eca = ea;
  clips_.push_back(c);
  *out_id = c.id;
  return CGFX_OK;
}

void WidgetAnimationSystem::stop(cgfx_animation_id id) noexcept {
  if (id == CGFX_ANIMATION_ID_NONE) {
    return;
  }
  clips_.erase(std::remove_if(clips_.begin(), clips_.end(),
                              [id](const Clip &c) { return c.id == id; }),
               clips_.end());
}

void WidgetAnimationSystem::stop_widget_property(cgfx_widget_id widget,
                                               int property_enum) noexcept {
  const auto want = static_cast<cgfx_anim_property>(property_enum);
  Channel ch = Channel::Translate;
  switch (want) {
  case CGFX_ANIM_PROPERTY_TRANSLATE_LOGICAL_PX:
    ch = Channel::Translate;
    break;
  case CGFX_ANIM_PROPERTY_OPACITY:
    ch = Channel::Opacity;
    break;
  case CGFX_ANIM_PROPERTY_FILL_RGBA:
    ch = Channel::FillRgba;
    break;
  default:
    return;
  }
  clips_.erase(std::remove_if(clips_.begin(), clips_.end(),
                              [widget, ch](const Clip &c) {
                                return c.widget == widget && c.channel == ch;
                              }),
               clips_.end());
}

bool WidgetAnimationSystem::is_active(cgfx_animation_id id) const noexcept {
  if (id == CGFX_ANIMATION_ID_NONE) {
    return false;
  }
  for (const Clip &c : clips_) {
    if (c.id == id) {
      return true;
    }
  }
  return false;
}

void WidgetAnimationSystem::advance(double dt_seconds) noexcept {
  if (!(dt_seconds > 0.0) || !std::isfinite(dt_seconds)) {
    return;
  }
  const float dt = static_cast<float>(dt_seconds);
  for (Clip &c : clips_) {
    if (c.finished) {
      continue;
    }
    c.local_time += dt;
    if (c.local_time >= c.duration) {
      c.local_time = c.duration;
      c.finished = true;
    }
  }
}

bool WidgetAnimationSystem::try_get_mod(cgfx_widget_id widget_id,
                                        AnimPaintMod *out_mod) const noexcept {
  if (!out_mod) {
    return false;
  }

  compose_scratch_.clear();
  for (const Clip &c : clips_) {
    if (c.widget == widget_id) {
      compose_scratch_.push_back(&c);
    }
  }
  if (compose_scratch_.empty()) {
    return false;
  }
  /** Newest clip id wins per independent channel (translate / opacity / fill). */
  std::sort(compose_scratch_.begin(), compose_scratch_.end(),
            [](const Clip *a, const Clip *b) { return a->id > b->id; });

  AnimPaintMod m{};
  bool have_t = false;
  bool have_o = false;
  bool have_f = false;

  for (const Clip *pc : compose_scratch_) {
    const Clip &c = *pc;
    const float denom = c.duration > 0.f ? c.duration : 1.f;
    float u = c.local_time / denom;
    if (u < 0.f) {
      u = 0.f;
    }
    if (u > 1.f) {
      u = 1.f;
    }
    const float w = animation_easing::apply(c.ease, u);
    switch (c.channel) {
    case Channel::Translate: {
      if (!have_t) {
        m.has_translate = true;
        m.translate_x_px = lerp_f(c.sx, c.ex, w);
        m.translate_y_px = lerp_f(c.sy, c.ey, w);
        have_t = true;
      }
      break;
    }
    case Channel::Opacity: {
      if (!have_o) {
        m.has_opacity_factor = true;
        m.opacity_factor = lerp_f(c.sa, c.ea, w);
        have_o = true;
      }
      break;
    }
    case Channel::FillRgba: {
      if (!have_f) {
        m.has_fill_rgba = true;
        m.fill_r = lerp_f(c.sr, c.er, w);
        m.fill_g = lerp_f(c.sg, c.eg, w);
        m.fill_b = lerp_f(c.sb, c.eb, w);
        m.fill_a = lerp_f(c.sca, c.eca, w);
        have_f = true;
      }
      break;
    }
    }
    if (have_t && have_o && have_f) {
      break;
    }
  }

  *out_mod = m;
  return true;
}

} // namespace cgfx

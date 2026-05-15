#include "layout/flex_layout.hpp"

#include "core/widget_tree.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace cgfx {

namespace {

struct SizeF {
  float w{};
  float h{};
};

constexpr float kEps = 1.0e-4f;

static bool axis_is_row(const WidgetStyle &st) noexcept {
  return st.flex_axis == WidgetStyle::Axis::Row;
}

static cgfx_layout_rect pack_rect_round(float ax, float ay, float fw,
                                        float fh) noexcept {
  if (fw < 0.f) {
    fw = 0.f;
  }
  if (fh < 0.f) {
    fh = 0.f;
  }

  cgfx_layout_rect r{};
  r.x = static_cast<int32_t>(std::floor(ax + 0.5f));
  r.y = static_cast<int32_t>(std::floor(ay + 0.5f));
  float rw = std::floor(fw + 0.5f);
  float rh = std::floor(fh + 0.5f);
  constexpr float kMaxPix = float((uint32_t)(-1));

  rw = std::min(rw, kMaxPix);
  rh = std::min(rh, kMaxPix);

  if (rw < 1.f && fw > kEps) {
    rw = 1.f;
  }
  if (rh < 1.f && fh > kEps) {
    rh = 1.f;
  }

  r.width = static_cast<uint32_t>(rw);
  r.height = static_cast<uint32_t>(rh);

  constexpr uint32_t kSanityCeil = uint32_t(1U << 30);
  if (r.width > kSanityCeil) {
    r.width = 0U;
  }
  if (r.height > kSanityCeil) {
    r.height = 0U;
  }
  return r;
}

static void collect_alive_children(const WidgetTree &tree, size_t parent_idx,
                                   std::vector<size_t> &out) noexcept {
  out.clear();
  const WidgetNode &p = tree.nodes()[parent_idx];
  for (size_t c : p.children) {
    if (c < tree.nodes().size() && tree.nodes()[c].alive) {
      out.push_back(c);
    }
  }
}

/** Bottom-up intrinsic "preferred" size used as flex bases (Phase 4 subset). */
static SizeF intrinsic_measure(const WidgetTree &tree, size_t idx) noexcept {
  const WidgetNode &n = tree.nodes()[idx];
  if (!n.alive) {
    return {};
  }

  bool any_alive_child = false;
  for (size_t ch : n.children) {
    if (ch < tree.nodes().size() && tree.nodes()[ch].alive) {
      any_alive_child = true;
      break;
    }
  }

  if (!any_alive_child) {
    SizeF leaf{};
    if (n.style.width_kind == WidgetStyle::SizeKind::Fixed) {
      leaf.w = static_cast<float>(n.style.width_fixed_px);
    }
    if (n.style.height_kind == WidgetStyle::SizeKind::Fixed) {
      leaf.h = static_cast<float>(n.style.height_fixed_px);
    }
    return leaf;
  }

  if (axis_is_row(n.style)) {
    float sum_w = 0.f;
    float max_h = 0.f;
    for (size_t ch : n.children) {
      if (!(ch < tree.nodes().size() && tree.nodes()[ch].alive)) {
        continue;
      }
      const SizeF s = intrinsic_measure(tree, ch);
      sum_w += s.w;
      max_h = std::max(max_h, s.h);
    }
    float out_w =
        (n.style.width_kind == WidgetStyle::SizeKind::Fixed)
            ? static_cast<float>(n.style.width_fixed_px)
            : sum_w;
    float out_h =
        (n.style.height_kind == WidgetStyle::SizeKind::Fixed)
            ? static_cast<float>(n.style.height_fixed_px)
            : max_h;
    return SizeF{out_w, out_h};
  }

  float max_w = 0.f;
  float sum_h = 0.f;
  for (size_t ch : n.children) {
    if (!(ch < tree.nodes().size() && tree.nodes()[ch].alive)) {
      continue;
    }
    const SizeF s = intrinsic_measure(tree, ch);
    max_w = std::max(max_w, s.w);
    sum_h += s.h;
  }
  float out_w =
      (n.style.width_kind == WidgetStyle::SizeKind::Fixed)
          ? static_cast<float>(n.style.width_fixed_px)
          : max_w;
  float out_h =
      (n.style.height_kind == WidgetStyle::SizeKind::Fixed)
          ? static_cast<float>(n.style.height_fixed_px)
          : sum_h;
  return SizeF{out_w, out_h};
}

static bool nearly_zero(float v) noexcept { return std::fabs(v) < kEps; }

static void distribute_main_axis(float inner_main,
                                 const std::vector<float> &basis_main,
                                 const std::vector<float> &flex_grow,
                                 const std::vector<float> &flex_shrink,
                                 std::vector<float> &out_main) noexcept {
  const size_t n = basis_main.size();
  out_main.resize(n);
  for (size_t i = 0; i < n; ++i) {
    out_main[i] = basis_main[i];
  }

  float sum_basis = 0.f;
  for (float b : basis_main) {
    sum_basis += std::max(0.f, b);
  }
  const float free_space = inner_main - sum_basis;

  if (!nearly_zero(free_space) && free_space > 0.f) {
    float sum_grow = 0.f;
    for (float g : flex_grow) {
      sum_grow += std::max(0.f, g);
    }
    if (sum_grow > kEps) {
      for (size_t i = 0; i < n; ++i) {
        const float gi = std::max(0.f, flex_grow[i]);
        out_main[i] = basis_main[i] + free_space * (gi / sum_grow);
      }
      return;
    }
  }

  if (!nearly_zero(free_space) && free_space < 0.f) {
    float denom = 0.f;
    for (size_t i = 0; i < n; ++i) {
      const float weight =
          std::max(0.f, basis_main[i]) * std::max(1.0e-6f, flex_shrink[i]);
      denom += weight;
    }
    if (denom > kEps) {
      const float need = -free_space;
      for (size_t i = 0; i < n; ++i) {
        const float weight =
            std::max(0.f, basis_main[i]) * std::max(1.0e-6f, flex_shrink[i]);
        const float shave = need * (weight / denom);
        out_main[i] = std::max(0.f, basis_main[i] - shave);
      }
    }
  }
}

static void layout_recursive(WidgetTree &tree, size_t idx, float ax, float ay,
                             float avail_w, float avail_h,
                             std::vector<size_t> &reuse_buf) noexcept {
  if (idx >= tree.nodes().size() || !tree.nodes()[idx].alive) {
    return;
  }

  WidgetNode &node = tree.node_at_mut(idx);
  const float iw = std::max(0.f, avail_w);
  const float ih = std::max(0.f, avail_h);

  node.bounds = pack_rect_round(ax, ay, iw, ih);

  collect_alive_children(tree, idx, reuse_buf);
  if (reuse_buf.empty()) {
    return;
  }

  const std::vector<size_t> ordered_children(reuse_buf);

  const bool row_mode = axis_is_row(node.style);
  const float inner_cross = row_mode ? ih : iw;
  const float inner_main = row_mode ? iw : ih;

  const size_t m = ordered_children.size();
  std::vector<float> basis_main(m);
  std::vector<float> grow(m);
  std::vector<float> shrink(m);
  std::vector<SizeF> intr(m);

  for (size_t i = 0; i < m; ++i) {
    const size_t ch = ordered_children[i];
    intr[i] = intrinsic_measure(tree, ch);
    const WidgetStyle &st = tree.nodes()[ch].style;

    basis_main[i] = row_mode ? intr[i].w : intr[i].h;

    if (row_mode) {
      if (st.width_kind == WidgetStyle::SizeKind::Fixed) {
        basis_main[i] = static_cast<float>(st.width_fixed_px);
      }
    } else if (st.height_kind == WidgetStyle::SizeKind::Fixed) {
      basis_main[i] = static_cast<float>(st.height_fixed_px);
    }

    grow[i] = st.flex_grow;
    shrink[i] = std::max(st.flex_shrink, 1.0e-6f);
  }

  std::vector<float> final_main;
  distribute_main_axis(inner_main, basis_main, grow, shrink, final_main);

  float main_cursor = 0.f;

  for (size_t i = 0; i < m; ++i) {
    const size_t ch_idx = ordered_children[i];
    const float main_sz =
        std::max(0.f, i < final_main.size() ? final_main[i]
                                            : basis_main[i]);

    if (row_mode) {
      const float child_ax = ax + main_cursor;
      const float child_ay = ay;
      const float child_aw = main_sz;
      const float child_ah = inner_cross;
      layout_recursive(tree, ch_idx, child_ax, child_ay, child_aw, child_ah,
                       reuse_buf);
    } else {
      const float child_ax = ax;
      const float child_ay = ay + main_cursor;
      const float child_aw = inner_cross;
      const float child_ah = main_sz;
      layout_recursive(tree, ch_idx, child_ax, child_ay, child_aw, child_ah,
                       reuse_buf);
    }
    main_cursor += main_sz;
  }
}

} // namespace

void run_flex_layout(WidgetTree &tree, uint32_t viewport_w_u32,
                     uint32_t viewport_h_u32) noexcept {
  if (tree.nodes().empty()) {
    return;
  }

  const float vw = static_cast<float>(viewport_w_u32);
  const float vh = static_cast<float>(viewport_h_u32);

  std::vector<size_t> scratch{};
  scratch.reserve(32);

  layout_recursive(tree, /*idx=*/0, 0.f, 0.f, vw, vh, scratch);
}

} // namespace cgfx

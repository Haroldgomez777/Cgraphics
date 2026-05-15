/** Phase 5: facet registry + paint emission + visibility-hit filtering (offline). */

#include "core/widget_tree.hpp"
#include "layout/flex_layout.hpp"
#include "render/render_command_list.hpp"
#include "style/ui_theme.hpp"
#include "style/widget_style_overrides.hpp"
#include "widgets/basic_widgets.hpp"

#include <cassert>
#include <cstdint>

namespace {

struct VisibleHitPayload {
  const cgfx::BasicWidgets *bw{};
  const cgfx::WidgetTree *tree{};
};

bool visible_hit_filter_thunk(cgfx_widget_id id, void *user) noexcept {
  auto *payload = reinterpret_cast<const VisibleHitPayload *>(user);
  if (!payload || !payload->bw || !payload->tree) {
    return false;
  }
  return payload->bw->chain_visible_for_input_hit(*payload->tree, id);
}

} // namespace

int main() {
  using cgfx::BasicWidgets;
  using cgfx::RenderCommandList;
  using cgfx::RenderCommandType;
  using cgfx::WidgetTree;
  using cgfx::run_flex_layout;

  WidgetTree tree{};
  BasicWidgets widgets{};
  const cgfx_widget_id root = tree.root_id();

  cgfx_widget_id panel = CGFX_WIDGET_ID_NONE;
  assert(widgets.create_panel(tree, root, &panel) == CGFX_OK &&
         panel != CGFX_WIDGET_ID_NONE);

  cgfx_widget_id lbl = CGFX_WIDGET_ID_NONE;
  assert(widgets.create_label(tree, panel, &lbl) == CGFX_OK &&
         lbl != CGFX_WIDGET_ID_NONE);

  cgfx_widget_id btn = CGFX_WIDGET_ID_NONE;
  assert(widgets.create_button(tree, panel, &btn) == CGFX_OK &&
         btn != CGFX_WIDGET_ID_NONE);

  cgfx::BasicWidgetKind k{};
  assert(widgets.try_get_kind(panel, &k));
  assert(k == cgfx::BasicWidgetKind::Panel);
  assert(widgets.try_get_kind(lbl, &k));
  assert(k == cgfx::BasicWidgetKind::Label);
  assert(widgets.try_get_kind(btn, &k));
  assert(k == cgfx::BasicWidgetKind::Button);

  assert(widgets.set_visible(panel, true) == CGFX_OK);

  /** Layout: stacked column inside root; root fills nominal 320x240. */
  constexpr uint32_t kW = 320U;
  constexpr uint32_t kH = 240U;

  assert(tree.set_width(root, CGFX_LAYOUT_SIZE_FIXED, kW) == CGFX_OK);
  assert(tree.set_height(root, CGFX_LAYOUT_SIZE_FIXED, kH) == CGFX_OK);
  assert(tree.set_layout_axis(root, CGFX_LAYOUT_AXIS_COLUMN) == CGFX_OK);

  assert(tree.set_width(panel, CGFX_LAYOUT_SIZE_AUTO, 0) == CGFX_OK);
  assert(tree.set_height(panel, CGFX_LAYOUT_SIZE_FIXED, 120) == CGFX_OK);
  assert(tree.set_layout_axis(panel, CGFX_LAYOUT_AXIS_COLUMN) == CGFX_OK);
  assert(tree.set_flex_grow(panel, 0.f) == CGFX_OK);

  assert(tree.set_width(lbl, CGFX_LAYOUT_SIZE_AUTO, 0) == CGFX_OK);
  assert(tree.set_height(lbl, CGFX_LAYOUT_SIZE_FIXED, 32) == CGFX_OK);

  assert(tree.set_width(btn, CGFX_LAYOUT_SIZE_AUTO, 0) == CGFX_OK);
  assert(tree.set_height(btn, CGFX_LAYOUT_SIZE_FIXED, 40) == CGFX_OK);

  assert(widgets.set_utf8_text(lbl, cgfx::BasicWidgetKind::Label, "Pi", 2) ==
         CGFX_OK);
  assert(widgets.set_utf8_text(btn, cgfx::BasicWidgetKind::Button, "Ok", 2) ==
         CGFX_OK);

  size_t llen = 0U;
  assert(widgets.utf8_text_byte_length(lbl, cgfx::BasicWidgetKind::Label,
                                       &llen) == CGFX_OK);
  assert(llen == 2U);

  run_flex_layout(tree, kW, kH);

  cgfx::UiTheme theme = cgfx::UiTheme::make_phase5_builtin();
  cgfx::WidgetStyleOverrides style_over{};
  RenderCommandList cmds{};
  assert(widgets.paint(tree, cmds, theme, style_over) == CGFX_OK);

  unsigned fill_count = 0U;
  for (const auto &cmd : cmds.commands()) {
    if (cmd.type == cgfx::RenderCommandType::FillRect) {
      ++fill_count;
    }
  }
  assert(fill_count == 4U);

  cgfx_layout_rect btn_bounds{};
  assert(tree.get_bounds(btn, &btn_bounds) == CGFX_OK &&
         btn_bounds.width > 2U && btn_bounds.height > 2U);
  const int32_t tcx =
      btn_bounds.x + static_cast<int32_t>(btn_bounds.width / 2);
  const int32_t tcy =
      btn_bounds.y + static_cast<int32_t>(btn_bounds.height / 2);

  VisibleHitPayload vis_payload{&widgets, &tree};
  const cgfx_widget_id deepest = tree.hit_test_logical_filtered(
      tcx, tcy, &visible_hit_filter_thunk,
      static_cast<void *>(&vis_payload));
  assert(deepest == btn);

  assert(widgets.set_visible(panel, false) == CGFX_OK);
  /** Geometric ancestor (root, no facet) still contains the coordinate; descendants
   *  under `visible=false` facets are skipped by the predicate. */
  const cgfx_widget_id miss = tree.hit_test_logical_filtered(
      tcx, tcy, &visible_hit_filter_thunk,
      static_cast<void *>(&vis_payload));
  assert(miss == tree.root_id());

  /** Subtree visibility: nothing under the hidden panel paints (not just the panel leaf). */

  cmds.reset();

  assert(widgets.paint(tree, cmds, theme, style_over) == CGFX_OK);

  fill_count = 0U;

  for (const auto &cmd : cmds.commands()) {

    if (cmd.type == cgfx::RenderCommandType::FillRect) {

      ++fill_count;

    }

  }

  assert(fill_count == 0U);

  return 0;
}

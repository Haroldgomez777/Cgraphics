#pragma once

#include <cgfx/cgfx_api.h>

#include "core/widget_tree.hpp"
#include "render/render_command_list.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>

namespace cgfx {

class CgfxWindow;

struct WidgetClickSynthesisResult {
  bool should_emit = false;
  cgfx_event_widget_click_payload payload{};
};

enum class BasicWidgetKind : uint8_t { Panel = 1, Label = 2, Button = 3 };

struct PanelFacet {
  float bg_r = 0.18f;
  float bg_g = 0.18f;
  float bg_b = 0.2f;
  float bg_a = 1.f;
};

struct LabelFacet {
  /** UTF-8 bytestring; full text shaping/raster deferred (Phase 5 placeholder paint). */
  std::string text_utf8{};
  /** Small marker underline when text is assigned (observable sans text engine). */
  float marker_r = 0.85f;
  float marker_g = 0.85f;
  float marker_b = 0.9f;
  float marker_a = 1.f;
};

struct ButtonFacet {
  std::string caption_utf8{};
  bool enabled = true;
  float bg_normal_r = 0.35f;
  float bg_normal_g = 0.38f;
  float bg_normal_b = 0.45f;
  float bg_hover_r = 0.45f;
  float bg_hover_g = 0.5f;
  float bg_hover_b = 0.58f;
  float bg_pressed_r = 0.28f;
  float bg_pressed_g = 0.3f;
  float bg_pressed_b = 0.36f;
  float bg_disabled_r = 0.25f;
  float bg_disabled_g = 0.25f;
  float bg_disabled_b = 0.27f;
  float caption_marker_r = 0.92f;
  float caption_marker_g = 0.92f;
  float caption_marker_b = 0.96f;
  float caption_marker_a = 1.f;
};

struct BasicFacetUnion {
  BasicWidgetKind kind = BasicWidgetKind::Panel;
  bool visible = true;

  PanelFacet panel{};
  LabelFacet label{};
  ButtonFacet button{};
};

/** Panel / Label / Button state layered on retained tree (Phase 5). Core layout stays in
 *  `WidgetTree`; this registry holds widget-specific visuals + interaction bookkeeping. */
class BasicWidgets {
public:
  cgfx_result create_panel(WidgetTree &tree, cgfx_widget_id parent_id,
                           cgfx_widget_id *out_id);
  cgfx_result create_label(WidgetTree &tree, cgfx_widget_id parent_id,
                           cgfx_widget_id *out_id);
  cgfx_result create_button(WidgetTree &tree, cgfx_widget_id parent_id,
                            cgfx_widget_id *out_id);

  void purge_subtree(const WidgetTree &tree, cgfx_widget_id root_subtree_id);

  bool try_get_kind(cgfx_widget_id id, BasicWidgetKind *out_kind) const noexcept;

  /** Facet-visible chain ends at subtree roots flagged `visible=false` (hits pass through). */
  bool chain_visible_for_input_hit(const WidgetTree &tree,
                                    cgfx_widget_id id) const noexcept;

  static bool input_visibility_filterThunk(cgfx_widget_id wid,
                                           void *user_data) noexcept;

  /** Updates hover/button capture from logical pointer coords (after layout sync). */
  void on_mouse_move_logical(const WidgetTree &tree,
                             cgfx_widget_id filtered_hit_leaf_id, int32_t x,
                             int32_t y) noexcept;

  /** Updates interaction state after routing picked @p `p`; when @p `out_click` is
   *  non-null, fills a synthesized **`CGFX_EVENT_WIDGET_CLICK`** payload for the dispatcher
   *  to enqueue (keeps BasicWidgets independent of **`CgfxContext`** / queue wiring). */
  void on_mouse_button_logical(CgfxWindow *window,
                               const cgfx_event_mouse_button_payload &p,
                               WidgetClickSynthesisResult *out_click) noexcept;

  cgfx_result paint(const WidgetTree &tree, RenderCommandList &cmds);

  /** Property helpers (reject unknown / wrong kind). */
  cgfx_result set_visible(cgfx_widget_id id, bool visible) noexcept;
  cgfx_result get_visible(cgfx_widget_id id, bool *out) const noexcept;

  cgfx_result set_panel_background_rgba(cgfx_widget_id id, float r, float g,
                                         float b, float a) noexcept;

  cgfx_result set_utf8_text(cgfx_widget_id id, BasicWidgetKind expected_kind,
                            const char *utf8_bytes, size_t utf8_byte_count_or_zero);
  cgfx_result get_utf8_text(cgfx_widget_id id, BasicWidgetKind expected_kind,
                            char *out_buf, size_t out_cap_bytes,
                            size_t *out_written_including_null) const noexcept;

  cgfx_result utf8_text_byte_length(cgfx_widget_id id, BasicWidgetKind expected_kind,
                                    size_t *out_bytes_excluding_null) const noexcept;

  cgfx_result set_button_enabled(cgfx_widget_id id, bool enabled) noexcept;
  cgfx_result get_button_enabled(cgfx_widget_id id, bool *out) const noexcept;

  cgfx_widget_id hovered_button_logical() const noexcept {
    return hovered_button_logical_;
  }

  cgfx_widget_id pressed_capture_button_logical() const noexcept {
    return capture_button_logical_;
  }

private:
  using Map = std::unordered_map<uint64_t, BasicFacetUnion>;
  Map facets_{};
  cgfx_widget_id hovered_button_logical_{CGFX_WIDGET_ID_NONE};
  cgfx_widget_id capture_button_logical_{CGFX_WIDGET_ID_NONE};
  int32_t last_pointer_x_{0};
  int32_t last_pointer_y_{0};
  /** True while left capture started on enabled button surface. */
  bool left_pressed_on_capture_{false};

  cgfx_result allocate_typed_leaf_(WidgetTree &tree, cgfx_widget_id parent_id,
                                    BasicFacetUnion facet,
                                    cgfx_widget_id *out_id);

  cgfx_widget_id resolve_button_logical_from_leaf_hit(
      const WidgetTree &tree, cgfx_widget_id filtered_hit_leaf) const noexcept;
};

} // namespace cgfx

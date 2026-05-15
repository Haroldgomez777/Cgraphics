#ifndef CGFX_API_H
#define CGFX_API_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#  ifdef cgfx_EXPORTS
#    define CGFX_API __declspec(dllexport)
#  else
#    define CGFX_API __declspec(dllimport)
#  endif
#else
#  define CGFX_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cgfx_context cgfx_context;
typedef struct cgfx_window cgfx_window;

typedef enum cgfx_result {
  CGFX_OK = 0,
  CGFX_ERROR_INVALID_ARGUMENT = 1,
  CGFX_ERROR_OUT_OF_MEMORY = 2,
  CGFX_ERROR_PLATFORM = 3,
  CGFX_ERROR_UNSUPPORTED = 4,
} cgfx_result;

typedef struct cgfx_window_desc {
  const char *title;
  uint32_t width;
  uint32_t height;
} cgfx_window_desc;

typedef enum cgfx_event_type {
  CGFX_EVENT_CLOSE_REQUEST = 1,
  CGFX_EVENT_RESIZE = 2,
  CGFX_EVENT_MOUSE_MOVE = 3,
  CGFX_EVENT_MOUSE_BUTTON = 4,
  CGFX_EVENT_KEY = 5,
  /** Phase 5: synthesized activation for retained `BUTTON` facets (polling model). */
  CGFX_EVENT_WIDGET_CLICK = 6,
} cgfx_event_type;

/** Phase 5 basic widget discriminators (panels/labels/buttons layered on retained tree).
 *  Returned as `CGFX_BASIC_WIDGET_KIND_NONE` for structural-only widgets. */
typedef enum cgfx_basic_widget_kind {
  CGFX_BASIC_WIDGET_KIND_NONE = 0,
  CGFX_BASIC_WIDGET_KIND_PANEL = 1,
  CGFX_BASIC_WIDGET_KIND_LABEL = 2,
  CGFX_BASIC_WIDGET_KIND_BUTTON = 3,
} cgfx_basic_widget_kind;

typedef enum cgfx_mouse_button {
  CGFX_MOUSE_LEFT = 0,
  CGFX_MOUSE_RIGHT = 1,
  CGFX_MOUSE_MIDDLE = 2,
  CGFX_MOUSE_X1 = 3,
  CGFX_MOUSE_X2 = 4,
} cgfx_mouse_button;

typedef enum cgfx_input_action {
  CGFX_PRESS = 0,
  CGFX_RELEASE = 1,
  CGFX_REPEAT = 2,
} cgfx_input_action;

typedef enum cgfx_key {
  CGFX_KEY_UNKNOWN = 0,
  CGFX_KEY_ESCAPE = 1,
  CGFX_KEY_SPACE = 2,
  CGFX_KEY_ENTER = 3,
  CGFX_KEY_TAB = 4,
  CGFX_KEY_BACKSPACE = 5,
  CGFX_KEY_DELETE = 6,
  CGFX_KEY_LEFT = 7,
  CGFX_KEY_RIGHT = 8,
  CGFX_KEY_UP = 9,
  CGFX_KEY_DOWN = 10,
  CGFX_KEY_LEFT_SHIFT = 11,
  CGFX_KEY_RIGHT_SHIFT = 12,
  CGFX_KEY_LEFT_CONTROL = 13,
  CGFX_KEY_RIGHT_CONTROL = 14,
  CGFX_KEY_LEFT_ALT = 15,
  CGFX_KEY_RIGHT_ALT = 16,
  CGFX_KEY_HOME = 17,
  CGFX_KEY_END = 18,
  CGFX_KEY_PAGE_UP = 19,
  CGFX_KEY_PAGE_DOWN = 20,
  CGFX_KEY_F1 = 101,
  CGFX_KEY_F2 = 102,
  CGFX_KEY_F3 = 103,
  CGFX_KEY_F4 = 104,
  CGFX_KEY_F5 = 105,
  CGFX_KEY_F6 = 106,
  CGFX_KEY_F7 = 107,
  CGFX_KEY_F8 = 108,
  CGFX_KEY_F9 = 109,
  CGFX_KEY_F10 = 110,
  CGFX_KEY_F11 = 111,
  CGFX_KEY_F12 = 112,
  CGFX_KEY_A = 300,
  CGFX_KEY_B,
  CGFX_KEY_C,
  CGFX_KEY_D,
  CGFX_KEY_E,
  CGFX_KEY_F,
  CGFX_KEY_G,
  CGFX_KEY_H,
  CGFX_KEY_I,
  CGFX_KEY_J,
  CGFX_KEY_K,
  CGFX_KEY_L,
  CGFX_KEY_M,
  CGFX_KEY_N,
  CGFX_KEY_O,
  CGFX_KEY_P,
  CGFX_KEY_Q,
  CGFX_KEY_R,
  CGFX_KEY_S,
  CGFX_KEY_T,
  CGFX_KEY_U,
  CGFX_KEY_V,
  CGFX_KEY_W,
  CGFX_KEY_X,
  CGFX_KEY_Y,
  CGFX_KEY_Z,
} cgfx_key;

typedef uint64_t cgfx_widget_id;

#ifndef CGFX_WIDGET_ID_NONE
#  define CGFX_WIDGET_ID_NONE UINT64_C(0)
#endif

typedef struct cgfx_event_widget_click_payload {
  cgfx_widget_id widget_id;
  cgfx_mouse_button button;
  int32_t x;
  int32_t y;
} cgfx_event_widget_click_payload;

/** How pointer / keyboard logical events are expanded into the per-window event queue
 *  (see `target_widget` / `routed_widget` on relevant payloads). */
typedef enum cgfx_input_propagation_policy {
  /** One queue entry per platform input: only the deepest hit / focus widget. */
  CGFX_INPUT_PROPAGATION_TARGET_ONLY = 0,
  /** One queue entry per ancestor on the parent chain, inner-to-outer (target, parent,
   *  …, root). Missed hits (`CGFX_WIDGET_ID_NONE`) still produce a single entry. */
  CGFX_INPUT_PROPAGATION_BUBBLE_TO_PARENT = 1,
} cgfx_input_propagation_policy;

typedef struct cgfx_event_resize_payload {
  uint32_t width;
  uint32_t height;
} cgfx_event_resize_payload;

typedef struct cgfx_event_mouse_move_payload {
  int32_t x;
  int32_t y;
  /** Phase 4.1: logical hit target; propagation is target-only (no bubble). */
  cgfx_widget_id target_widget;
} cgfx_event_mouse_move_payload;

typedef struct cgfx_event_mouse_button_payload {
  cgfx_mouse_button button;
  cgfx_input_action action;
  int32_t x;
  int32_t y;
  /** Deepest logical hit (pointer origin). Unchanged from Phase 4.1. */
  cgfx_widget_id target_widget;
  /** Widget this queue entry is delivered to: equals `target_widget` in
   *  `CGFX_INPUT_PROPAGATION_TARGET_ONLY`; walks ancestors when bubbling. */
  cgfx_widget_id routed_widget;
} cgfx_event_mouse_button_payload;

typedef struct cgfx_event_key_payload {
  cgfx_key key;
  uint32_t native_code;
  cgfx_input_action action;
  int repeat;
  /** Focus widget (keyboard origin). Unchanged from Phase 4.1. */
  cgfx_widget_id target_widget;
  /** Same semantics as `routed_widget` on mouse button payloads. */
  cgfx_widget_id routed_widget;
} cgfx_event_key_payload;

typedef struct cgfx_event_close_payload {
  int unused;
} cgfx_event_close_payload;

typedef struct cgfx_event {
  cgfx_event_type type;
  cgfx_window *window;
  union {
    cgfx_event_close_payload close;
    cgfx_event_resize_payload resize;
    cgfx_event_mouse_move_payload mouse_move;
    cgfx_event_mouse_button_payload mouse_button;
    cgfx_event_key_payload key;
    cgfx_event_widget_click_payload widget_click;
  } payload;
  /** Per-context enqueue order assigned when the event is admitted to the
   *  queue (starts at 1). Use together with dequeue order for correlation;
   *  `priority_front` retries preserve an existing non-zero sequence. */
  uint64_t sequence;
} cgfx_event;

CGFX_API cgfx_result cgfx_context_create(cgfx_context **out_context);
CGFX_API void cgfx_context_destroy(cgfx_context *context);

/** Default propagation for windows created afterward (existing windows unchanged). */
CGFX_API void cgfx_context_set_default_input_propagation_policy(
    cgfx_context *context, cgfx_input_propagation_policy policy);
CGFX_API cgfx_input_propagation_policy
cgfx_context_get_default_input_propagation_policy(const cgfx_context *context);

CGFX_API cgfx_result cgfx_window_create(cgfx_context *context,
                                         const cgfx_window_desc *desc,
                                         cgfx_window **out_window);
CGFX_API void cgfx_window_destroy(cgfx_window *window);

/** Per-window pointer/keyboard queue expansion policy (defaults from context at
 *  `cgfx_window_create`). */
CGFX_API void cgfx_window_set_input_propagation_policy(
    cgfx_window *window, cgfx_input_propagation_policy policy);
CGFX_API cgfx_input_propagation_policy
cgfx_window_get_input_propagation_policy(const cgfx_window *window);

CGFX_API cgfx_result cgfx_poll_events(cgfx_context *context);

/** When true, each propagated pointer/button or key enqueue prints one stderr
 * line (default false — no overhead otherwise). Correlates with `cgfx_event.sequence`. */
CGFX_API void cgfx_context_set_input_routing_trace_enabled(cgfx_context *context,
                                                              bool enabled);
CGFX_API bool cgfx_context_get_input_routing_trace_enabled(
    const cgfx_context *context);

CGFX_API bool cgfx_next_event(cgfx_context *context, cgfx_event_type *type,
                               cgfx_window **window_handle, void *event_payload,
                               size_t payload_capacity, size_t *out_payload_used);

/** Like `cgfx_next_event`; when @p out_sequence is non-NULL and dequeue
 * succeeds, writes the admitted queue sequence (`cgfx_event.sequence`). */
CGFX_API bool cgfx_next_event_with_sequence(cgfx_context *context,
                                            cgfx_event_type *type,
                                            cgfx_window **window_handle,
                                            void *event_payload,
                                            size_t payload_capacity,
                                            size_t *out_payload_used,
                                            uint64_t *out_sequence);

/** Convenience dequeue that fills `cgfx_event`. */
CGFX_API bool cgfx_next_event_into(cgfx_context *context,
                                    cgfx_event *out_event);

/** Byte size for the canonical payload blob for @p type (0 if unknown). */
CGFX_API size_t cgfx_event_payload_byte_size(cgfx_event_type type);

typedef enum cgfx_event_queue_overflow_policy {
  /** Incoming events are discarded while the queue is at capacity */
  CGFX_EVENT_QUEUE_OVERFLOW_DROP_NEWEST = 0,
  /** Oldest events are discarded to make space for newcomers */
  CGFX_EVENT_QUEUE_OVERFLOW_DROP_OLDEST = 1,
} cgfx_event_queue_overflow_policy;

/** @param max_pending_events Use 0 for the compile-time default (~4k slots). */
CGFX_API cgfx_result cgfx_context_set_event_queue_limits(
    cgfx_context *context, size_t max_pending_events,
    cgfx_event_queue_overflow_policy overflow_policy);

CGFX_API cgfx_result cgfx_context_get_event_queue_limits(
    const cgfx_context *context, size_t *out_max_pending,
    cgfx_event_queue_overflow_policy *out_overflow_policy);

/** When enabled, successive resize payloads for the same window coalesce into one slot. */
CGFX_API void cgfx_context_set_event_resize_coalesce(cgfx_context *context,
                                                       bool enabled);
CGFX_API bool cgfx_context_get_event_resize_coalesce(const cgfx_context *context);

/** Monotonic drops since last reset (lost due to overflow or drop-newest). */
CGFX_API uint64_t cgfx_context_event_queue_drop_count(const cgfx_context *context);
CGFX_API void cgfx_context_event_queue_reset_drop_count(cgfx_context *context);

CGFX_API cgfx_result cgfx_window_begin_present_pass(cgfx_window *window,
                                                    uint32_t *out_width_px,
                                                    uint32_t *out_height_px,
                                                    float *out_dpi_scale);
CGFX_API void cgfx_window_end_present_pass(cgfx_window *window);

CGFX_API cgfx_result cgfx_surface_clear_normalized_rgba(cgfx_window *window, float r,
                                                      float g, float b, float a);
CGFX_API cgfx_result cgfx_surface_fill_rect_pixels(cgfx_window *window,
                                                   int32_t x_px, int32_t y_px,
                                                   uint32_t width_px,
                                                   uint32_t height_px, float r,
                                                   float g, float b, float a);

typedef struct cgfx_surface_fill_rect_item {
  int32_t x_px;
  int32_t y_px;
  uint32_t width_px;
  uint32_t height_px;
  float r;
  float g;
  float b;
  float a;
} cgfx_surface_fill_rect_item;

/** Filled rects in framebuffer pixel coordinates; colors are normalized RGBA.
 *  Items are concatenated into a single backend batch for the frame pass.
 *
 *   @param items Pointer to packed rows unless @p stride_bytes is larger than
 *                `sizeof(cgfx_surface_fill_rect_item)`; aligned for the field types.
 *
 *   @param stride_bytes Byte distance between successive row starts (item 0, then
 *                       item 1, ...). Pass `0` for `sizeof(cgfx_surface_fill_rect_item)`.
 *                       Must be at least that size when non-zero (and must satisfy
 *                       `(item_count-1)*stride + sizeof(record)` stays representable).
 */
CGFX_API cgfx_result cgfx_surface_fill_rect_batch_pixels(
    cgfx_window *window, const void *items, size_t item_count,
    size_t stride_bytes);
CGFX_API cgfx_result cgfx_window_get_size_pixels(const cgfx_window *window,
                                                 uint32_t *out_width,
                                                 uint32_t *out_height);
CGFX_API cgfx_result cgfx_window_get_dpi_scale(const cgfx_window *window,
                                               float *out_scale);

/* ---- Phase 4: retained widget tree + flex layout foundation ---- */

typedef struct cgfx_layout_rect {
  int32_t x;
  int32_t y;
  uint32_t width;
  uint32_t height;
} cgfx_layout_rect;

typedef enum cgfx_layout_axis {
  CGFX_LAYOUT_AXIS_ROW = 0,
  CGFX_LAYOUT_AXIS_COLUMN = 1,
} cgfx_layout_axis;

typedef enum cgfx_layout_size_kind {
  CGFX_LAYOUT_SIZE_AUTO = 0,
  CGFX_LAYOUT_SIZE_FIXED = 1,
} cgfx_layout_size_kind;

CGFX_API cgfx_widget_id cgfx_window_widget_root(const cgfx_window *window);

CGFX_API cgfx_result cgfx_widget_create_child(cgfx_window *window,
                                              cgfx_widget_id parent_id,
                                              cgfx_widget_id *out_child_id);

CGFX_API cgfx_result cgfx_widget_destroy(cgfx_window *window,
                                         cgfx_widget_id widget_id);

CGFX_API cgfx_result cgfx_widget_reparent(cgfx_window *window,
                                          cgfx_widget_id widget_id,
                                          cgfx_widget_id new_parent_id);

CGFX_API cgfx_result cgfx_widget_set_layout_axis(cgfx_window *window,
                                                 cgfx_widget_id widget_id,
                                                 cgfx_layout_axis axis);

CGFX_API cgfx_result cgfx_widget_set_width(cgfx_window *window,
                                         cgfx_widget_id widget_id,
                                         cgfx_layout_size_kind kind,
                                         uint32_t fixed_px);

CGFX_API cgfx_result cgfx_widget_set_height(cgfx_window *window,
                                          cgfx_widget_id widget_id,
                                          cgfx_layout_size_kind kind,
                                          uint32_t fixed_px);

CGFX_API cgfx_result cgfx_widget_set_flex_grow(cgfx_window *window,
                                             cgfx_widget_id widget_id,
                                             float flex_grow);

CGFX_API cgfx_result cgfx_widget_set_flex_shrink(cgfx_window *window,
                                                 cgfx_widget_id widget_id,
                                                 float flex_shrink);

CGFX_API cgfx_result cgfx_widget_bounds_logical_px(const cgfx_window *window,
                                                   cgfx_widget_id widget_id,
                                                   cgfx_layout_rect *out_bounds);

/* ---- Phase 4.1: hit-testing + input routing ---- */

/** Hit-test using the same flex layout snapshot as input routing (refreshes flex from
 * `PlatformSurface::query_size_px`; requires a mutable window pointer because layout caches update). */
CGFX_API cgfx_result cgfx_window_hit_test_logical_px(cgfx_window *window,
                                                      int32_t x, int32_t y,
                                                      cgfx_widget_id *out_target);

/** Current keyboard focus widget (validated each access; stale ids fall back to root). */
CGFX_API cgfx_widget_id cgfx_window_focus_widget(const cgfx_window *window);

/** Direct focus assignment; rejects unknown or destroyed widget ids (CGFX_WIDGET_ID_NONE → root focus). */
CGFX_API cgfx_result cgfx_window_set_focus_widget(cgfx_window *window,
                                                  cgfx_widget_id widget_id);

/* ---- Phase 5: Panel / Label / Button basics (facet registry on retained tree) ---- */

CGFX_API cgfx_result cgfx_basic_widget_kind_of(cgfx_window *window,
                                               cgfx_widget_id widget_id,
                                               cgfx_basic_widget_kind *out_kind);

CGFX_API cgfx_result cgfx_basic_widget_panel_create(cgfx_window *window,
                                                  cgfx_widget_id parent_id,
                                                  cgfx_widget_id *out_id);

CGFX_API cgfx_result cgfx_basic_widget_label_create(cgfx_window *window,
                                                    cgfx_widget_id parent_id,
                                                    cgfx_widget_id *out_id);

CGFX_API cgfx_result cgfx_basic_widget_button_create(cgfx_window *window,
                                                     cgfx_widget_id parent_id,
                                                     cgfx_widget_id *out_id);

/** Record default visuals for facets into the Phase 2 command list — call each frame inside
 *  `cgfx_window_begin_present_pass` / `cgfx_window_end_present_pass`.
 *
 * Labels render UTF-8 as a clipped placeholder underscore today; wire real shaping next in
 *  the text subsystem (Phase 6/7 roadmap). */
CGFX_API cgfx_result cgfx_window_draw_basic_widgets(cgfx_window *window);

CGFX_API cgfx_result cgfx_basic_widget_set_visible(cgfx_window *window,
                                                   cgfx_widget_id widget_id,
                                                   int visible);

CGFX_API cgfx_result cgfx_basic_widget_get_visible(cgfx_window *window,
                                                   cgfx_widget_id widget_id,
                                                   int *out_visible);

CGFX_API cgfx_result cgfx_basic_widget_panel_set_background_rgba_normalized(
    cgfx_window *window, cgfx_widget_id panel_id, float r, float g, float b, float a);

CGFX_API cgfx_result cgfx_basic_widget_button_set_enabled(cgfx_window *window,
                                                          cgfx_widget_id button_id,
                                                          int enabled);

CGFX_API cgfx_result cgfx_basic_widget_button_get_enabled(cgfx_window *window,
                                                          cgfx_widget_id button_id,
                                                          int *out_enabled);

CGFX_API cgfx_result cgfx_basic_widget_utf8_text_set(cgfx_window *window,
                                                    cgfx_widget_id widget_id,
                                                    const char *utf8_bytes,
                                                    size_t utf8_byte_length);

CGFX_API cgfx_result cgfx_basic_widget_utf8_text_get_length(
    cgfx_window *window, cgfx_widget_id widget_id, size_t *out_byte_length);

CGFX_API cgfx_result cgfx_basic_widget_utf8_text_get(cgfx_window *window,
                                                    cgfx_widget_id widget_id,
                                                    char *out_bytes,
                                                    size_t out_capacity,
                                                    size_t *out_written_including_null);

#ifdef __cplusplus
}
#endif

#endif

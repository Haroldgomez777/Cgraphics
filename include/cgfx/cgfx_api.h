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
} cgfx_event_type;

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

typedef struct cgfx_event_resize_payload {
  uint32_t width;
  uint32_t height;
} cgfx_event_resize_payload;

typedef struct cgfx_event_mouse_move_payload {
  int32_t x;
  int32_t y;
} cgfx_event_mouse_move_payload;

typedef struct cgfx_event_mouse_button_payload {
  cgfx_mouse_button button;
  cgfx_input_action action;
  int32_t x;
  int32_t y;
} cgfx_event_mouse_button_payload;

typedef struct cgfx_event_key_payload {
  cgfx_key key;
  uint32_t native_code;
  cgfx_input_action action;
  int repeat;
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
  } payload;
} cgfx_event;

CGFX_API cgfx_result cgfx_context_create(cgfx_context **out_context);
CGFX_API void cgfx_context_destroy(cgfx_context *context);

CGFX_API cgfx_result cgfx_window_create(cgfx_context *context,
                                         const cgfx_window_desc *desc,
                                         cgfx_window **out_window);
CGFX_API void cgfx_window_destroy(cgfx_window *window);

CGFX_API cgfx_result cgfx_poll_events(cgfx_context *context);
CGFX_API bool cgfx_next_event(cgfx_context *context, cgfx_event_type *type,
                               cgfx_window **window_handle, void *event_payload,
                               size_t payload_capacity, size_t *out_payload_used);

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

#ifdef __cplusplus
}
#endif

#endif

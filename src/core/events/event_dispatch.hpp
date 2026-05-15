#pragma once

#include <cgfx/cgfx_api.h>

namespace cgfx {

class CgfxContext;
class CgfxWindow;

/** Central enqueue entry points shared by Win32/X11 (normalizes push path & queue policy). */
void event_dispatch_close(CgfxContext &ctx, CgfxWindow *window);

void event_dispatch_resize(CgfxContext &ctx, CgfxWindow *window,
                           uint32_t width_px, uint32_t height_px);

void event_dispatch_mouse_move(CgfxContext &ctx, CgfxWindow *window, int32_t x,
                               int32_t y);

void event_dispatch_mouse_button(CgfxContext &ctx, CgfxWindow *window,
                                 cgfx_mouse_button button,
                                 cgfx_input_action action, int32_t x, int32_t y);

void event_dispatch_key(CgfxContext &ctx, CgfxWindow *window, cgfx_key key,
                        uint32_t native_code, cgfx_input_action action,
                        int repeat);

} // namespace cgfx

# cgfx (Phase 2 → Phase 4 layout)

`cgfx` is a cross-platform GUI graphics framework in C/C++.

Phase 1 includes:
- Platform window creation (`Win32`, `X11`)
- OpenGL context bootstrap (`WGL`, `GLX`)
- Event polling and normalized input events
- Present pass with double buffering
- Minimal C API and smoke test

Phase 2 adds incremental render-command recording for the present pass (clear, filled rectangles including a batched path).

Phase 3 hardens **event ingestion and delivery**:

- Typed internal representation (`InternalEvent`) with centralized **dispatch enqueue** helpers used by Win32/X11 backends (no duplicated queue wiring).
- `EventQueue` with explicit **ordering (FIFO)**; **capacity** (~4 096 pending slots by default, configurable); **resize coalescing** for the same window (successive resize storms collapse to the latest payload in one queue slot).
- Overflow policy: **`CGFX_EVENT_QUEUE_OVERFLOW_DROP_OLDEST`** (default) frees the stalest event before accepting a newcomer, or **`CGFX_EVENT_QUEUE_OVERFLOW_DROP_NEWEST`** to keep the backlog and reject bursts; both increment **`cgfx_context_event_queue_drop_count`** for observability.
- The undersized dequeue retry path (**`cgfx_next_event`**) re-queues via **`push_priority_front`**, which now **respects `max_depth` by dropping the youngest tail slots** instead of permitting unbounded deque growth.

### Phase 3: using events from C

Backward compatible **`cgfx_poll_events`** + **`cgfx_next_event`** behave as before (`event_payload_bytes` sizing is described by **`cgfx_event_payload_byte_size(type)`**).

New helper **`cgfx_next_event_into`** returns a **`cgfx_event`** struct (typed union keyed by **`type`**), avoiding manual `memcpy` layout handling when you control the codebase.

Tune the queue early (before heavy input) if needed:

```c
cgfx_context_set_event_queue_limits(ctx,
    256 /* 0 restores default */,
    CGFX_EVENT_QUEUE_OVERFLOW_DROP_OLDEST);
cgfx_context_set_event_resize_coalesce(ctx, true);
```

**`cgfx_next_event`** still re-queues undelivered events at the head when the caller buffer is too small (parity with Phase 1).

### Phase 3: tests

- `cgfx_minimal_shutdown` — library smoke load.
- **`cgfx_event_queue_test`** — ordering, resize coalesce, DROP_OLDEST / DROP_NEWEST, bounded priority re-queue coverage, **enqueue sequence ids** (no window or GPU required).

### Phase 4: widget tree + flex layout baseline

Production-oriented **retained-mode tree**, **deterministic Phase 4 flex layout**, and **per-frame synchronization** wired before render recording:

| Layer | Responsibility |
| --- | --- |
| **`src/core/widget_tree`** | Stable `cgfx_widget_id` handles (`uint64_t`), persistent parent/sibling ordering, subtree destroy semantics, validated reparent guards (no ancestor/descendant cycles). |
| **`src/layout/flex_layout`** | Row/column directional containers over logical pixel sizes, **`AUTO`** vs **`FIXED`** sizing knobs, proportional **grow/shrink**, absolute bounds written as **`cgfx_layout_rect`**. |

Layout runs immediately after the platform **`begin_present`** surface dimensions are sampled and **before** any render commands enqueue for that pass (Phase 4 currently maps logical pixels directly to framebuffer pixels; further DPI mapping of hit coordinates is a follow-on item).

Minimal C bindings (additive to earlier phases):

- **Lookup root:** `cgfx_window_widget_root`
- **Structural ops:** `cgfx_widget_create_child`, `cgfx_widget_destroy`, `cgfx_widget_reparent`
- **Sizing & flex knobs:** `cgfx_widget_set_width|height`, `cgfx_widget_set_layout_axis`, `cgfx_widget_set_flex_grow|shrink`
- **Per-frame rects:** `cgfx_widget_bounds_logical_px`

Example sketch (pseudo-C):

```c
cgfx_widget_id root = cgfx_window_widget_root(win);

cgfx_widget_id stripe = 0;
cgfx_widget_create_child(win, root, &stripe);
cgfx_widget_set_layout_axis(win, stripe, CGFX_LAYOUT_AXIS_ROW);

cgfx_widget_id left_panel = 0;
cgfx_widget_create_child(win, stripe, &left_panel);
cgfx_widget_set_width(win, left_panel, CGFX_LAYOUT_SIZE_FIXED, 240);
cgfx_widget_set_height(win, left_panel, CGFX_LAYOUT_SIZE_AUTO, 0);

cgfx_widget_id filler = 0;
cgfx_widget_create_child(win, stripe, &filler);
cgfx_widget_set_width(win, filler, CGFX_LAYOUT_SIZE_AUTO, 0);
cgfx_widget_set_flex_grow(win, filler, 1.0f);

/* After cgfx_window_begin_present_pass(...) each frame ... */
cgfx_layout_rect filler_bounds;
if (cgfx_widget_bounds_logical_px(win, filler, &filler_bounds) == CGFX_OK) {
    /* feeder for future painters / clipping */
}
```

### Phase 4: tests

- **`cgfx_widget_layout_test`** — tree lifecycle + grow/shrink flex scenarios without booting GPU surfaces.

### Phase 4.1: hit-testing + input routing (target-only)

**Stacking / z-order:** among siblings, the widget **last** in the parent’s `children` order is treated as **on top** when rectangles overlap (“last appended wins”), matching intuition for painters that draw siblings in sequential order.

**Hit-test:** Deepest descendant whose bounds contain the point `(x,y)` in **logical client pixels**. Half-open rectangles: `[x,x+width) × [y,y+height)`. Zero-area bounds never hit. **`CGFX_WIDGET_ID_NONE`** is returned only when nothing contains the coordinate (typically outside the laid-out root).

**Layout snapshot:** Routed events reuse the Phase 4 flex pass driven by **`PlatformSurface::query_size_px`** (same path as **`cgfx_window_hit_test_logical_px`**), independent of backends.

**Propagation:** Default remains **target-only** (backward compatible): one **`CGFX_EVENT_MOUSE_BUTTON`** / **`CGFX_EVENT_KEY`** dequeue per logical input with **`target_widget`** as the hit / focus leaf. Optional **`CGFX_INPUT_PROPAGATION_BUBBLE_TO_PARENT`** clones each pointer or keyboard event along the deterministic parent chain **inner-first** (`leaf → parent → … → root`; missed hits **`CGFX_WIDGET_ID_NONE`** still collapse to **one** slot). **`routed_widget`** on button/key payloads identifies the ancestor stop for **that** queue entry while **`target_widget`** stays the origin leaf (parity: both match in target-only mode). **`CGFX_EVENT_MOUSE_MOVE`** stays target-only regardless of policy.

Configure per window, or prime the factory default before **`cgfx_window_create`**:

```c
cgfx_context_set_default_input_propagation_policy(
    ctx, CGFX_INPUT_PROPAGATION_TARGET_ONLY); /* omitted = default anyway */
cgfx_window_set_input_propagation_policy(win,
    CGFX_INPUT_PROPAGATION_BUBBLE_TO_PARENT);
```

**Keyboard focus:** Default focus is the **root** widget. **`cgfx_window_set_focus_widget`** updates focus (**`CGFX_WIDGET_ID_NONE` resets to root**). A **primary mouse press** (**`CGFX_PRESS`**) on the hit target adopts focus automatically. **`cgfx_window_focus_widget`** always returns an alive widget (falling back to root after destroys). Routed **`CGFX_EVENT_KEY`** payloads set **`target_widget`** to the focused widget (the bubble origin when that policy is enabled).

### Phase 4.1: C API & event fields

- **`cgfx_window_hit_test_logical_px(win, x, y, &id)`** — refresh layout from surface size, then pick topmost logical hit.
- **`cgfx_window_focus_widget` / `cgfx_window_set_focus_widget`**
- Pointer / key payload fields **`target_widget`** (origin) and **`routed_widget`** (current delivery stop) on **`cgfx_event_mouse_button_payload`**, **`cgfx_event_key_payload`** — plus **`target_widget`** on **`cgfx_event_mouse_move_payload`**.
- **`cgfx_context_[get|set]_default_input_propagation_policy`**, **`cgfx_window_[get|set]_input_propagation_policy`**

```c
cgfx_widget_id id = CGFX_WIDGET_ID_NONE;
if (cgfx_window_hit_test_logical_px(win, mx, my, &id) == CGFX_OK &&
    id != CGFX_WIDGET_ID_NONE) {
  /* point hit */
}

cgfx_window_set_focus_widget(win, some_widget);

cgfx_event ev;
while (cgfx_next_event_into(ctx, &ev)) {
  if (ev.type == CGFX_EVENT_KEY) {
    cgfx_widget_id leaf = ev.payload.key.target_widget;
    cgfx_widget_id routed_stop = ev.payload.key.routed_widget;
    (void)leaf;
    (void)routed_stop;
    (void)ev.sequence; /* queue admission id; compare with stderr when tracing */
  }
}
```

### Phase 4.1: tests

- **`cgfx_widget_input_routing_test`** — hit depth, overlapping siblings + reparent z-order, focus validation helper.
- **`cgfx_input_route_receivers_test`** — deterministic bubble expansion vs target-only and focus-at-root analogue.
- **`cgfx_event_trace_toggle_api_test`** — routing trace flag defaults off and round-trips through the C API.

### Phase 4.1: event sequence & routed-stream diagnostics

Each time an event is **admitted** to the per-context queue it receives a **`uint64_t` sequence** counter (starting at **1**). **`cgfx_next_event_into`** and **`cgfx_next_event_with_sequence`** expose it; use it to line up **expanded bubble entries** (same physical input, different `routed_widget`) with tooling or logs.

```c
cgfx_context_set_input_routing_trace_enabled(ctx, true);
/* stderr: cgfx-route … seq=… tgt=… rtd=… pol=… — one line per routed queue slot */
```

Tracing is **off by default** (`fprintf` is skipped entirely when disabled). **`cgfx_next_event_with_sequence`** mirrors **`cgfx_next_event`** but optionally returns the same id as **`cgfx_event.sequence`**.

**Note:** sequence reflects **enqueue order**, not necessarily **dequeue order** if the undersized-buffer path re-queues via **`push_priority_front`** (existing sequence is preserved on retry; a fresh priority insert gets a new higher id).

## Prerequisites

### Windows
- CMake 3.20+
- Ninja
- One supported toolchain:
  - MinGW-w64 (`gcc`/`g++`) **or**
  - MSVC (Visual Studio Build Tools)

### Linux
- CMake 3.20+
- C++ toolchain (`gcc`/`g++` or `clang`)
- Ninja or Make
- X11 + OpenGL development packages (distro-specific)

## Build

### Recommended: use presets

```powershell
cmake --preset windows-mingw-debug
cmake --build --preset build-windows-mingw-debug
```

Then run tests:

```powershell
ctest --preset test-windows-mingw-debug
```

### Windows (Ninja + MinGW-w64)

```powershell
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build build
```

### Windows (MSVC)

```powershell
cmake -S . -B build
cmake --build build --config Release
```

### Linux (Ninja)

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Linux (Make)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run Example

After building, run the demo executable:

- Preset output (recommended): Windows `build/windows-mingw-debug/cgfx_demo.exe`, Linux `build/linux-gcc-debug/cgfx_demo`
- Plain `cmake -S . -B build` layouts: Windows `build/cgfx_demo.exe`, Linux `build/cgfx_demo`

## Phase 2.3: Batch filled rectangles (`cgfx_surface_fill_rect_batch_pixels`)

The demo composites a tiled checker-style grid drawn with **`cgfx_surface_fill_rect_batch_pixels`** (stride `0` means packed rows of **`cgfx_surface_fill_rect_item`**) and overlays the middle rectangle using the single **`cgfx_surface_fill_rect_pixels`** helper so both paths remain exercised each frame.

To verify Phase 2.3 locally:

- Build and test via the presets (see sections above): `cmake --preset windows-mingw-debug`, then `cmake --build --preset build-windows-mingw-debug`, finally `ctest --preset test-windows-mingw-debug`
- Launch `cgfx_demo` from the configure directory and confirm both the tiled background (batch path) and central highlight (legacy single-rect API) render without errors

Stride notes:

- `stride_bytes >= sizeof(cgfx_surface_fill_rect_item)` (or pass `0` to use exactly that size).
- Rows may embed extra trailing padding/strides commonly used when batch data lives in richer engine structs—the first **`sizeof(cgfx_surface_fill_rect_item)`** bytes of each row must mirror the canonical layout (**x**, **y**, **width**, **height**, tightly followed by **`r,g,b,a`** floats).
- `items` is a `const void *` row pointer (`cgfx_surface_fill_rect_item*` is fine when rows are densely packed).

## Test

```bash
ctest --test-dir build -C Release
```

Notes:
- The `-C Release` flag is primarily relevant for multi-config generators (for example, Visual Studio).
- For single-config generators (Ninja/Make), this command still works with the existing build directory.

## Baseline Verification (Phase 2–3)

The library baseline is considered stable when all of these pass:
- Configure: `cmake --preset windows-mingw-debug` (or equivalent platform preset)
- Build: `cmake --build --preset build-windows-mingw-debug`
- Tests: `ctest --preset test-windows-mingw-debug`

## Project Layout (Current Phase)

- `include/cgfx/` - public C headers
- `src/api/` - C API entry points
- `src/core/` - context + window internals + **`widget_tree`**
- `src/layout/` - Phase 4 flex sizing / measurement engine
- `src/platform/` - Win32/X11 platform backends
- `src/render/` - minimal OpenGL surface/present helpers
- `examples/` - demo app
- `tests/` - smoke tests + **`event_queue_test`** + **`widget_layout_test`**

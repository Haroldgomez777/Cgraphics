# cgfx (Phase 2 → Phase 8 animations)

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

### Phase 5: retained `Panel` / `Label` / `Button`

Phase 5 layers a **facet registry** on the existing tree/flex core (`src/widgets/basic_widgets.*`) so visuals and simple interaction stay modular. Layout and handles remain in **`WidgetTree`**; each basic widget is created as a dedicated child with an attached facet (`cgfx_basic_widget_kind`).

| Concern | Responsibility |
| --- | --- |
| **Geometry** | Phase 4 flex + `cgfx_widget_bounds_logical_px` unchanged. |
| **Hit testing** | **`cgfx_window_hit_test_logical_px`** remains a **pure geometric** pick (back-compat). Routed pointer events (**`target_widget`** on move/button) use **`visible=false` facet** filtering so hidden subtrees behave like pass-through. |
| **Rendering** | **`cgfx_window_draw_basic_widgets(win)`** emits fills via **Phase 6 style resolution** plus **Phase 7** deterministic text measurement (`src/text`): label and button captions use resolved **`cgfx_theme_metric_token`** font sizes scaled by **present-pass DPI**, centered placeholder rectangles sized to **`cgfx_text_measure_utf8_line_pixels`**, tinted with **`CGFX_THEME_COLOR_LABEL_TEXT`** / **`CGFX_THEME_COLOR_BUTTON_TEXT`** (not the older placeholder strips). Rasterization hooks live in **`text_glyph_raster_placeholder.hpp`** (no textured glyphs yet — see Phase 7.1 below). |
| **Text** | UTF-8 on facets; **`cgfx_text_measure_utf8_line_pixels`** / **`cgfx_text_measure_utf8_line_cstr_pixels`**, **`cgfx_font_builtin_acquire_mono_stub`**, and **`cgfx_context_set_text_font`** (or **`cgfx_context_text_font_select`**) form the seam. Glyph upload / atlas / shaders remain **TODO** (`submit_glyph_rasterization_placeholder_todo`). |
| **Clicks** | **Polling model:** on left-button release after a press on the **same** logical `BUTTON` facet, the library appends **`CGFX_EVENT_WIDGET_CLICK`** with **`cgfx_event_widget_click_payload`** (**`widget_id`**, **`button`**, **`x`**, **`y`**). dequeue via **`cgfx_next_event_into`**. Disabled buttons suppress hover tinting and activation. |

```c
cgfx_widget_id root = cgfx_window_widget_root(win);

/* Phase 7: optional pre-flight measurement (same stub metrics as basic widgets). */
cgfx_font_id face = CGFX_FONT_ID_INVALID;
cgfx_font_builtin_acquire_mono_stub(ctx, &face);

float dpi = 1.f;
(void)cgfx_window_get_dpi_scale(win, &dpi);

cgfx_text_line_metrics cap = {0};
if (cgfx_text_measure_utf8_line_cstr_pixels(
        ctx, face, "OK", /*font_size_sp=*/14.f, dpi, &cap) == CGFX_OK) {
  /* cap.width_px / cap.line_height_px — widgets call the same primitive in
     cgfx_window_draw_basic_widgets when text is visible. */
}
cgfx_basic_widget_panel_create(win, root, &dock);
cgfx_widget_set_layout_axis(win, dock, CGFX_LAYOUT_AXIS_COLUMN);

cgfx_widget_id title = 0;
cgfx_basic_widget_label_create(win, dock, &title);
cgfx_basic_widget_utf8_text_set(win, title, "Title", strlen("Title"));

cgfx_widget_id submit = 0;
cgfx_basic_widget_button_create(win, dock, &submit);
cgfx_basic_widget_utf8_text_set(win, submit, "OK", strlen("OK"));

/* Inside present pass each frame after flex runs: */
/* cgfx_window_draw_basic_widgets(win); */

/* Event loop — handle synthesized activation alongside platform input */
cgfx_event ev;
while (cgfx_next_event_into(ctx, &ev)) {
  if (ev.type == CGFX_EVENT_WIDGET_CLICK &&
      ev.window == win &&
      ev.payload.widget_click.widget_id == submit) {
    /* Button activated — consume by handling here only (no bubbling on this channel). */
  }
}
```

### Phase 5: tests

- **`cgfx_basic_widgets_test`** — facet kind round-trip, UTF-8 length, **`RenderCommandList`** fill-rect tally, visibility-filter hit pass-through (**no GPU**).

### Phase 6: styling and theming (tokens + overrides + resolver)

Phase 6 adds a **scoped resolution path** layered _without replacing_ the retained `WidgetTree` / basic-widget facet registry:

| Layer | Responsibility |
| --- | --- |
| **`UiTheme` (per window)** | Canonical **palette + spacing / typography placeholders** (`panel_background`, button state backgrounds, **`padding_sm_px`**, **`label_font_size_sp`**, etc.). Fresh windows start from **`cgfx_theme_reset_to_defaults`** parity (= historical Phase 5 literals). Mutations affect the **next** `cgfx_window_draw_basic_widgets` pass (no deferred invalidation caches). |
| **`WidgetStyleOverrides` (map)** | **Explicit per-widget bitmask overrides** wired through `cgfx_widget_style_set_*`; cleared selectively with `cgfx_widget_style_clear_overrides`. Entries are destroyed when subtrees are removed (`cgfx_widget_destroy` purges facets + style rows). |
| **Facet carry-over (Phase 5)** | `PanelFacet.bg_explicit`: **`cgfx_basic_widget_panel_set_background_rgba_normalized`** pins RGBA on the facet for compatibility. It resolves **below** Phase 6 overrides **but above** the panel theme token. There is **no** dedicated C API to clear this facet flag; new work should use overrides only, or recreate the widget if you must drop legacy state. |
| **Effective paint/debug** | `BasicWidgets::paint` asks `style_resolution::*` helpers: **override → legacy explicit facet → theme token**. `cgfx_widget_style_query_resolved_*` mirrors the combiner for debugging. |

**Panel background — recommended API:** use **`cgfx_widget_style_set_panel_background_rgba_normalized`** and **`cgfx_widget_style_clear_overrides(..., CGFX_WIDGET_STYLE_OVERRIDE_PANEL_BACKGROUND)`**. Treat **`cgfx_basic_widget_panel_set_background_rgba_normalized`** as Phase 5 compatibility only (separate storage from overrides; see precedence table above).

**Opaque theme snapshots (`cgfx_theme`)** duplicate the same blob as each window — use **`cgfx_window_theme_copy_to` / `cgfx_window_theme_apply`** or create standalone bundles with **`cgfx_theme_create`**.

```c
/* Darken every panel + hover affordance globally for one window — next frame visuals */
cgfx_color_rgba navy = {0.05f, 0.08f, 0.16f, 1.f};
cgfx_window_theme_set_color_rgba(win, CGFX_THEME_COLOR_PANEL_BACKGROUND, &navy);

cgfx_color_rgba hot = {0.35f, 0.55f, 0.95f, 1.f};
cgfx_window_theme_set_color_rgba(win, CGFX_THEME_COLOR_BUTTON_BACKGROUND_HOVER, &hot);

/* Pin a single panel while other panels still follow the live theme */
cgfx_widget_style_set_panel_background_rgba_normalized(win, settings_panel, 0.12f, 0.12f, 0.14f, 1.f);

/* Roll back one channel */
cgfx_widget_style_clear_overrides(win, settings_panel,
    CGFX_WIDGET_STYLE_OVERRIDE_PANEL_BACKGROUND);

/* Debug: inspect hypothetical button face without driving real pointer capture */
cgfx_color_rgba face{};
cgfx_widget_style_query_resolved_button_face_rgba_normalized(
    win, play_button, CGFX_BUTTON_FACE_QUERY_PRESSED, &face);
```

### Phase 6: tests

- **`cgfx_phase6_style_test`** — token-driven paint, override vs theme vs legacy panel precedence, selective `clear_overrides` vs legacy facet, query API vs paint, hover path, pressed override via resolution queries (**no GPU**).

### Phase 7: text measurement + widget integration (deterministic stub)

Phase 7 introduces **`src/text/`**: a platform-neutral **stub font**, **deterministic UTF-8 line measurement** (no HarfBuzz/FreeType linkage yet), **`FontRegistry`** on **`cgfx_context`**, and **`BasicWidgets`** paint wired to **`cgfx_text_measure_utf8_line_pixels`** (same formulas as widgets when DPI + theme match).

| Layer | Responsibility |
| --- | --- |
| **`FontRegistry`** | Validates **`cgfx_font_id`** handles. Today only **`CGFX_FONT_ID_BUILTIN_DEFAULT`** (**`cgfx_font_builtin_acquire_mono_stub`** or **`cgfx_font_builtin_acquire(..., CGFX_FONT_BUILTIN_MONO_STUB)`**) is admitted. |
| **`text_logical_font_px_round` / `text_measure_utf8_line_stub`** | Rounds **`font_size_sp * dpi_scale`** to integer pixels (`logical_font_px`). Horizontal advances classify Latin vs wide codepoint ranges plus a bounded “other-script” blend. Line box uses fixed ascent/descents derived from **`logical_font_px`**. Invalid UTF-8 advances one byte yielding **`U+FFFD`**. Measurement stops at the first **`\\n`** (single logical line API). |
| **Raster seam** | `text_glyph_raster_placeholder.hpp` documents **`submit_glyph_rasterization_placeholder_todo`** (currently empty). **`BasicWidgets`** still emits a **centered fill-rect strip** tinted with **`label_text_color` / resolved button text color**. |
| **Presenter DPI** | `CgfxWindow` saves the **`begin_present_pass`** DPI scale so **`cgfx_window_draw_basic_widgets`** matches device measurement without leaking Win32/X11 specifics into **`BasicWidgets`**. |

Measured line layout helper (standalone or ahead of sizing code). Use
`cgfx_text_measure_utf8_line_cstr_pixels` for NUL-terminated literals; pass an explicit byte length
when you have a substring or buffer without an interior NUL anchor.

```c
cgfx_font_id face = CGFX_FONT_ID_INVALID;
cgfx_font_builtin_acquire_mono_stub(ctx, &face);

float dpi = 1.f;
cgfx_window_get_dpi_scale(win, &dpi);

cgfx_text_line_metrics m = {0};
cgfx_text_measure_utf8_line_cstr_pixels(ctx, CGFX_FONT_ID_INVALID /* context default font */,
                                        "Hello\nTail", 13.f /* sp */, dpi, &m);
/* m.width_px covers "Hello"; newline truncates horizontal accumulation (single-line API). */
```

Prefer **get/set** names when exploring the headers; they alias the `_text_font_*` entry points:

```c
cgfx_context_set_text_font(ctx, face);
cgfx_font_id round_trip = CGFX_FONT_ID_INVALID;
cgfx_context_get_text_font(ctx, &round_trip);
```

Select the context’s default measurement font once (optional; defaults to `CGFX_FONT_ID_BUILTIN_DEFAULT`):

```c
cgfx_context_set_text_font(ctx, face);
```

#### Phase 7: C API entry points

- **`cgfx_font_builtin_acquire`** / **`cgfx_font_builtin_acquire_mono_stub`**; **`CGFX_FONT_BUILTIN_KIND_DEFAULT`** aliases the lone stub kind today.
- **`cgfx_context_set_text_font`** / **`cgfx_context_get_text_font`** (discoverable aliases for `cgfx_context_text_font_select` / `cgfx_context_text_font_selected`).
- **`cgfx_text_measure_utf8_line_pixels`** and **`cgfx_text_measure_utf8_line_cstr_pixels`**: pass **`CGFX_FONT_ID_INVALID`** for `font_id` to use the context-selected font. Non-finite **`dpi_scale`** or **`<= 0`** clamps to **`1.f`**. For **`utf8_byte_length == 0`** with a non-NULL pointer, length is **`strlen`**.

#### Phase 7: tests

- **`cgfx_text_measurement_test`** — public C API + deterministic width rules + DPI scaling + invalid args.
- **`cgfx_widget_text_integration_test`** — offline paint rectangles match `text_layout_placeholder_centered` planning for label + button captions.

#### Phase 7: known limitations

- No real glyph outlines, kerning, ligatures, bidi, or font files — metrics are **documented stub tables** only.
- Label placeholder marker color (`LabelFacet.marker_explicit` / `CGFX_THEME_COLOR_LABEL_PLACEHOLDER`) is **not** used for the Phase 7 caption strip; paint uses **`CGFX_THEME_COLOR_LABEL_TEXT`** (plus Phase 6 text-color overrides). Use marker fields for future decoration if needed.
- Multi-line wrapping, ellipses, and rich text are **Phase 9+** (animations are **Phase 8**).
- Passing **`CGFX_FONT_ID_INVALID`** to **`cgfx_text_measure_utf8_line_pixels`** resolves the context’s **`cgfx_context_text_font_selected`** (**`cgfx_context_get_text_font`**) handle.

#### Phase 7.1 (next)

- Load/select **platform or bundled `.ttf` / `.otf`** resources through **`FontRegistry`**, preserving the same measurement API surface where possible.
- Implement **`submit_glyph_rasterization_placeholder_todo`** with atlas + shader path on **`RenderCommandList`** (new command bucket) while keeping **`BasicWidgets` measurement-only** coupling.
- Optional CoreText/DirectWrite shapers behind **`text_measure_utf8_line_stub`** indirection.

### Phase 8: property animations (timeline + clock + easing)

Phase 8 introduces a **small, platform-neutral animation core** wired into **`cgfx_window_begin_present_pass`** and **`cgfx_window_draw_basic_widgets`**:

| Layer | Responsibility |
| --- | --- |
| **`AnimationClock`** (`src/animation/animation_clock.*`) | **Wall-clock** head (`std::chrono::steady_clock`) or **manual** deterministic time (`cgfx_context_animation_*` helpers). |
| **`animation_easing`** (`src/animation/easing.*`) | Normalized **`CGFX_ANIM_EASE_*`** curves for clip sampling. |
| **`WidgetAnimationSystem`** (`src/animation/widget_animation_system.*`) | Per-window clips: translate (logical px), opacity multiplier, lerped fill RGBA. **Newest clip id wins** when several clips collide on the same widget channel. |
| **`IAnimPaintCompositor` / `AnimPaintMod`** | Adapter passed into **`BasicWidgets::paint`** so widgets remain decoupled from backends. |

**C API (additive):** **`cgfx_context_[get|set]_animation_speed_scale`**, manual clock getters/setters, **`cgfx_animation_start_*`** / **`cgfx_animation_stop`**, **`cgfx_animation_stop_widget_property`**, **`cgfx_animation_is_active`** (see `cgfx_api.h`). Destroying widgets purges overlapping animation clips alongside facets/styles.

Each **`cgfx_window_begin_present_pass`** runs flex layout, samples the context clock vs the window’s prior sample (first step uses **`dt = 0`**), clamps **`dt`** to **`0.25` s**, scales by **`animation_speed_scale`**, then advances that window’s clip timeline before recording GL commands.

**Tests:** **`cgfx_animation_phase8_test`** (easing + deterministic `advance` + paint rect integration). Verification: **`cmake --preset windows-mingw-debug`**, **`cmake --build --preset build-windows-mingw-debug`**, **`ctest --preset test-windows-mingw-debug`**.

#### Phase 8: caveats

- **Translate** offsets affect **paint only** — hit testing still uses intrinsic flex rects.
- Stopping clips **drops** modulation (no implicit “sticky” final style).
- Animated opacity / fill apply to **basic-widget** fill-rect emission; **`cgfx_widget_style_query_resolved_*`** remain static.

#### Phase 8.1 (next)

- Hit-test parity for translated visuals (or layout-level offset channels).
- Completion callbacks / loops / overlapping policy beyond newest-wins.

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

## Baseline Verification

The library baseline is considered stable when **`cmake --preset windows-mingw-debug`**, **`cmake --build --preset build-windows-mingw-debug`**, and **`ctest --preset test-windows-mingw-debug`** all succeed on Windows (or the equivalent Linux preset). Include **Phase 7–8** coverage — **`cgfx_text_measurement_test`**, **`cgfx_widget_text_integration_test`**, **`cgfx_animation_phase8_test`** — alongside earlier suites.

## Project Layout (Current Phase)

- `include/cgfx/` - public C headers
- `src/api/` - C API entry points
- `src/core/` - context + window internals + **`widget_tree`**
- `src/layout/` - Phase 4 flex sizing / measurement engine
- `src/style/` - Phase 6 theme tokens + override map + resolution helpers
- `src/animation/` - Phase 8 animation clock, easing, timelines, paint compositor adapters
- `src/text/` - Phase 7 font registry + deterministic measurement + raster placeholder seam
- `src/platform/` - Win32/X11 platform backends
- `src/render/` - minimal OpenGL surface/present helpers
- `examples/` - demo app
- `tests/` - smoke tests + **`event_queue_test`** + **`widget_layout_test`**

# cgfx (Phase 2 + Phase 3 events)

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
- **`cgfx_event_queue_test`** — ordering, resize coalesce, DROP_OLDEST / DROP_NEWEST (no window or GPU required).

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
- `src/core/` - context + window core internals
- `src/platform/` - Win32/X11 platform backends
- `src/render/` - minimal OpenGL surface/present helpers
- `examples/` - demo app
- `tests/` - smoke tests + Phase 3 `event_queue` unit test

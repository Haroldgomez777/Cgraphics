# cgfx (Phase 2)

`cgfx` is a cross-platform GUI graphics framework in C/C++.

Phase 1 includes:
- Platform window creation (`Win32`, `X11`)
- OpenGL context bootstrap (`WGL`, `GLX`)
- Event polling and normalized input events
- Present pass with double buffering
- Minimal C API and smoke test

Phase 2 adds incremental render-command recording for the present pass (clear, filled rectangles including a batched path).

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

## Test

```bash
ctest --test-dir build -C Release
```

Notes:
- The `-C Release` flag is primarily relevant for multi-config generators (for example, Visual Studio).
- For single-config generators (Ninja/Make), this command still works with the existing build directory.

## Baseline Verification (Phase 2)

Phase 1 baseline is considered stable when all of these pass:
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
- `tests/` - smoke tests

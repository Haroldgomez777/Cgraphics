#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "core/context.hpp"
#include "core/events/event_dispatch.hpp"
#include "core/window_impl.hpp"

#include <windows.h>
#include <windowsx.h>

#include <GL/gl.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace cgfx::win32_detail {

template <typename Fn>
static Fn GetUser32Proc(const char *name) noexcept {
  HMODULE user32 = ::GetModuleHandleW(L"user32.dll");
  if (!user32 || !name) {
    return nullptr;
  }

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
  return reinterpret_cast<Fn>(::GetProcAddress(user32, name));
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
}

static UINT QueryWindowDpi(HWND hwnd, HDC hdc) noexcept {
  if (!hwnd || !hdc) {
    return 96U;
  }

  UINT dpi = static_cast<UINT>(
      std::ceil(static_cast<double>(::GetDeviceCaps(hdc, LOGPIXELSX))));

  using GetDpiForWindowFn = UINT(WINAPI *)(HWND hwnd_in);
  GetDpiForWindowFn get_dpi = GetUser32Proc<GetDpiForWindowFn>("GetDpiForWindow");
  if (get_dpi != nullptr) {
    dpi = (std::max)(dpi, get_dpi(hwnd));
  }

  return dpi;
}

static void EnableHighDpiAwarenessBestEffort() noexcept {
  using SetProcessDpiAwarenessContextFn = BOOL(WINAPI *)(HANDLE);
  SetProcessDpiAwarenessContextFn set_pm_v2 =
      GetUser32Proc<SetProcessDpiAwarenessContextFn>(
          "SetProcessDpiAwarenessContext");
  if (set_pm_v2) {
    // -4 maps to DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2.
    const HANDLE kPerMonitorAwareV2 =
        reinterpret_cast<HANDLE>(static_cast<LONG_PTR>(-4));
    if (set_pm_v2(kPerMonitorAwareV2)) {
      return;
    }
  }

  using SetProcessDpiAwareFn = BOOL(WINAPI *)();
  SetProcessDpiAwareFn set_dd =
      GetUser32Proc<SetProcessDpiAwareFn>("SetProcessDPIAware");
  if (set_dd) {
    (void)set_dd();
  }
}

static std::wstring Utf8ToWide(const char *maybe_utf8) {
  if (!maybe_utf8 || maybe_utf8[0] == '\0') {
    return L"cgfx";
  }

  const int required =
      ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, maybe_utf8, -1,
                            nullptr, 0);
  if (required <= 0) {
    return L"cgfx";
  }

  std::wstring out(static_cast<size_t>(required), L'\0');
  ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, maybe_utf8, -1,
                        out.data(), required);
  if (!out.empty() && out.back() == L'\0') {
    out.pop_back();
  }

  return out;
}

static LPCWSTR WindowClassName() noexcept { return L"cgfx_win32_phase1"; }

static cgfx_mouse_button DecodeMouseButton(UINT msg,
                                           WPARAM wparam) noexcept {
  switch (msg) {
  case WM_LBUTTONDOWN:
  case WM_LBUTTONDBLCLK:
  case WM_LBUTTONUP:
    return CGFX_MOUSE_LEFT;

  case WM_RBUTTONDOWN:
  case WM_RBUTTONDBLCLK:
  case WM_RBUTTONUP:
    return CGFX_MOUSE_RIGHT;

  case WM_MBUTTONDOWN:
  case WM_MBUTTONDBLCLK:
  case WM_MBUTTONUP:
    return CGFX_MOUSE_MIDDLE;

  default:
    break;
  }

  const UINT hx = GET_XBUTTON_WPARAM(wparam);
  return (hx == XBUTTON1) ? CGFX_MOUSE_X1 : CGFX_MOUSE_X2;
}

static cgfx_input_action DecodeMouseAction(UINT msg) noexcept {
  switch (msg) {
  case WM_LBUTTONUP:
  case WM_RBUTTONUP:
  case WM_MBUTTONUP:
  case WM_XBUTTONUP:
    return CGFX_RELEASE;
  default:
    return CGFX_PRESS;
  }
}

static bool IsExtendedKey(LPARAM lparam) noexcept {
  return (lparam & (static_cast<LPARAM>(1) << 24)) != 0;
}

static cgfx_key MapVirtualKey(UINT vk, LPARAM lparam) noexcept {
  switch (vk) {
  case VK_ESCAPE:
    return CGFX_KEY_ESCAPE;
  case VK_SPACE:
    return CGFX_KEY_SPACE;
  case VK_RETURN:
    return CGFX_KEY_ENTER;
  case VK_TAB:
    return CGFX_KEY_TAB;
  case VK_BACK:
    return CGFX_KEY_BACKSPACE;
  case VK_DELETE:
    return CGFX_KEY_DELETE;
  case VK_LEFT:
    return CGFX_KEY_LEFT;
  case VK_RIGHT:
    return CGFX_KEY_RIGHT;
  case VK_UP:
    return CGFX_KEY_UP;
  case VK_DOWN:
    return CGFX_KEY_DOWN;
  case VK_HOME:
    return CGFX_KEY_HOME;
  case VK_END:
    return CGFX_KEY_END;
  case VK_PRIOR:
    return CGFX_KEY_PAGE_UP;
  case VK_NEXT:
    return CGFX_KEY_PAGE_DOWN;
  case VK_F1:
    return CGFX_KEY_F1;
  case VK_F2:
    return CGFX_KEY_F2;
  case VK_F3:
    return CGFX_KEY_F3;
  case VK_F4:
    return CGFX_KEY_F4;
  case VK_F5:
    return CGFX_KEY_F5;
  case VK_F6:
    return CGFX_KEY_F6;
  case VK_F7:
    return CGFX_KEY_F7;
  case VK_F8:
    return CGFX_KEY_F8;
  case VK_F9:
    return CGFX_KEY_F9;
  case VK_F10:
    return CGFX_KEY_F10;
  case VK_F11:
    return CGFX_KEY_F11;
  case VK_F12:
    return CGFX_KEY_F12;
  default:
    break;
  }

  if (vk == VK_SHIFT) {
    return CGFX_KEY_LEFT_SHIFT;
  }
  if (vk == VK_CONTROL) {
    return IsExtendedKey(lparam) ? CGFX_KEY_RIGHT_CONTROL
                                 : CGFX_KEY_LEFT_CONTROL;
  }
  if (vk == VK_MENU) {
    return IsExtendedKey(lparam) ? CGFX_KEY_RIGHT_ALT : CGFX_KEY_LEFT_ALT;
  }

  if (vk >= static_cast<UINT>('A') && vk <= static_cast<UINT>('Z')) {
    const int offset = static_cast<int>(vk) - static_cast<int>('A');
    return static_cast<cgfx_key>(static_cast<int>(CGFX_KEY_A) + offset);
  }

  return CGFX_KEY_UNKNOWN;
}

static cgfx_input_action DecodeKeyboardAction(UINT msg,
                                              LPARAM lparam) noexcept {
  if (msg == WM_KEYUP || msg == WM_SYSKEYUP) {
    return CGFX_RELEASE;
  }

  const unsigned long bits =
      static_cast<unsigned long>(
          static_cast<unsigned long long>(lparam) & 0xFFFFFFFFULL);
  const unsigned long prev_down = (bits >> 30U) & 1UL;
  return prev_down ? CGFX_REPEAT : CGFX_PRESS;
}

static void RegisterWindowClassOnce(HINSTANCE module,
                                    WNDPROC wnd_proc) noexcept {
  static bool done = false;
  if (done) {
    return;
  }
  done = true;

  if (!module || !wnd_proc) {
    return;
  }

  WNDCLASSEXW wc{};
  wc.cbSize = sizeof(wc);
  wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = wnd_proc;
  wc.hInstance = module;
  wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground =
      reinterpret_cast<HBRUSH>(static_cast<ULONG_PTR>(COLOR_WINDOW) + 1);
  wc.lpszClassName = WindowClassName();

  (void)::RegisterClassExW(&wc);
}

class Win32Surface final : public PlatformSurface {
public:
  explicit Win32Surface(CgfxWindow *owner_in) noexcept
      : PlatformSurface(owner_in) {}

  Win32Surface(const Win32Surface &) = delete;
  Win32Surface &operator=(const Win32Surface &) = delete;

  ~Win32Surface() override { teardown(); }

  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam,
                                  LPARAM lparam) noexcept {
    Win32Surface *surface = reinterpret_cast<Win32Surface *>(
        ::GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (msg == WM_CREATE) {
      auto *cs = reinterpret_cast<CREATESTRUCTW *>(lparam);
      auto *self = static_cast<Win32Surface *>(cs->lpCreateParams);
      ::SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                         reinterpret_cast<LONG_PTR>(self));

      if (self->BootstrapGl(hwnd) != 0) {
        return static_cast<LRESULT>(-1);
      }

      return ::DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    if (!surface) {
      return ::DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    switch (msg) {
    case WM_SIZE: {
      RECT client{};
      if (::GetClientRect(hwnd, &client) != FALSE) {
        surface->EnqueueResize(client);
      }
      break;
    }

    case WM_CLOSE: {
      CgfxWindow *wnd = surface->owner();
      event_dispatch_close(wnd->context(), wnd);
      return 0;
    }

    case WM_MOUSEMOVE: {
      CgfxWindow *wnd = surface->owner();
      event_dispatch_mouse_move(wnd->context(), wnd, GET_X_LPARAM(lparam),
                                GET_Y_LPARAM(lparam));
      break;
    }

    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONDBLCLK:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP: {
      CgfxWindow *wnd = surface->owner();
      event_dispatch_mouse_button(wnd->context(), wnd,
                               DecodeMouseButton(msg, wparam),
                               DecodeMouseAction(msg),
                               GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
      break;
    }

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYUP: {
      const UINT vk =
          static_cast<UINT>(wparam & static_cast<WPARAM>(0xFFFFFFFFull));

      const cgfx_input_action action = DecodeKeyboardAction(msg, lparam);

      CgfxWindow *wnd = surface->owner();
      event_dispatch_key(wnd->context(), wnd, MapVirtualKey(vk, lparam),
                        static_cast<uint32_t>(vk), action,
                         (action == CGFX_REPEAT) ? 1 : 0);
      break;
    }

    default:
      break;
    }

    return ::DefWindowProcW(hwnd, msg, wparam, lparam);
  }

  cgfx_result CreateWindowNative(const cgfx_window_desc *desc,
                                 HINSTANCE module) {
    if (!desc) {
      return CGFX_ERROR_INVALID_ARGUMENT;
    }

    module_ = module;

    const std::wstring title = Utf8ToWide(desc->title);

    RECT bounds{};
    bounds.right =
        desc->width ? static_cast<LONG>(desc->width) : static_cast<LONG>(960);
    bounds.bottom = desc->height ? static_cast<LONG>(desc->height)
                                 : static_cast<LONG>(540);

    const DWORD style = static_cast<DWORD>(
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    if (::AdjustWindowRect(&bounds, style, FALSE) == FALSE) {
      return CGFX_ERROR_PLATFORM;
    }

    const int outer_w = static_cast<int>(bounds.right - bounds.left);
    const int outer_h = static_cast<int>(bounds.bottom - bounds.top);

    HWND hwnd = ::CreateWindowExW(
        0, WindowClassName(), title.c_str(), style, CW_USEDEFAULT,
        CW_USEDEFAULT, outer_w, outer_h, nullptr, nullptr, module_,
        reinterpret_cast<void *>(this));

    if (!hwnd) {
      return CGFX_ERROR_PLATFORM;
    }

    (void)hwnd;
    return CGFX_OK;
  }

  int BootstrapGl(HWND hwnd) {
    hwnd_ = hwnd;
    hdc_ = ::GetDC(hwnd_);
    if (!hdc_) {
      return -1;
    }

    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = static_cast<DWORD>(
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER);
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;

    const int pixel_fmt = ::ChoosePixelFormat(hdc_, &pfd);
    if (pixel_fmt <= 0) {
      return -1;
    }
    if (::SetPixelFormat(hdc_, pixel_fmt, &pfd) == FALSE) {
      return -1;
    }

    glrc_ = ::wglCreateContext(hdc_);
    if (!glrc_) {
      return -1;
    }

    owns_resources_ = true;

    RECT client{};
    if (::GetClientRect(hwnd_, &client) != FALSE) {
      EnqueueResize(client);
    }

    ::ShowWindow(hwnd_, SW_SHOW);
    ::UpdateWindow(hwnd_);

    return 0;
  }

  void teardown() noexcept override {
    if (!owns_resources_) {
      hwnd_ = nullptr;
      hdc_ = nullptr;
      glrc_ = nullptr;
      in_present_ = false;
      return;
    }

    owns_resources_ = false;

    if (glrc_) {
      (void)::wglMakeCurrent(nullptr, nullptr);
      ::wglDeleteContext(glrc_);
      glrc_ = nullptr;
    }

    HWND hwnd_local = hwnd_;
    hwnd_ = nullptr;

    if (hwnd_local != nullptr && hdc_ != nullptr) {
      (void)::ReleaseDC(hwnd_local, hdc_);
    }
    hdc_ = nullptr;

    if (hwnd_local != nullptr) {
      ::DestroyWindow(hwnd_local);
    }

    in_present_ = false;
  }

  cgfx_result begin_present(uint32_t *out_width, uint32_t *out_height,
                            float *out_scale) override {
    if (!hwnd_ || !hdc_ || !glrc_) {
      return CGFX_ERROR_PLATFORM;
    }

    RECT client{};
    if (::GetClientRect(hwnd_, &client) == FALSE) {
      return CGFX_ERROR_PLATFORM;
    }

    UpdateClientExtents(client);

    if (::wglMakeCurrent(hdc_, glrc_) == FALSE) {
      return CGFX_ERROR_PLATFORM;
    }

    const UINT dpi = QueryWindowDpi(hwnd_, hdc_);

    float scale =
        static_cast<float>(dpi) / static_cast<float>(96); // NOLINT
    if (!(scale > 0.0f)) {
      scale = 1.0f;
    }

    in_present_ = true;

    if (out_width != nullptr) {
      *out_width = static_cast<uint32_t>(last_w_);
    }
    if (out_height != nullptr) {
      *out_height = static_cast<uint32_t>(last_h_);
    }
    if (out_scale != nullptr) {
      *out_scale = scale;
    }

    return CGFX_OK;
  }

  void end_present() override {
    if ((hdc_ != nullptr) && in_present_) {
      (void)::SwapBuffers(hdc_);
    }
    in_present_ = false;
    (void)::wglMakeCurrent(nullptr, nullptr);
  }

  cgfx_result query_size_px(uint32_t *out_w,
                            uint32_t *out_h) const override {
    if (!out_w || !out_h) {
      return CGFX_ERROR_INVALID_ARGUMENT;
    }
    if (!hwnd_) {
      return CGFX_ERROR_PLATFORM;
    }

    RECT rc{};
    if (::GetClientRect(hwnd_, &rc) == FALSE) {
      return CGFX_ERROR_PLATFORM;
    }

    const LONG w = rc.right - rc.left;
    const LONG h = rc.bottom - rc.top;
    *out_w = static_cast<uint32_t>(std::max<LONG>(w, 1));
    *out_h = static_cast<uint32_t>(std::max<LONG>(h, 1));

    return CGFX_OK;
  }

  cgfx_result query_dpi_scale(float *out_scale) const override {
    if (!out_scale) {
      return CGFX_ERROR_INVALID_ARGUMENT;
    }

    if (!hdc_ || !hwnd_) {
      return CGFX_ERROR_PLATFORM;
    }

    const UINT dpi = QueryWindowDpi(hwnd_, hdc_);

    float scale =
        static_cast<float>(dpi) / static_cast<float>(96); // NOLINT

    if (!(scale > 0.0f)) {
      scale = 1.0f;
    }

    *out_scale = scale;

    return CGFX_OK;
  }

private:
  void UpdateClientExtents(const RECT &client) noexcept {
    last_w_ = std::max<LONG>(client.right - client.left, static_cast<LONG>(1));
    last_h_ = std::max<LONG>(client.bottom - client.top, static_cast<LONG>(1));
  }

  void EnqueueResize(const RECT &client) noexcept {
    UpdateClientExtents(client);

    const LONG w_px = client.right - client.left;
    const LONG h_px = client.bottom - client.top;
    const uint32_t w =
        static_cast<uint32_t>(std::max<LONG>(w_px, static_cast<LONG>(1)));
    const uint32_t h =
        static_cast<uint32_t>(std::max<LONG>(h_px, static_cast<LONG>(1)));

    CgfxWindow *wnd = owner();
    event_dispatch_resize(wnd->context(), wnd, w, h);
  }

  bool owns_resources_{false};
  bool in_present_{false};

  HWND hwnd_{nullptr};
  HDC hdc_{nullptr};
  HGLRC glrc_{nullptr};

  LONG last_w_{1};
  LONG last_h_{1};
  HINSTANCE module_{nullptr};
};

class Win32Backend final : public PlatformBackend {
public:
  Win32Backend() : module_(::GetModuleHandleW(nullptr)) {
    EnableHighDpiAwarenessBestEffort();
    RegisterWindowClassOnce(module_, &Win32Surface::WndProc);
  }

  void poll_events(CgfxContext *) override {
    MSG msg{};
    while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE) != FALSE) {
      if (msg.message == WM_QUIT) {
        continue;
      }
      ::TranslateMessage(&msg);
      ::DispatchMessageW(&msg);
    }
  }

  std::unique_ptr<PlatformSurface>
  create_surface(CgfxWindow *owner, const cgfx_window_desc *desc,
                 cgfx_result &status) override {
    status = CGFX_OK;

    if ((!owner) || (!desc)) {
      status = CGFX_ERROR_INVALID_ARGUMENT;
      return nullptr;
    }

    auto surface = std::make_unique<Win32Surface>(owner);
    status = surface->CreateWindowNative(desc, module_);
    if (status != CGFX_OK) {
      surface->teardown();
      surface.reset();
      return nullptr;
    }

    return surface;
  }

private:
  HINSTANCE module_;
};

} // namespace cgfx::win32_detail

namespace cgfx {

std::unique_ptr<PlatformBackend> cgfx_make_win32_backend() {
  return std::make_unique<win32_detail::Win32Backend>();
}

} // namespace cgfx

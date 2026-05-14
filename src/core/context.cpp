#include "core/window_impl.hpp"
#include "core/context.hpp"

namespace cgfx {
#if defined(_WIN32)
std::unique_ptr<PlatformBackend> cgfx_make_win32_backend();
#elif defined(__linux__)
std::unique_ptr<PlatformBackend> cgfx_make_x11_backend();
#else
#error "cgfx: unsupported compilation target."
#endif

static std::unique_ptr<PlatformBackend> make_platform_backend() {
#if defined(_WIN32)
  return cgfx_make_win32_backend();
#else
  return cgfx_make_x11_backend();
#endif
}
} // namespace cgfx

namespace cgfx {

CgfxContext::CgfxContext() = default;

CgfxContext::~CgfxContext() { backend_.reset(); }

cgfx_result CgfxContext::create(std::unique_ptr<CgfxContext> &out) {
  try {
    auto ctx = std::make_unique<CgfxContext>();
    ctx->backend_ = make_platform_backend();
    if (!ctx->backend_) {
      return CGFX_ERROR_PLATFORM;
    }
    out = std::move(ctx);
    return CGFX_OK;
  } catch (...) {
    return CGFX_ERROR_OUT_OF_MEMORY;
  }
}

void CgfxContext::push_event(const QueuedEvent &ev) { events_.push_back(ev); }

void CgfxContext::push_priority_event_front(const QueuedEvent &ev) {
  events_.push_front(ev);
}

bool CgfxContext::pop_event(QueuedEvent &ev) {
  if (events_.empty()) {
    return false;
  }
  ev = events_.front();
  events_.pop_front();
  return true;
}

void CgfxContext::poll_events_impl() {
  if (backend_) {
    backend_->poll_events(this);
  }
}

cgfx_result CgfxContext::create_window_impl(const cgfx_window_desc *desc,
                                            cgfx_window **out_window) {
  if (!desc || !out_window) {
    return CGFX_ERROR_INVALID_ARGUMENT;
  }
  auto *wnd = new (std::nothrow) CgfxWindow(this);
  if (!wnd) {
    return CGFX_ERROR_OUT_OF_MEMORY;
  }
  const cgfx_result init_status = wnd->initialize(desc);
  if (init_status != CGFX_OK) {
    delete wnd;
    return init_status;
  }
  *out_window = wnd->opaque();
  return CGFX_OK;
}

void CgfxContext::destroy_window_impl(CgfxWindow *wnd) { delete wnd; }

} // namespace cgfx

#include "core/events/internal_event.hpp"
#include "core/events/event_queue.hpp"

#include <cassert>
#include <variant>
namespace {

cgfx::InternalEvent resize_evt(cgfx::CgfxWindow *w, uint32_t ww,
                               uint32_t hh) {
  cgfx_event_resize_payload p{};
  p.width = ww;
  p.height = hh;
  return cgfx::InternalEvent::resize(w, p);
}

cgfx::InternalEvent move_evt(cgfx::CgfxWindow *w, int32_t x, int32_t y) {
  cgfx_event_mouse_move_payload p{};
  p.x = x;
  p.y = y;
  return cgfx::InternalEvent::mouse_move(w, p);
}

} // namespace

int main() {
  using cgfx::EventQueue;
  using cgfx::EventQueueOverflowPolicy;
  using cgfx::InternalEvent;

  auto *wnd = reinterpret_cast<cgfx::CgfxWindow *>(uintptr_t{0x4});

  // FIFO ordering between distinct event kinds
  {
    EventQueue q;
    q.set_coalesce_resize(false);
    cgfx_event_close_payload c{};
    q.push(InternalEvent::close_request(wnd, c));
    q.push(move_evt(wnd, 1, 2));
    InternalEvent a{};
    InternalEvent b{};
    assert(q.pop(a));
    assert(q.pop(b));
    assert(cgfx::internal_event_kind(a) == CGFX_EVENT_CLOSE_REQUEST);
    assert(cgfx::internal_event_kind(b) == CGFX_EVENT_MOUSE_MOVE);
    assert(q.empty());
  }

  // Same-window resize coalescing merges into tail slot (no reorder)
  {
    EventQueue q;
    q.set_max_depth(8);
    q.set_coalesce_resize(true);
    q.push(resize_evt(wnd, 10, 11));
    q.push(resize_evt(wnd, 20, 22));
    assert(q.size() == 1);
    InternalEvent e{};
    assert(q.pop(e));
    const auto *r = std::get_if<cgfx::EventResize>(&e.body);
    assert(r);
    assert(r->payload.width == 20);
    assert(r->payload.height == 22);
    assert(q.resize_coalesce_commit_count() >= 1);
  }

  // DROP_OLDEST makes room — oldest input event is sacrificed
  {
    EventQueue q;
    q.set_max_depth(2);
    q.set_coalesce_resize(false);
    q.set_overflow_policy(EventQueueOverflowPolicy::DropOldest);
    q.push(move_evt(wnd, 1, 1));
    q.push(move_evt(wnd, 2, 2));
    q.reset_drop_count();
    q.push(move_evt(wnd, 3, 3));
    InternalEvent first{};
    InternalEvent second{};
    assert(q.pop(first));
    assert(q.pop(second));
    assert(q.dropped_event_count() == 1);
    auto *fv = std::get_if<cgfx::EventMouseMove>(&first.body);
    auto *sv = std::get_if<cgfx::EventMouseMove>(&second.body);
    assert(fv && sv);
    assert(fv->payload.x == 2);
    assert(sv->payload.x == 3);
  }

  // push_priority_front stays within max_depth (tail eviction + drop stat)
  {
    EventQueue q;
    q.set_max_depth(2);
    q.set_coalesce_resize(false);
    q.set_overflow_policy(EventQueueOverflowPolicy::DropOldest);
    q.push(move_evt(wnd, 1, 1));
    q.push(move_evt(wnd, 2, 2));
    q.push_priority_front(move_evt(wnd, 9, 9));
    assert(q.size() == 2);
    assert(q.dropped_event_count() == 1);
    InternalEvent a{};
    InternalEvent b{};
    assert(q.pop(a));
    assert(q.pop(b));
    auto *av = std::get_if<cgfx::EventMouseMove>(&a.body);
    auto *bv = std::get_if<cgfx::EventMouseMove>(&b.body);
    assert(av && bv);
    assert(av->payload.x == 9);
    assert(bv->payload.x == 1);
  }

  // DROP_NEWEST rejects under back-pressure
  {
    EventQueue q;
    q.set_max_depth(2);
    q.set_coalesce_resize(false);
    q.set_overflow_policy(EventQueueOverflowPolicy::DropNewest);
    q.push(move_evt(wnd, 1, 1));
    q.push(move_evt(wnd, 2, 2));
    q.reset_drop_count();
    q.push(move_evt(wnd, 99, 99));
    assert(q.dropped_event_count() == 1);
    InternalEvent x{};
    InternalEvent y{};
    assert(q.pop(x));
    assert(q.pop(y));
    auto *xv = std::get_if<cgfx::EventMouseMove>(&x.body);
    auto *yv = std::get_if<cgfx::EventMouseMove>(&y.body);
    assert(xv && yv && xv->payload.x == 1 && yv->payload.x == 2);
  }

  return 0;
}

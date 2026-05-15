#include "core/events/event_queue.hpp"

#include <utility>

namespace cgfx {

void EventQueue::set_max_depth(size_t max_events) noexcept {
  max_depth_ =
      max_events > 0 ? max_events : static_cast<size_t>(kDefaultMaxDepth);
}

bool EventQueue::try_resize_coalesce_in_place_(const InternalEvent &incoming) {
  if (queue_.empty()) {
    return false;
  }
  InternalEvent &back = queue_.back();
  auto *out_resize = std::get_if<EventResize>(&back.body);
  const auto *in_resize = std::get_if<EventResize>(&incoming.body);
  if (!out_resize || !in_resize) {
    return false;
  }
  if (back.window != incoming.window) {
    return false;
  }
  out_resize->payload = in_resize->payload;
  resize_coalesce_hits_ += 1;
  return true;
}

void EventQueue::discard_oldest_() {
  if (queue_.empty()) {
    return;
  }
  queue_.pop_front();
  dropped_events_ += 1;
}

bool EventQueue::make_room_tail_() {
  if (queue_.empty() || queue_.size() < max_depth_) {
    return true;
  }

  switch (overflow_policy_) {
  case EventQueueOverflowPolicy::DropOldest:
    discard_oldest_();
    return queue_.size() < max_depth_;

  case EventQueueOverflowPolicy::DropNewest:
    dropped_events_ += 1;
    return false;
  }
  return false;
}

void EventQueue::push(InternalEvent ev) {
  if (coalesce_resize_) {
    if (try_resize_coalesce_in_place_(ev)) {
      return;
    }
  }

  if (!make_room_tail_()) {
    return;
  }

  ev.sequence = ++next_sequence_;
  last_enqueued_sequence_ = ev.sequence;
  queue_.push_back(std::move(ev));
}

void EventQueue::push_priority_front(const InternalEvent &ev) {
  // Re-queue path must stay bounded: a caller could spam this without reaching
  // max_depth (normal pushes coalesce evictions). Evict from the tail so the
  // retried head event always fits without unbounded deque growth.
  while (!queue_.empty() && queue_.size() >= max_depth_) {
    queue_.pop_back();
    dropped_events_ += 1;
  }
  InternalEvent copy = ev;
  if (copy.sequence == 0) {
    copy.sequence = ++next_sequence_;
  }
  last_enqueued_sequence_ = copy.sequence;
  queue_.push_front(std::move(copy));
}

bool EventQueue::pop(InternalEvent &out) {
  if (queue_.empty()) {
    return false;
  }
  out = queue_.front();
  queue_.pop_front();
  return true;
}

} // namespace cgfx

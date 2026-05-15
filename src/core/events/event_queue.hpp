#pragma once

#include "core/events/internal_event.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>

namespace cgfx {

enum class EventQueueOverflowPolicy {
  DropNewest = 0,
  DropOldest = 1,
};

/** FIFO typed event queue with optional resize coalescing and overflow accounting. */
class EventQueue final {
public:
  static constexpr size_t kDefaultMaxDepth = 4096;

  EventQueue() = default;

  void set_max_depth(size_t max_events) noexcept;
  size_t max_depth() const noexcept { return max_depth_; }

  void set_overflow_policy(EventQueueOverflowPolicy p) noexcept {
    overflow_policy_ = p;
  }
  EventQueueOverflowPolicy overflow_policy() const noexcept {
    return overflow_policy_;
  }

  void set_coalesce_resize(bool v) noexcept { coalesce_resize_ = v; }
  bool coalesce_resize() const noexcept { return coalesce_resize_; }

  void push(InternalEvent ev);
  /** Re-queue an event at the front (used when the user buffer is undersized).
   *  Does not apply max-depth/eviction rules so the dequeue can retry safely. */
  void push_priority_front(const InternalEvent &ev);

  bool pop(InternalEvent &out);

  bool empty() const noexcept { return queue_.empty(); }
  size_t size() const noexcept { return queue_.size(); }

  uint64_t dropped_event_count() const noexcept { return dropped_events_; }
  void reset_drop_count() noexcept { dropped_events_ = 0; }

  size_t resize_coalesce_commit_count() const noexcept {
    return resize_coalesce_hits_;
  }
  void reset_resize_coalesce_stats() noexcept { resize_coalesce_hits_ = 0; }

private:
  bool try_resize_coalesce_in_place_(const InternalEvent &incoming);

  void discard_oldest_();
  bool make_room_tail_(); // Returns true if a slot exists / was freed for push_back

  std::deque<InternalEvent> queue_{};
  size_t max_depth_{kDefaultMaxDepth};
  EventQueueOverflowPolicy overflow_policy_{EventQueueOverflowPolicy::DropOldest};
  bool coalesce_resize_{true};

  uint64_t dropped_events_{0};
  uint64_t resize_coalesce_hits_{0};
};

} // namespace cgfx

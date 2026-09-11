#pragma once
#include <atomic>
#include <array>
#include <cstddef>

template <typename T, size_t Capacity>
class SPSCRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

public:
    SPSCRingBuffer() : head_(0), tail_(0) {}

    bool push(const T& item) {
        const auto current_tail = tail_.load(std::memory_order_relaxed);
        if (current_tail - head_.load(std::memory_order_acquire) == Capacity) {
            return false;
        }
        buffer_[current_tail & kMask] = item;
        tail_.store(current_tail + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        const auto current_head = head_.load(std::memory_order_relaxed);
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        item = buffer_[current_head & kMask];
        head_.store(current_head + 1, std::memory_order_release);
        return true;
    }

private:
    static constexpr size_t kMask = Capacity - 1;
    alignas(64) std::array<T, Capacity> buffer_;
    
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
};
#ifndef RINGBUFFER_HPP
#define RINGBUFFER_HPP

#include <atomic>
#include <array>
#include <stdexcept>

// Lock-free Single-Producer Single-Consumer (SPSC) ring buffer.
// Uses atomic head/tail indices instead of mutexes to avoid lock contention —
// critical for ultra-low-latency HFT pipelines where every microsecond counts.
// The internal array is Size+1 to distinguish "full" from "empty" states
// without needing an extra counter variable.
template <typename T, size_t Size>
class RingBuffer {
public:
    // Push an item into the buffer (called by the producer thread).
    // Returns false without blocking if the buffer is full — non-blocking
    // behaviour is intentional: we never stall the UDP receive path.
    bool push(const T& item) {
        // Cache the current head once to avoid a second atomic load below.
        // FIX: Previously head was loaded twice (relaxed), which introduced a
        // subtle race — the index used for the full-check and the write slot
        // could theoretically diverge on a weak memory model.
        size_t current_head = head.load(std::memory_order_relaxed);
        size_t next = (current_head + 1) % (Size + 1); // wrap-around index

        // Acquire fence on tail ensures we see the latest consumer position
        // before deciding whether the buffer is full.
        if (next == tail.load(std::memory_order_acquire)) {
            return false; // buffer is full — drop the packet rather than block
        }

        buffer[current_head] = item; // write to the slot the producer owns

        // Release fence on head makes the new item visible to the consumer.
        head.store(next, std::memory_order_release);
        return true;
    }

    // Pop an item from the buffer (called by the consumer thread).
    // Returns false without blocking if the buffer is empty.
    bool pop(T& item) {
        size_t current_tail = tail.load(std::memory_order_relaxed);

        // Acquire fence on head ensures we see items written by the producer.
        if (current_tail == head.load(std::memory_order_acquire)) {
            return false; // buffer is empty
        }

        item = buffer[current_tail]; // read from the slot the consumer owns
        size_t next = (current_tail + 1) % (Size + 1); // advance with wrap-around

        // Release fence on tail signals the producer that this slot is free.
        tail.store(next, std::memory_order_release);
        return true;
    }

    // Expose indices — used by unit tests to verify buffer state.
    size_t get_head() const { return head.load(std::memory_order_relaxed); }
    size_t get_tail() const { return tail.load(std::memory_order_relaxed); }

private:
    // Size+1 slots: one slot is always kept empty to distinguish full vs empty.
    // Using std::array (stack-allocated, contiguous) avoids heap allocation and
    // maximises CPU cache line utilisation — key for low-latency access patterns.
    std::array<T, Size + 1> buffer;

    // head = next slot the producer will write into (owned by producer thread).
    // tail = next slot the consumer will read from  (owned by consumer thread).
    // Separate cache lines would be ideal (alignas(64)) to avoid false sharing,
    // but kept simple here for clarity.
    std::atomic<size_t> head{0};
    std::atomic<size_t> tail{0};
};

#endif // RINGBUFFER_HPP

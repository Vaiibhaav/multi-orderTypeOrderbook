#pragma once
#include <vector>
#include <atomic>
#include <cstddef>
#include <stdexcept>
#include <memory>

template <typename T>
class ThreadSafeQueue
{
    struct Slot {
        T item;
        std::atomic<bool> ready{false};
    };

public:
    explicit ThreadSafeQueue(std::size_t capacity = 65536)
    {
        if (!(capacity > 0 && (capacity & (capacity - 1)) == 0)) {
            throw std::invalid_argument("capacity must be power of 2");
        }
        capacity_ = capacity;
        mask_ = capacity - 1;
        buffer_ = std::make_unique<Slot[]>(capacity);
        //This tells make_unique what type of data we want to allocate.
        //Normally, you might see std::make_unique<Slot>() to create exactly one Slot object.
        //By adding the empty brackets [], we are telling C++ to allocate a C-style array of Slots.
        /*Why didn't we just use std::vector<Slot>?
        //Earlier, we tried to use std::vector to hold our slots. However, our Slot struct contains a std::atomic<bool>. By design in C++, std::atomic variables cannot be copied or moved. 
        //When you tell a std::vector to resize itself (e.g., buffer_.resize(capacity)), the vector requires the items inside it to be copyable or movable just in case it needs to shuffle memory around. Because our Slot cannot be copied, the compiler threw a massive error.
        //The Solution: std::unique_ptr<Slot[]> gives us the exact same thing as a vector—a dynamic, contiguous array of memory—but without the strict copying requirements and overhead of the std::vector class. It simply allocates the memory chunk once, default-initializes the atomic booleans inside it, and leaves it alone. It's faster, leaner, and perfect for low-latency code!*/
    }

    bool Push(const T& item)
    {
        std::size_t current_tail = tail_.load(std::memory_order_relaxed);
        while (true) {
            if (shutdown_.load(std::memory_order_relaxed)) return false;
            
            std::size_t current_head = head_.load(std::memory_order_acquire);
            if (current_tail - current_head >= capacity_) {
                // Queue is full, busy wait
                current_tail = tail_.load(std::memory_order_relaxed);
                continue;
            }
            
            // Try to claim this slot using CAS compare and swap
            if (tail_.compare_exchange_weak(current_tail, current_tail + 1, std::memory_order_relaxed)) {
                break;
            }
        }

        Slot& slot = buffer_[current_tail & mask_];
        slot.item = item;
        // Release memory order ensures the item copy is visible before ready=true
        slot.ready.store(true, std::memory_order_release);
        return true;
    }

    bool Pop(T& item)
    {
        std::size_t current_head = head_.load(std::memory_order_relaxed);
        while (true) {
            Slot& slot = buffer_[current_head & mask_];
            if (slot.ready.load(std::memory_order_acquire)) {
                item = slot.item;
                slot.ready.store(false, std::memory_order_relaxed);
                head_.store(current_head + 1, std::memory_order_release);
                return true;
            }
            
            // Slot is not ready yet. 
            // Are we empty or just waiting for a producer to finish writing?
            if (current_head == tail_.load(std::memory_order_acquire)) {
                if (shutdown_.load(std::memory_order_acquire)) {
                    // Double check in case a producer pushed right before shutdown
                    if (current_head == tail_.load(std::memory_order_acquire)) {
                        return false; 
                    }
                }
            }
            // Busy wait (spin)
        }
    }

    std::size_t PopBatch(std::vector<T>& batch, std::size_t maxBatch)
    {
        std::size_t current_head = head_.load(std::memory_order_relaxed);
        std::size_t popped = 0;
        
        while (popped == 0) {
            while (popped < maxBatch) {
                Slot& slot = buffer_[(current_head + popped) & mask_];
                if (slot.ready.load(std::memory_order_acquire)) {
                    batch.push_back(slot.item);
                    slot.ready.store(false, std::memory_order_relaxed);
                    popped++;
                } else {
                    break; // No more items ready at this exact moment
                }
            }
            
            if (popped > 0) {
                head_.store(current_head + popped, std::memory_order_release);
                return popped;
            }
            
            if (current_head == tail_.load(std::memory_order_acquire)) {
                if (shutdown_.load(std::memory_order_acquire)) {
                    if (current_head == tail_.load(std::memory_order_acquire)) {
                        return 0;
                    }
                }
            }
            // Busy wait (spin)
        }
        return popped;
    }

    void Shutdown()
    {
        shutdown_.store(true, std::memory_order_release);
    }

    std::size_t Size() const
    {
        std::size_t t = tail_.load(std::memory_order_acquire);
        std::size_t h = head_.load(std::memory_order_acquire);
        return (t > h) ? (t - h) : 0;
    }

    bool Empty() const
    {
        return Size() == 0;
    }

private:
    std::unique_ptr<Slot[]> buffer_;
    std::size_t capacity_;
    std::size_t mask_;
    
    // Prevent false sharing by aligning to 64 bytes (cache line size)
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
    alignas(64) std::atomic<bool> shutdown_{false};
};
#pragma once
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstddef>
#include <stdexcept>

template <typename T>
class ThreadSafeQueue
{
public:
    // capacity should be a power of two
    // Slow:  index % capacity
    // Fast:  index & (capacity - 1)    // only works when capacity is power of 2
    // Example: capacity = 8 (binary: 1000), capacity-1 = 7 (binary: 0111)
    // 10 % 8 = 2
    // 10 & 7 = 1010 & 0111 = 0010 = 2  ✓
    explicit ThreadSafeQueue(std::size_t capacity = 65536)
    {
        if (!(capacity > 0 && (capacity & (capacity - 1)) == 0)) {
            throw std::invalid_argument("capacity must be power of 2");
        }
        capacity_ = capacity;
        mask_ = capacity - 1;
        buffer_.resize(capacity);
    }

    bool Push(const T& item)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(!shutdown_.load() && count_ >= capacity_){
            notFull_.wait(lock);
        }
        if(shutdown_.load()){ return false; }
        buffer_[tail_ & mask_] = item;
        tail_ = (tail_ + 1) & mask_;
        count_++;
        notEmpty_.notify_one();
        return true;
    }

    bool Pop(T& item)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(!shutdown_.load() && count_ == 0){
            notEmpty_.wait(lock);
        }
        // Only return false if shutdown AND queue is empty (fully drained)
        if(shutdown_.load() && count_ == 0){ return false; }
        item = buffer_[head_ & mask_];
        head_ = (head_ + 1) & mask_;
        count_--;
        notFull_.notify_one();
        return true;
    }

    std::size_t PopBatch(std::vector<T>& batch, std::size_t maxBatch)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(!shutdown_.load() && count_ == 0){
            notEmpty_.wait(lock);
        }
        // Only return 0 if shutdown AND queue is empty (fully drained)
        if(shutdown_.load() && count_ == 0){ return 0; }
        std::size_t popped = 0;
        while(count_ > 0 && popped < maxBatch){
            batch.push_back(buffer_[head_ & mask_]);
            head_ = (head_ + 1) & mask_;
            count_--;
            popped++;
        }
        notFull_.notify_all();
        return popped;
    }

    void Shutdown()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_.store(true);
        notEmpty_.notify_all();
        notFull_.notify_all();
    }

    std::size_t Size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }

    bool Empty() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_ == 0;
    }

private:
    std::vector<T> buffer_;              // The fixed-size ring
    std::size_t capacity_;               // Always power of 2
    std::size_t mask_;                   // capacity_ - 1, for fast modulo
    std::size_t head_ = 0;              // Read position
    std::size_t tail_ = 0;              // Write position
    std::size_t count_ = 0;             // Current number of items
    mutable std::mutex mutex_;
    std::condition_variable notEmpty_;    // Consumer waits on this
    std::condition_variable notFull_;     // Producer waits on this
    std::atomic<bool> shutdown_{false};
};
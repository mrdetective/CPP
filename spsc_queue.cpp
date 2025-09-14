#include <vector>
#include <atomic>
#include <optional>
#include <cassert>
#include <thread>

template <typename T>
class SPSCQueue {
public:
    explicit SPSCQueue(size_t capacity)
        : capacity_(roundUpToPowerOfTwo(capacity)),
          buffer_(capacity_),
          mask_(capacity_ - 1) 
    {
        assert((capacity_ & (capacity_ - 1)) == 0 && "capacity must be power of two");
    }

   
    bool push(const T& item) {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next = (head + 1) & mask_;   

        if (next == tail_.load(std::memory_order_acquire)) {
            return false; 
        }

        buffer_[head] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }

    
    std::optional<T> pop() {
        size_t tail = tail_.load(std::memory_order_relaxed);

        if (tail == head_.load(std::memory_order_acquire)) {
            return std::nullopt; 
        }

        T item = buffer_[tail];
        tail_.store((tail + 1) & mask_, std::memory_order_release);
        return item;
    }

private:
    size_t capacity_;
    std::vector<T> buffer_;
    size_t mask_; 
    std::atomic<size_t> head_{0}; 
    std::atomic<size_t> tail_{0}; 

    static size_t roundUpToPowerOfTwo(size_t x) {
        if (x < 2) return 2;
      
        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        if constexpr (sizeof(size_t) == 8)
            x |= x >> 32;
        return ++x;
    }
};


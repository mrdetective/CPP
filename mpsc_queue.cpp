#include <atomic>
#include <vector>
#include <optional>
#include <cassert>

template<typename T>
class mpsc_queue {
public:
    explicit mpsc_queue(size_t capacity)
        : capacity_(roundUpToPowerOfTwo(capacity)),
          buffer_(capacity_),
          mask_(capacity_ - 1) 
    {}

    bool push(const T& item) {
        size_t head, next_head, tail;
        do {
            head = head_.load(std::memory_order_relaxed);
            tail = tail_.load(std::memory_order_acquire);

            if ((head - tail) >= capacity_) {
                return false; 
            }

            next_head = head + 1;
        } while (!head_.compare_exchange_weak(
                    head, next_head,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed));

        buffer_[head & mask_] = item;
        return true;
    }


    std::optional<T> pop() {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t head = head_.load(std::memory_order_acquire);

        if (tail == head) {
            return std::nullopt;
        }

        T item = buffer_[tail & mask_];
        tail_.store(tail + 1, std::memory_order_release);
        return item;
    }

private:
    const size_t capacity_;
    const size_t mask_;
    std::vector<T> buffer_;
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};

    static size_t roundUpToPowerOfTwo(size_t x) {
        if (x < 2) return 2;
        x--;
        x |= x >> 1; x |= x >> 2; x |= x >> 4;
        x |= x >> 8; x |= x >> 16;
        if constexpr(sizeof(size_t) == 8) x |= x >> 32;
        return ++x;
    }
};

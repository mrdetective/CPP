#include<iostream>
#include<atomic>
#include<vector>
#include<optional>

template<typename T>
class mpsc_queue{
    public:

        mpsc_queue(size_t capacity): capacity_(roundUpToPowerOfTwo(capacity)), buffer_(capacity_), mask_(capacity_ - 1){
            assert((capacity_ & (capacity_ - 1)) == 0 && "capacity must be power of two");
        }
        
        bool push(const T &item){
            
            const size_t head = head_.fetch_add(1, std::memory_order_acq_rel);
            size_t tail = tail_.load(std::memory_order_acquire);
            
            if((head - tail) >= capacity_){
                return false;
            }

            buffer_[head & mask_] = item;
            return true;
        }

        std::optional<T> pop(){
            size_t tail = tail_.load(std::memory_order_acquire);
            
            if(tail == head_.load(std::memory_order_acquire)){
                return std::nullopt;
            }
            
            T item = buffer_[tail & mask_];
            tail_.store(tail + 1, std::memory_order_release);
            return item;
        }
    
    private:
        const size_t capacity_; 
        const size_t mask_;
        std::atomic<size_t> head_{0};
        std::atomic<size_t> tail_{0};
        std::vector<T> buffer_;
        
        static size_t roundUpToPowerOfTwo(size_t x){
            if(x < 2) return 2;

            x--;
            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            x |= x >> 8;
            x |= x >> 16;

            if constexpr(sizeof(size_t) == 8){
                x |= x >> 32;
            }

            return ++x;
        }
        
};
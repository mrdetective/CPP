#include<vector>
#include<atomic>
#include<optional>
#include<cstdint>



template<typename T>
class mpmc_queue{
    mpmc_queue(size_t size): _capacity(roundToPowerOfTwo(size)), _buffer(_capacity), _mask(_capacity - 1){
        for (size_t i = 0; i < _capacity; ++i) {
            _buffer[i].seq.store(i, std::memory_order_relaxed);
        }
    } 

    bool push(const T& item){
        
        Cell *cell;
        size_t pos = _tail.load(std::memory_order_relaxed);

        while(true){
            cell = &_buffer[pos & _mask];
            size_t seq = cell->seq.load(std::memory_order_acquire);

            intptr_t dif = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
            
            if(dif == 0){
                if(_tail.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)){
                    break;
                }
            }else if(dif < 0){
                return false;
            }else{
                pos = _tail.load(std::memory_order_relaxed);
            }
        }

        cell->storage = item;
        cell->seq.store(pos + 1, std::memory_order_relaxed);
        return true; 
    }

    std::optional<T> pop() {
        Cell* cell;
        size_t pos = _head.load(std::memory_order_relaxed);

        while (true) {
            cell = &_buffer[pos & _mask];
            size_t seq = cell->seq.load(std::memory_order_acquire);
            intptr_t dif = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);

            if (dif == 0) {
                if (_head.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (dif < 0) {
                return std::nullopt; 
            } else {
                pos = _head.load(std::memory_order_relaxed);
            }
        }

        T result = std::move(cell->storage);
        cell->seq.store(pos + _capacity, std::memory_order_release);
        return result;
    }

    private:
        struct Cell
        {
            T storage;
            std::atomic<size_t> seq;
        };
        

        size_t _capacity;
        size_t _mask;
        std::vector<Cell> _buffer;
        std::atomic<size_t> _head{0};
        std::atomic<size_t>_tail{0};
        
        
        size_t roundToPowerOfTwo(size_t x){
            if(x <= 2){
                return 2;
            }
            x--;
            x |= x << 1;
            x |= x << 2;
            x |= x << 4;
            x |= x << 8;
            x |= x << 16;

            if(sizeof(x) == 8){
                x |= x << 32;
            }
        
        }
};
// memory_ordering_demo.cpp
//
// A demonstration of memory ordering in C++11 atomics.
//
// This file shows several classic examples:
//   1. Producer/Consumer without fencing
//   2. Producer/Consumer with acquire/release fencing
//   3. Message passing using acquire/release
//   4. Counter increment with relaxed vs sequentially consistent ordering
//
// Note: On x86 (Intel/AMD) you may not see "broken" behavior in cases (1) and (2)
//       because x86 enforces strong memory ordering (TSO). To actually observe
//       weak memory behavior, run on ARM/POWER or use tools like CppMem.
//

#include <iostream>
#include <thread>
#include <atomic>

// --------------------------------------------------
// 1. Producer/Consumer without fencing
// --------------------------------------------------
int data1 = 0;
std::atomic<bool> ready1(false);

void producer_no_fence() {
    data1 = 42;                 // write data
    ready1.store(true);         // signal ready (no ordering guarantees)
}

void consumer_no_fence() {
    while (!ready1.load()) {}   // spin until ready
    std::cout << "[NoFence] Consumer sees data = " << data1 << "\n";
}

// --------------------------------------------------
// 2. Producer/Consumer with acquire/release fencing
// --------------------------------------------------
int data2 = 0;
std::atomic<bool> ready2(false);

void producer_acq_rel() {
    data2 = 42;
    ready2.store(true, std::memory_order_release);  // release fence
}

void consumer_acq_rel() {
    while (!ready2.load(std::memory_order_acquire)) {}
    std::cout << "[AcqRel] Consumer sees data = " << data2 << "\n";
}

// --------------------------------------------------
// 3. Message passing using acquire/release
// --------------------------------------------------
std::atomic<bool> flag(false);
int payload = 0;

void sender() {
    payload = 100;
    flag.store(true, std::memory_order_release);
}

void receiver() {
    while (!flag.load(std::memory_order_acquire)) {}
    std::cout << "[MessagePassing] Receiver got payload = " << payload << "\n";
}

// --------------------------------------------------
// 4. Counter with relaxed vs sequential consistency
// --------------------------------------------------
std::atomic<int> counter{0};

void work_relaxed() {
    for (int i = 0; i < 100000; i++) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
}

void work_seq_cst() {
    for (int i = 0; i < 100000; i++) {
        counter.fetch_add(1, std::memory_order_seq_cst);
    }
}

// --------------------------------------------------
// Main
// --------------------------------------------------
int main() {
    {
        // Example 1: Producer/Consumer without fences
        data1 = 0;
        ready1 = false;
        std::thread t1(producer_no_fence);
        std::thread t2(consumer_no_fence);
        t1.join();
        t2.join();
    }

    {
        // Example 2: Producer/Consumer with acquire/release
        data2 = 0;
        ready2 = false;
        std::thread t1(producer_acq_rel);
        std::thread t2(consumer_acq_rel);
        t1.join();
        t2.join();
    }

    {
        // Example 3: Message passing
        payload = 0;
        flag = false;
        std::thread t1(sender);
        std::thread t2(receiver);
        t1.join();
        t2.join();
    }

    {
        // Example 4: Relaxed vs Sequential Consistency
        counter = 0;
        std::thread t1(work_relaxed);
        std::thread t2(work_relaxed);
        t1.join();
        t2.join();
        std::cout << "[Relaxed] Counter = " << counter.load() << " (expected 200000)\n";

        counter = 0;
        std::thread t3(work_seq_cst);
        std::thread t4(work_seq_cst);
        t3.join();
        t4.join();
        std::cout << "[SeqCst]  Counter = " << counter.load() << " (expected 200000)\n";
    }

    return 0;
}

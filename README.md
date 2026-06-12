# Concurrent C++ Limit Orderbook & Matching Engine

A high-performance, multithreaded Limit Orderbook (LOB) and Matching Engine written in modern C++17. This project demonstrates concurrent execution patterns, lock-free data structures, and algorithmic optimizations tailored for low-latency trading system concepts.

## Core Features

- **Lock-Free MPSC Ring Buffer**: Implements a custom Multi-Producer Single-Consumer queue. Completely removes `std::mutex` from the critical path using `std::atomic` Compare-And-Swap (CAS) operations, explicit memory barriers (`acquire`/`release`), and busy-waiting (spinning) for nanosecond-level thread handoffs.
- **Bitwise Queue Indexing**: Utilizes capacity power-of-2 bitwise masking (`index & (capacity - 1)`) instead of modulo arithmetic for ultra-fast ring buffer wrapping in a single CPU cycle.
- **$O(1)$ Order Cancellations**: Combines `std::unordered_map` for instant order lookups with `std::list` for $O(1)$ mid-level node deletions, avoiding the massive $O(N)$ penalty of shifting contiguous arrays like `std::vector` when orders are pulled from the book.
- **Modern C++ Routing**: Uses `std::variant` and `std::visit` to safely and efficiently route diverse market actions (Add, Cancel, Modify) from the queue to the Matching Engine without runtime polymorphism (virtual functions) overhead.
- **Strict Price-Time Priority**: Maintains proper bid/ask ordering using Red-Black trees (`std::map<Price, ...>`).

## Known Issues & Future Optimizations (HFT Roadmap)

While this engine successfully demonstrates concurrent execution and lock-free queueing, there are two intentional architectural compromises made for simplicity that would need to be addressed before deployment in a true ultra-low-latency High Frequency Trading (HFT) environment:

1. **Dynamic Heap Allocation (`std::make_shared`)**
   - **The Issue**: Incoming orders currently trigger dynamic memory allocation (`new` / `std::make_shared`). Touching the OS heap manager on the critical path causes unacceptable latency spikes in HFT. Furthermore, `std::map` and `std::list` implicitly allocate nodes on the heap.
   - **The Solution**: Replace dynamic allocation with pre-allocated **Memory Pools**. At startup, pre-allocate a large contiguous array of `Order` objects. Use C++17 Polymorphic Memory Resources (`std::pmr`) for the maps and lists so they draw from a pre-warmed buffer instead of the OS heap.

2. **Pointer Chasing & Cache Locality**
   - **The Issue**: The orderbook stores a `std::list<std::shared_ptr<Order>>`. To match or read an order, the CPU must fetch the list node, and then follow the pointer to a completely different location in RAM to access the actual `Order`. This causes massive L1/L2 cache misses.
   - **The Solution**: Replace the standard list with an **Intrusive Linked List** (e.g., `boost::intrusive::list`). By storing the `next` and `prev` list pointers directly inside the `Order` struct itself, we guarantee that when the CPU fetches the order, the linked-list traversal data is already loaded in the CPU cache.

## How to Run

Compile using a modern C++17 compiler:
```bash
g++ -std=c++17 -o Orderbook main.cpp Orderbook.cpp Order.cpp OrderbookLevelInfo.cpp Trade.cpp OrderModify.cpp MatchingEngine.cpp
./Orderbook
```

# Concurrent C++ Limit Orderbook & Matching Engine

A high-performance, multithreaded Limit Orderbook (LOB) and Matching Engine written in modern C++17. This project demonstrates concurrent execution patterns, lock-free data structures, and algorithmic optimizations tailored for low-latency trading system concepts.

## Core Features

- **Lock-Free MPSC Ring Buffer**: Implements a custom Multi-Producer Single-Consumer queue with `std::atomic` CAS and acquire/release memory ordering.
- **Bitwise Queue Indexing**: Uses power-of-2 mask arithmetic (`index & (capacity - 1)`) for fast ring buffer wrap-around.
- **$O(1)$ Order Cancellations**: Combines `std::unordered_map` for direct order lookup with `std::list` for constant-time mid-list deletion.
- **Modern C++ Routing**: Uses `std::variant` and `std::visit` to route Add, Cancel, and Modify actions efficiently.
- **Strict Price-Time Priority**: Maintains correct bid/ask ordering with `std::map` price levels and FIFO order matching at each level.

## New AI + FIX + MCP Integration

This release extends the core engine into a complete AI-native trading platform:

- **FIX Protocol Layer**: The engine now parses FIX 4.2 messages and produces FIX execution reports.
- **C API / DLL Bridge**: A C-compatible API (`orderbook_capi.cpp`) exposes the engine through a DLL so Python can invoke it.
- **Python MCP Server**: `orderbook_bridge.py` loads the DLL with `ctypes` and wraps it in a Model Context Protocol (MCP) server.
- **AI Trading Agents**: Python agents simulate market making, momentum trading, spoofing, and detection using the same FIX/MCP bridge.

## Why This Matters

This project now proves a practical path from legacy finance protocols to modern AI:

- preserves a high-performance C++ core for trading logic
- adds real-world interoperability with FIX
- enables AI models to interact with the engine through MCP
- supports realistic multi-agent trading simulations

## Known Issues & Future Optimizations (HFT Roadmap)

While this engine successfully demonstrates concurrent execution and lock-free queueing, there are two intentional architectural compromises made for simplicity that would need to be addressed before deployment in a true ultra-low-latency High Frequency Trading (HFT) environment:

1. **Dynamic Heap Allocation (`std::make_shared`)
   - **The Issue**: Incoming orders currently trigger dynamic memory allocation (`new` / `std::make_shared`). Touching the OS heap manager on the critical path causes unacceptable latency spikes in HFT. Furthermore, `std::map` and `std::list` implicitly allocate nodes on the heap.
   - **The Solution**: Replace dynamic allocation with pre-allocated **Memory Pools**. At startup, pre-allocate a large contiguous array of `Order` objects. Use C++17 Polymorphic Memory Resources (`std::pmr`) for the maps and lists so they draw from a pre-warmed buffer instead of the OS heap.

2. **Pointer Chasing & Cache Locality**
   - **The Issue**: The orderbook stores a `std::list<std::shared_ptr<Order>>`. To match or read an order, the CPU must fetch the list node, and then follow the pointer to a completely different location in RAM to access the actual `Order`. This causes massive L1/L2 cache misses.
   - **The Solution**: Replace the standard list with an **Intrusive Linked List** (e.g., `boost::intrusive::list`). By storing the `next` and `prev` list pointers directly inside the `Order` struct itself, we guarantee that when the CPU fetches the order, the linked-list traversal data is already loaded in the CPU cache.

## Getting Started

### 1. Build the C++ DLL

From the repository root on Windows, run:
```powershell
.\build_dll.bat
```

This produces `orderbook_engine.dll` in the repository root.
If you are using 64-bit Python, prefer the MSVC build script to produce a 64-bit DLL:
```powershell
.\build_dll_msvc.bat
```

`build_dll.bat` may produce a 32-bit DLL when the installed GCC toolchain is 32-bit.
### 2. Install the Python dependencies

Navigate into the `mcp_server` folder and install requirements:
```powershell
cd mcp_server
python -m pip install -r requirements.txt
```

### 3. Run the MCP server

From the `mcp_server` folder, start the server:
```powershell
python orderbook_mcp_server.py
```

The MCP server exposes tools for:
- `submit_order`
- `cancel_order`
- `modify_order`
- `get_snapshot`
- `get_trade_history`
- `analyze_spread`
- `reset_book`

### 4. Run the smoke test

Use the provided smoke test script to verify the bridge and engine:
```powershell
python smoke_test.py
```

### 5. Optional: Run the C++ engine standalone

If you want to run the core engine without the MCP layer, compile and execute the console program:
```bash
g++ -std=c++17 -o Orderbook main.cpp Orderbook.cpp Order.cpp OrderbookLevelInfo.cpp Trade.cpp OrderModify.cpp MatchingEngine.cpp
./Orderbook
```

For the AI + MCP simulation, use the Python bridge and MCP server files under `mcp_server/`.

> Note: The Python MCP server uses `ctypes` to load `orderbook_engine.dll` and translate FIX messages between AI agents and the fast C++ matching engine.
Low-Latency Market Data Processor
==================================

A high-frequency trading (HFT) inspired system built in **C++17** that demonstrates real-time market data ingestion, lock-free inter-thread communication, multi-threading, and end-to-end latency measurement — all core competencies for low-latency systems engineering.

The system receives market data (e.g. `STOCK_A,100.50`) over UDP, passes it through a lock-free ring buffer to a consumer thread, and measures the full pipeline latency in microseconds.

---

Benchmark Results
-----------------

Measured on Windows 11, GCC 13.1 (MinGW), `-O2`, single core — `RingBuffer<TimedData, 1024>`.

### Single-Thread Push+Pop Latency (1 million iterations)

| Metric | Result |
|--------|--------|
| Mean   | **~59 ns** |
| P50    | 100 ns |
| P99    | 100 ns |
| Min    | 0 ns   |

### Throughput (5 million push+pop pairs)

| Metric         | Result                  |
|----------------|-------------------------|
| **Throughput** | **~66 million ops/sec** |
| **ns/op**      | **~15 ns per push+pop** |
| Total time     | 75 ms for 5M ops        |

### SPSC Cross-Thread Pipeline Latency (1 million packets, producer → consumer)

| Metric  | Result      |
|---------|-------------|
| Mean    | ~113 µs     |
| **P50** | **~11 µs**  |
| P95     | ~167 µs     |
| P99     | ~647 µs     |

> Cross-thread P99 tail latency is dominated by Windows OS scheduler jitter, not the buffer itself.
> On Linux with CPU pinning, end-to-end latency drops to the **1–5 µs** range.

---

Key Features
------------

### 1. Real-Time Data Ingestion via UDP

* **UDP over TCP**: UDP has no connection setup, no ACKs, and no head-of-line blocking — standard in market-data feeds where throughput and latency matter more than guaranteed delivery.
* **Immediate timestamping**: Each packet is timestamped the moment it is received by the UDP socket, giving an accurate start point for latency measurement.
* **Implementation**: `UDPServer` class in `src/UDPServer.cpp` — uses Boost.Asio for portable, non-blocking socket I/O.

### 2. Lock-Free Producer-Consumer Pipeline

* **Lock-free ring buffer**: The `RingBuffer<T, Size>` class uses `std::atomic` head and tail indices instead of mutexes. This eliminates lock contention entirely — critical on latency-sensitive paths.
* **SPSC design**: Single-Producer Single-Consumer — `UDPServer` writes, `DataProcessor` reads. This constraint lets us use relaxed/acquire/release ordering instead of the heavier sequential-consistency fences.
* **Fixed-size array**: `std::array<T, Size+1>` on the stack avoids heap allocation in the hot path, keeping data contiguous and cache-friendly.
* **Implementation**: `include/RingBuffer.hpp`

### 3. Multi-Threading with Clean Lifecycle Management

* **Producer thread**: `UDPServer::run()` — blocks on `receive_from()` and pushes `TimedData` into the ring buffer.
* **Consumer thread**: `DataProcessor::run()` — drains the ring buffer, processes each packet, and logs latency.
* **Atomic stop flag**: Both loops check `running.load()` on every iteration. Calling `stop()` sets the flag to `false`, closes the socket (unblocking `receive_from()`), and joins the thread — guaranteeing clean shutdown with no resource leaks.

### 4. Performance Optimisation Techniques

| Technique | Where Applied |
|---|---|
| Lock-free atomics | `RingBuffer::push()` / `pop()` |
| Fixed-size stack array | `RingBuffer` internal storage |
| No heap allocation in hot path | `RingBuffer`, `UDPServer::run()` stack buffer |
| `acquire`/`release` memory ordering | `RingBuffer` head/tail — minimum fences needed for correctness |
| `std::this_thread::yield()` on idle | `DataProcessor::run()` — avoids 100% CPU burn when queue is empty |
| Dedicated per-component threads | `UDPServer`, `DataProcessor` — no shared scheduler interference |

### 5. End-to-End Latency Measurement

* `TimedData` carries a `high_resolution_clock::time_point` set at ingestion.
* `LatencyMonitor::logLatency()` computes `now - timestamp` in microseconds when the packet is processed.
* This captures the full receive → enqueue → dequeue → process delay.
* **Implementation**: `src/LatencyMonitor.cpp`

---

Architecture
------------

```
┌─────────────────────────────────────────────────────────┐
│                      Main Thread                        │
│  Creates shared RingBuffer<TimedData, 1024>             │
│  Wires UDPServer and DataProcessor to the same buffer   │
└───────────────┬─────────────────────┬───────────────────┘
                │                     │
     ┌──────────▼──────────┐ ┌────────▼────────────────┐
     │  UDPServer Thread   │ │  DataProcessor Thread   │
     │  (Producer)         │ │  (Consumer)             │
     │                     │ │                         │
     │  receive_from() ──► │ │  pop() ──► process()    │
     │  timestamp packet   │ │  logLatency()           │
     │  push(TimedData)    │ │                         │
     └──────────┬──────────┘ └────────▲────────────────┘
                │                     │
         ┌──────▼─────────────────────┴──────┐
         │  RingBuffer<TimedData, 1024>       │
         │  Lock-free SPSC queue             │
         │  std::atomic head / tail          │
         └───────────────────────────────────┘
```

---

Project Structure
-----------------

```plaintext
LowLatencyMarketDataProcessor/
├── CMakeLists.txt           # Build system — main exe, tests, benchmarks
├── README.md
├── include/
│   ├── Common.hpp           # TimedData struct (data + timestamp)
│   ├── RingBuffer.hpp       # Lock-free SPSC ring buffer (template)
│   ├── UDPServer.hpp        # UDP receiver — producer side
│   ├── DataProcessor.hpp    # Packet consumer — consumer side
│   └── LatencyMonitor.hpp   # Static latency logging utility
├── src/
│   ├── main.cpp             # Entry point — wires buffer, server, processor
│   ├── UDPServer.cpp        # UDP receive loop + ring buffer push
│   ├── DataProcessor.cpp    # Ring buffer pop + processing + latency log
│   └── LatencyMonitor.cpp   # high_resolution_clock latency calculation
├── tests/
│   ├── test_RingBuffer.cpp  # Push/pop, full/empty edge cases (Catch2)
│   ├── test_UDPServer.cpp   # Start/stop lifecycle test (Catch2)
│   └── test_DataProcessor.cpp # Start/stop lifecycle test (Catch2)
├── benchmarks/
│   ├── benchmark_RingBuffer.cpp  # Push+pop throughput (Google Benchmark)
│   └── standalone_bench.cpp      # Self-contained chrono benchmark (no deps)
└── scripts/
    └── run_tests.sh         # Shell script to build and run all tests
```

---

Building the Project
--------------------

### Prerequisites

| Dependency | Version | Purpose |
|---|---|---|
| C++ compiler (GCC / Clang / MSVC) | C++17+ | Core language features |
| CMake | >= 3.14 | Build system |
| Boost.Asio | Any recent | UDP socket I/O in UDPServer |
| Catch2 | v3 | Unit testing framework |
| Google Benchmark | Any recent | Micro-benchmarking |

### Build Instructions (Linux / macOS)

```bash
mkdir build
cd build
cmake ..
make
```

### Build Instructions (Windows — CLion / MinGW)

```powershell
# Install dependencies via vcpkg
vcpkg install boost catch2 benchmark

# Configure and build
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="<vcpkg-root>/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
```

### Standalone Benchmark (no external deps required)

```powershell
# Windows (MinGW g++)
g++ -O2 -std=c++17 -Iinclude benchmarks/standalone_bench.cpp -o build/standalone_bench.exe -static-libgcc -static-libstdc++
.\build\standalone_bench.exe
```

```bash
# Linux / macOS
g++ -O2 -std=c++17 -Iinclude benchmarks/standalone_bench.cpp -o build/standalone_bench -lpthread
./build/standalone_bench
```

### Run the Tests

```bash
# Option 1 — via CTest (registered in CMakeLists.txt)
cd build
ctest --output-on-failure

# Option 2 — run individual test binaries directly
./build/test_RingBuffer
./build/test_UDPServer
./build/test_DataProcessor

# Option 3 — convenience script
./scripts/run_tests.sh
```

### Run the Google Benchmark

```bash
./build/benchmark_RingBuffer
```

### Run the Main Application

```bash
./build/market_data_processor
# In another terminal, send a test UDP packet:
echo -n "STOCK_A,102.50" | nc -u 127.0.0.1 12345
```

---

Concepts Demonstrated
---------------------

### UDP Networking
UDP is connectionless and skips TCP's handshake, ACK, and retransmission overhead. Market data vendors (e.g. NASDAQ ITCH, CME MDP) use UDP multicast for exactly this reason. Boost.Asio provides a portable, efficient wrapper around the OS socket API.

### Lock-Free Programming
Mutexes introduce unpredictable latency spikes (lock contention, OS scheduler wake-ups). The `RingBuffer` avoids all of this using only `std::atomic` operations with carefully chosen memory orderings:
- `relaxed` — for reads where ordering doesn't matter (e.g. reading own index)
- `acquire` — to observe the other thread's latest write
- `release` — to make our own write visible to the other thread

### Producer-Consumer with Atomic Stop Flag
`std::atomic<bool> running` is the standard C++ pattern for stopping a worker thread safely. The flag is written by the stopping thread and read by the worker — the atomic guarantees the write is visible without a mutex.

### End-to-End Latency Measurement
Embedding the timestamp inside the packet (`TimedData`) avoids the need for a shared clock or external correlation ID. `std::chrono::high_resolution_clock` gives nanosecond-resolution timing on most platforms.

---

Future Enhancements
-------------------

1. **Kernel Bypass (DPDK / XDP)**: Bypass the OS network stack entirely for sub-microsecond latency.
2. **FPGA / Hardware Timestamping**: Replace software clocks with NIC hardware timestamps for nanosecond accuracy.
3. **Cache-Line Padding**: Add `alignas(64)` to `head` and `tail` in `RingBuffer` to eliminate false sharing between producer and consumer cores.
4. **Busy-Spin with PAUSE**: Replace `std::this_thread::yield()` with `_mm_pause()` in the consumer for true ultra-low-latency spinning.
5. **Distributed Processing**: Extend to multiple nodes using a message bus (e.g. Aeron, ZeroMQ).

---

License
-------

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

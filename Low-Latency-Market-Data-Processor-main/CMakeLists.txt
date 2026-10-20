cmake_minimum_required(VERSION 3.14)
project(LowLatencyMarketDataProcessor)

# Enforce C++17 — required for std::atomic, structured bindings, and if-constexpr
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# All headers live in include/ — added to every target's include path
include_directories(include)

# Boost.Asio is used for cross-platform UDP socket I/O in UDPServer
find_package(Boost REQUIRED COMPONENTS system)

# Catch2 is the unit-testing framework (header-only v3, provides its own main)
find_package(Catch2 REQUIRED)

# Google Benchmark is used for micro-benchmarking the RingBuffer hot path
find_package(benchmark REQUIRED)

# ── Main executable ───────────────────────────────────────────────────────────
add_executable(market_data_processor
    src/main.cpp
    src/UDPServer.cpp
    src/DataProcessor.cpp
    src/LatencyMonitor.cpp
)
target_link_libraries(market_data_processor Boost::system)

# ── Unit tests ────────────────────────────────────────────────────────────────
enable_testing() # activates CTest so `ctest` can discover and run all tests

# RingBuffer test — tests lock-free push/pop logic, full/empty edge cases
add_executable(test_RingBuffer tests/test_RingBuffer.cpp)
target_link_libraries(test_RingBuffer Catch2::Catch2WithMain)
# FIX: removed duplicate Catch2::Catch2 — only WithMain is needed and linking
# both caused symbol conflicts in some toolchains
add_test(NAME test_RingBuffer COMMAND test_RingBuffer) # register with CTest

# UDPServer test — tests start/stop lifecycle and clean thread shutdown
add_executable(test_UDPServer tests/test_UDPServer.cpp src/UDPServer.cpp)
target_link_libraries(test_UDPServer Catch2::Catch2WithMain Boost::system)
add_test(NAME test_UDPServer COMMAND test_UDPServer) # register with CTest

# DataProcessor test — tests start/stop lifecycle and consumer thread join
add_executable(test_DataProcessor tests/test_DataProcessor.cpp src/DataProcessor.cpp src/LatencyMonitor.cpp)
target_link_libraries(test_DataProcessor Catch2::Catch2WithMain)
add_test(NAME test_DataProcessor COMMAND test_DataProcessor) # register with CTest

# ── Benchmark ─────────────────────────────────────────────────────────────────
# Benchmarks are separate from tests — run manually, not via ctest
add_executable(benchmark_RingBuffer benchmarks/benchmark_RingBuffer.cpp)
target_link_libraries(benchmark_RingBuffer benchmark::benchmark)
#include "RingBuffer.hpp"
#include "Common.hpp"
#include <benchmark/benchmark.h>

// BM_RingBufferPushPop — measures the throughput of a single push+pop cycle
// on the lock-free ring buffer. This is the hot path in the HFT pipeline:
// every received market-data packet does exactly one push (producer) and one
// pop (consumer). Google Benchmark runs this in a tight loop and reports
// iterations/second and time-per-iteration so we can verify the buffer stays
// in the nanosecond range.
static void BM_RingBufferPushPop(benchmark::State& state) {
    RingBuffer<TimedData, 1024> buffer;
    TimedData item{"test", std::chrono::high_resolution_clock::now()};

    // state is the Google Benchmark loop driver — it handles warmup, timing
    // windows, and result aggregation automatically.
    for (auto _ : state) {
        buffer.push(item); // producer step
        buffer.pop(item);  // consumer step
    }
}

// Register the benchmark and generate its main() entry point.
BENCHMARK(BM_RingBufferPushPop);
BENCHMARK_MAIN();
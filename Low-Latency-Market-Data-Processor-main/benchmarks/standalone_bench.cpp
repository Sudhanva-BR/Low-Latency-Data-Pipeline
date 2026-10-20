// Standalone benchmark — no external libraries required.
// Measures RingBuffer push+pop latency and throughput using std::chrono.
#include "../include/RingBuffer.hpp"
#include "../include/Common.hpp"
#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>
#include <algorithm>
#include <thread>
#include <cstring>

using Clock = std::chrono::high_resolution_clock;
using ns    = std::chrono::nanoseconds;

// ── 1. Single-thread push+pop latency (hot path) ─────────────────────────────
void bench_single_thread_latency() {
    constexpr int WARMUP    = 100'000;
    constexpr int ITERS     = 1'000'000;

    RingBuffer<TimedData, 1024> buf;
    TimedData item{"STOCK_A,100.50", Clock::now()};
    TimedData out;

    // warmup
    for (int i = 0; i < WARMUP; ++i) { buf.push(item); buf.pop(out); }

    std::vector<long long> samples;
    samples.reserve(ITERS);

    for (int i = 0; i < ITERS; ++i) {
        auto t0 = Clock::now();
        buf.push(item);
        buf.pop(out);
        auto t1 = Clock::now();
        samples.push_back(std::chrono::duration_cast<ns>(t1 - t0).count());
    }

    std::sort(samples.begin(), samples.end());
    double avg = (double)std::accumulate(samples.begin(), samples.end(), 0LL) / ITERS;
    long long p50  = samples[ITERS * 50  / 100];
    long long p95  = samples[ITERS * 95  / 100];
    long long p99  = samples[ITERS * 99  / 100];
    long long p999 = samples[ITERS * 999 / 1000];

    std::cout << "\n=== Single-Thread Push+Pop Latency (1M iterations) ===\n";
    std::cout << "  Mean  : " << avg   << " ns\n";
    std::cout << "  P50   : " << p50   << " ns\n";
    std::cout << "  P95   : " << p95   << " ns\n";
    std::cout << "  P99   : " << p99   << " ns\n";
    std::cout << "  P99.9 : " << p999  << " ns\n";
    std::cout << "  Min   : " << samples.front() << " ns\n";
    std::cout << "  Max   : " << samples.back()  << " ns\n";
}

// ── 2. Throughput — ops/second ────────────────────────────────────────────────
void bench_throughput() {
    constexpr int ITERS = 5'000'000;

    RingBuffer<TimedData, 1024> buf;
    TimedData item{"STOCK_A,100.50", Clock::now()};
    TimedData out;

    // warmup
    for (int i = 0; i < 100'000; ++i) { buf.push(item); buf.pop(out); }

    auto t0 = Clock::now();
    for (int i = 0; i < ITERS; ++i) {
        buf.push(item);
        buf.pop(out);
    }
    auto t1 = Clock::now();

    double elapsed_s = std::chrono::duration<double>(t1 - t0).count();
    double ops_per_sec = ITERS / elapsed_s;

    std::cout << "\n=== Throughput (5M push+pop pairs) ===\n";
    std::cout << "  Total time : " << (long long)(elapsed_s * 1000) << " ms\n";
    std::cout << "  Throughput : " << (long long)(ops_per_sec / 1e6) << "M ops/sec\n";
    std::cout << "  ns/op      : " << (elapsed_s * 1e9 / ITERS) << " ns\n";
}

// ── 3. SPSC producer-consumer across threads ──────────────────────────────────
void bench_spsc_threads() {
    constexpr int ITERS = 1'000'000;

    RingBuffer<TimedData, 1024> buf;
    std::vector<long long> latencies(ITERS);
    int received = 0;

    // Consumer thread: measures round-trip from push timestamp to pop time
    std::thread consumer([&]() {
        TimedData out;
        int count = 0;
        while (count < ITERS) {
            if (buf.pop(out)) {
                auto now = Clock::now();
                latencies[count++] = std::chrono::duration_cast<ns>(now - out.timestamp).count();
            }
        }
        received = count;
    });

    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < ITERS; ++i) {
            TimedData item{"STOCK_A,100.50", Clock::now()};
            while (!buf.push(item)) { /* spin if full */ }
        }
    });

    producer.join();
    consumer.join();

    std::sort(latencies.begin(), latencies.end());
    double avg = (double)std::accumulate(latencies.begin(), latencies.end(), 0LL) / received;
    long long p50  = latencies[received * 50  / 100];
    long long p95  = latencies[received * 95  / 100];
    long long p99  = latencies[received * 99  / 100];
    long long p999 = latencies[received * 999 / 1000];

    std::cout << "\n=== SPSC Cross-Thread Pipeline Latency (1M packets) ===\n";
    std::cout << "  Mean  : " << (long long)avg << " ns\n";
    std::cout << "  P50   : " << p50   << " ns\n";
    std::cout << "  P95   : " << p95   << " ns\n";
    std::cout << "  P99   : " << p99   << " ns\n";
    std::cout << "  P99.9 : " << p999  << " ns\n";
    std::cout << "  Min   : " << latencies.front() << " ns\n";
    std::cout << "  Max   : " << latencies.back()  << " ns\n";
}

int main() {
    std::cout << "Low-Latency Market Data Processor — RingBuffer Benchmark\n";
    std::cout << std::string(57, '=') << "\n";

    bench_single_thread_latency();
    bench_throughput();
    bench_spsc_threads();

    std::cout << "\nDone.\n";
    return 0;
}

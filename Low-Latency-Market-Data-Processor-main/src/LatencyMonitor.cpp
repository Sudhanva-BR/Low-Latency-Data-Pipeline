#include "LatencyMonitor.hpp"
#include <iostream>

// logLatency() — calculates and prints the end-to-end latency for a single packet.
// "End-to-end" means the time from when UDPServer timestamped the packet on receipt
// to the moment DataProcessor calls this function after popping it from the buffer.
// high_resolution_clock is used because it gives the finest tick resolution
// available on the platform (nanosecond precision on most modern systems).
void LatencyMonitor::logLatency(const TimedData& data) {
    auto now = std::chrono::high_resolution_clock::now();

    // duration_cast to microseconds gives a practical unit for intra-process
    // latency — nanoseconds would be noisy, milliseconds would lose detail.
    auto latency = std::chrono::duration_cast<std::chrono::microseconds>(now - data.timestamp).count();

    std::cout << "[LatencyMonitor] Latency: " << latency << " microseconds" << std::endl;
}
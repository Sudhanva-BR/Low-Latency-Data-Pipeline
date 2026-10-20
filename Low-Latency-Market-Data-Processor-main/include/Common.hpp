#ifndef COMMON_HPP
#define COMMON_HPP

#include <chrono>
#include <string>

// TimedData — the core packet type flowing through the pipeline.
// Pairing the raw data with a timestamp at the point of ingestion lets
// LatencyMonitor compute accurate end-to-end latency later without needing
// a separate timestamp lookup or shared clock structure.
struct TimedData {
    std::string data;                                        // the received market data payload (e.g. "STOCK_A,102.50")
    std::chrono::high_resolution_clock::time_point timestamp; // when UDPServer received this packet
};

#endif // COMMON_HPP
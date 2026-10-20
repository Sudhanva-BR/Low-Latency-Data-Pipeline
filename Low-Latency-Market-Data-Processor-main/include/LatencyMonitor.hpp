#ifndef LATENCYMONITOR_HPP
#define LATENCYMONITOR_HPP

#include <chrono>
#include <string>
#include "Common.hpp" // TimedData — carries the ingestion timestamp

// LatencyMonitor — measures and logs the end-to-end latency for each packet.
// Implemented as a static utility class (no state) so it can be called from
// any thread without synchronisation overhead.
class LatencyMonitor {
public:
    // Calculates elapsed time from data.timestamp (set at ingestion) to now
    // (called at processing time) and prints it in microseconds.
    static void logLatency(const TimedData& data);
};

#endif // LATENCYMONITOR_HPP
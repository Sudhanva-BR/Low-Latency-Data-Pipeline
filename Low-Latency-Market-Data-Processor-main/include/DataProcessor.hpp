#ifndef DATAPROCESSOR_HPP
#define DATAPROCESSOR_HPP

#include "RingBuffer.hpp"    // lock-free queue to read packets from
#include "LatencyMonitor.hpp" // used inside run() to log per-packet latency
#include <string>
#include <thread>
#include <atomic>

// DataProcessor — the consumer side of the producer-consumer pipeline.
// Runs on its own thread, continuously draining the shared ring buffer and
// processing each TimedData packet (parse, log latency, etc.).
class DataProcessor {
public:
    // Takes a reference to the shared buffer — same object that UDPServer writes into.
    DataProcessor(RingBuffer<TimedData, 1024>& buffer);
    ~DataProcessor();

    void start(); // spawn the consumer thread
    void stop();  // signal shutdown and join the thread

private:
    void run();   // main consumer loop — runs on processor_thread

    RingBuffer<TimedData, 1024>& buffer; // reference to the shared lock-free queue
    std::thread processor_thread;        // dedicated consumer thread
    std::atomic<bool> running{false};    // atomic flag used to stop the loop safely
};

#endif // DATAPROCESSOR_HPP
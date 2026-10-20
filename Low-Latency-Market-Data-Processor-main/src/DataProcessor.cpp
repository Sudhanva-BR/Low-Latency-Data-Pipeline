#include "DataProcessor.hpp"
#include "LatencyMonitor.hpp"
#include <iostream>
#include <thread> // std::this_thread::yield

// Constructor: stores a reference to the shared ring buffer.
// The reference is intentional — DataProcessor does not own the buffer;
// UDPServer and DataProcessor both share the same buffer created in main().
DataProcessor::DataProcessor(RingBuffer<TimedData, 1024>& buffer)
    : buffer(buffer) {}

DataProcessor::~DataProcessor() {
    stop(); // ensure the thread is joined before the object is destroyed
}

// start() — spawns the consumer thread.
void DataProcessor::start() {
    running.store(true);
    processor_thread = std::thread(&DataProcessor::run, this);
}

// stop() — signals the consumer loop to exit and waits for the thread to finish.
void DataProcessor::stop() {
    running.store(false);
    if (processor_thread.joinable()) {
        processor_thread.join();
    }
}

// run() — the main consumer loop executed on processor_thread.
// Continuously polls the ring buffer and processes each TimedData packet.
void DataProcessor::run() {
    TimedData item;
    while (running.load()) {
        if (buffer.pop(item)) {
            // A packet is available — process it.
            // In a real HFT system this is where order-book updates, price
            // calculations, or signal generation would happen.
            std::cout << "[DataProcessor] Processed: " << item.data << std::endl;

            // Measure and log the end-to-end latency for this packet.
            // item.timestamp was set by UDPServer the moment the packet arrived,
            // so this captures the full receive → process pipeline delay.
            LatencyMonitor::logLatency(item);
        } else {
            // FIX: yield when the buffer is empty instead of spinning at 100% CPU.
            // yield() hints to the OS scheduler to give this CPU slice to another
            // thread. This keeps the consumer responsive without wasting power.
            // In a true ultra-low-latency deployment you would busy-spin with
            // _mm_pause() (x86 PAUSE instruction) instead, but yield() is the
            // portable and safe choice for a general demo.
            std::this_thread::yield();
        }
    }
}
#include "UDPServer.hpp"
#include "DataProcessor.hpp"
#include "RingBuffer.hpp"
#include <iostream>

int main() {
    // Shared lock-free ring buffer — the single point of data exchange between
    // the UDP receive thread (producer) and the processor thread (consumer).
    // Capacity 1024 means up to 1024 packets can be queued without dropping.
    RingBuffer<TimedData, 1024> buffer;

    // Create the UDP server and pass the shared buffer so received packets
    // flow directly into the pipeline.
    // FIX: previously the buffer was NOT passed here, so the pipeline was broken
    // — UDPServer and DataProcessor were completely disconnected.
    UDPServer server("127.0.0.1", 12345, buffer);
    server.start(); // spawns the receive thread

    // Create the data processor — it reads from the same shared buffer and
    // measures end-to-end latency for every packet it processes.
    DataProcessor processor(buffer);
    processor.start(); // spawns the consumer thread

    // Keep the main thread alive until the user decides to stop.
    // In a production system this would be a signal handler (SIGINT/SIGTERM).
    std::cout << "Market data processor running. Press Enter to exit..." << std::endl;
    std::cin.get();

    // Graceful shutdown: stop producer first so no new data enters the buffer,
    // then stop the consumer after it drains remaining items.
    server.stop();
    processor.stop();

    return 0;
}
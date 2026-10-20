#include "UDPServer.hpp"
#include <iostream>
#include <chrono>

// Constructor: binds the UDP socket to the given ip:port and stores a reference
// to the shared ring buffer so the receive loop can push packets into it.
UDPServer::UDPServer(const std::string& ip, int port, RingBuffer<TimedData, 1024>& buffer)
    : socket(io, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port)),
      server_endpoint(boost::asio::ip::make_address(ip), port),
      ringBuffer(buffer) {} // FIX: store the ring buffer reference — was missing before

UDPServer::~UDPServer() {
    stop(); // ensure the thread is joined before the object is destroyed
}

// start() — spawns a dedicated receive thread.
// Running on a separate thread means the UDP receive loop never blocks main().
void UDPServer::start() {
    running.store(true);
    server_thread = std::thread(&UDPServer::run, this);
}

// stop() — signals the loop to exit, closes the socket to unblock
// receive_from(), then waits for the thread to finish cleanly.
void UDPServer::stop() {
    running.store(false);
    // Closing the socket causes receive_from() to return immediately with an
    // operation_aborted error, which lets the thread exit without waiting for
    // the next packet to arrive.
    socket.close();
    if (server_thread.joinable()) {
        server_thread.join(); // block until the thread has fully exited
    }
}

// run() — the main receive loop executed on server_thread.
// Each received UDP datagram is timestamped immediately and pushed into the
// lock-free ring buffer for the DataProcessor thread to consume.
void UDPServer::run() {
    char buf[1024]; // fixed-size stack buffer — no heap allocation in the hot path
    while (running.load()) {
        boost::system::error_code error;

        // Blocking call — waits until a UDP packet arrives or the socket closes.
        // remote_endpoint is filled with the sender's address by Boost.Asio.
        std::size_t len = socket.receive_from(boost::asio::buffer(buf), remote_endpoint, 0, error);

        // FIX: handle operation_aborted explicitly so stop() causes a clean exit
        // rather than logging a spurious error every time the server is shut down.
        if (error == boost::asio::error::operation_aborted) {
            break; // socket was closed by stop() — exit the loop cleanly
        }

        if (!error) {
            // Timestamp the packet as early as possible after receipt to get an
            // accurate start point for end-to-end latency measurement.
            TimedData td;
            td.data      = std::string(buf, len);
            td.timestamp = std::chrono::high_resolution_clock::now();

            // FIX: push into the shared ring buffer so DataProcessor can consume it.
            // Previously the data was only printed here and never reached the buffer.
            if (!ringBuffer.push(td)) {
                // Buffer full — packet dropped. In a real HFT system you would
                // record this as a missed-packet metric. For this demo we log it.
                std::cerr << "[UDPServer] Ring buffer full — packet dropped." << std::endl;
            }

            std::cout << "[UDPServer] Received: " << td.data << std::endl;
        } else {
            std::cerr << "[UDPServer] Receive error: " << error.message() << std::endl;
        }
    }
}

// sendData() — utility method to send a UDP message to a given destination.
// Used mainly by tests and the demo to simulate incoming market-data packets.
void UDPServer::sendData(const std::string& message, const std::string& dest_ip, int dest_port) {
    boost::system::error_code error;
    boost::asio::ip::udp::endpoint dest_endpoint(boost::asio::ip::make_address(dest_ip), dest_port);

    socket.send_to(boost::asio::buffer(message), dest_endpoint, 0, error);

    if (error) {
        std::cerr << "[UDPServer] Error sending data: " << error.message() << std::endl;
    }
}

#ifndef UDPSERVER_HPP
#define UDPSERVER_HPP

#include <boost/asio.hpp>
#include <string>
#include <thread>
#include <atomic>
#include "RingBuffer.hpp" // lock-free queue shared with DataProcessor
#include "Common.hpp"     // TimedData — the packet type pushed into the buffer

// UDPServer listens on a UDP socket and acts as the producer in the
// producer-consumer pipeline. UDP is chosen over TCP because it has no
// connection overhead, no ACK round-trips, and no head-of-line blocking —
// all essential for market-data feeds where throughput and latency beat
// delivery guarantees.
class UDPServer {
public:
    // Constructor takes the shared ring buffer so received packets can be
    // pushed directly into the pipeline without any intermediate copy or lock.
    UDPServer(const std::string& ip, int port, RingBuffer<TimedData, 1024>& buffer);
    ~UDPServer();

    void start(); // spawn the receive thread
    void stop();  // signal shutdown and join the thread
    void sendData(const std::string& message, const std::string& dest_ip, int dest_port);

private:
    void run();   // main receive loop — runs on server_thread

    boost::asio::io_context io;                       // Boost.Asio event loop (used for socket I/O)
    boost::asio::ip::udp::socket socket;              // UDP socket bound to ip:port
    boost::asio::ip::udp::endpoint server_endpoint;   // local endpoint (our address)
    boost::asio::ip::udp::endpoint remote_endpoint;   // filled by receive_from with sender's address
    std::thread server_thread;                        // dedicated receive thread
    std::atomic<bool> running{false};                 // flag to signal the loop to stop
    RingBuffer<TimedData, 1024>& ringBuffer;          // reference to the shared lock-free queue
};

#endif // UDPSERVER_HPP

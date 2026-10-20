#include <catch2/catch_all.hpp>

#include "UDPServer.hpp"
#include "RingBuffer.hpp"
#include "Common.hpp"

TEST_CASE("UDPServer start and stop", "[UDPServer]") {

    SECTION("Start and stop server") {
        // FIX: UDPServer now requires a RingBuffer reference — create one here.
        // Previously the constructor took only ip and port, so this test compiled.
        // After the pipeline fix (UDPServer needs to push into the buffer),
        // the constructor signature changed and the test must provide a buffer.
        RingBuffer<TimedData, 1024> buffer;
        UDPServer server("127.0.0.1", 12345, buffer);

        server.start();
        // Verifies that stop() joins cleanly without throwing — tests the
        // socket-close + thread-join shutdown sequence.
        REQUIRE_NOTHROW(server.stop());
    }
}
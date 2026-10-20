// FIX: removed #define CATCH_CONFIG_MAIN — the CMake target links against
// Catch2::Catch2WithMain which already provides main(). Defining it here too
// caused a "multiple definition of main" linker error.
#include <catch2/catch_all.hpp>

#include "RingBuffer.hpp"
#include "Common.hpp"

TEST_CASE("RingBuffer push and pop", "[RingBuffer]") {

    SECTION("Push and pop single item") {
        // Verify the basic round-trip: one push followed by one pop returns the same data.
        RingBuffer<TimedData, 1024> buffer;
        TimedData item;

        REQUIRE(buffer.push({"test", std::chrono::high_resolution_clock::now()}));
        REQUIRE(buffer.pop(item));
        REQUIRE(item.data == "test");
    }

    SECTION("Buffer full") {
        // FIX: use a small buffer (capacity 10) so we can fill it in the test.
        // Previously used RingBuffer<TimedData, 1024> but only pushed 10 items,
        // so the 11th push succeeded and the REQUIRE_FALSE assertion failed.
        // Size+1 internal slots means capacity-10 buffer rejects the 11th push.
        RingBuffer<TimedData, 10> smallBuffer;

        for (int i = 0; i < 10; ++i) {
            REQUIRE(smallBuffer.push({"data" + std::to_string(i), std::chrono::high_resolution_clock::now()}));
        }
        // 11th push must fail — buffer is full
        REQUIRE_FALSE(smallBuffer.push({"overflow", std::chrono::high_resolution_clock::now()}));
    }

    SECTION("Buffer empty") {
        // Popping from an empty buffer must return false without blocking.
        RingBuffer<TimedData, 1024> buffer;
        TimedData item;
        REQUIRE_FALSE(buffer.pop(item));
    }
}
#include <catch2/catch_all.hpp>

#include "DataProcessor.hpp"
#include "RingBuffer.hpp"
#include "Common.hpp"

TEST_CASE("DataProcessor start and stop", "[DataProcessor]") {

    SECTION("Start and stop processor") {
        // Create a shared buffer — same setup as in main().
        RingBuffer<TimedData, 1024> buffer;
        DataProcessor processor(buffer);

        processor.start();
        // Verifies that stop() signals the loop and joins the thread cleanly
        // without deadlocking or throwing — covers the atomic flag + join path.
        REQUIRE_NOTHROW(processor.stop());
    }
}
#include <xrpl/beast/clock/basic_seconds_clock.h>

#include <doctest/doctest.h>

using namespace beast;

TEST_SUITE_BEGIN("basic_seconds_clock");

TEST_CASE("basic_seconds_clock::now() works")
{
    // Just verify that now() can be called without throwing
    auto t = basic_seconds_clock::now();
    // Verify that the time point is valid (not zero)
    CHECK(t.time_since_epoch().count() > 0);
}

TEST_SUITE_END();


#include <test/csf/Scheduler.h>

#include <doctest/doctest.h>

#include <set>

using namespace xrpl::test::csf;

TEST_SUITE_BEGIN("Scheduler");

TEST_CASE("Scheduler basic operations")
{
    using namespace std::chrono_literals;
    Scheduler scheduler;
    std::set<int> seen;

    scheduler.in(1s, [&] { seen.insert(1); });
    scheduler.in(2s, [&] { seen.insert(2); });
    auto token = scheduler.in(3s, [&] { seen.insert(3); });
    scheduler.at(scheduler.now() + 4s, [&] { seen.insert(4); });
    scheduler.at(scheduler.now() + 8s, [&] { seen.insert(8); });

    auto start = scheduler.now();

    // Process first event
    CHECK(seen.empty());
    CHECK(scheduler.step_one());
    CHECK(seen == std::set<int>({1}));
    CHECK(scheduler.now() == (start + 1s));

    // No processing if stepping until current time
    CHECK(scheduler.step_until(scheduler.now()));
    CHECK(seen == std::set<int>({1}));
    CHECK(scheduler.now() == (start + 1s));

    // Process next event
    CHECK(scheduler.step_for(1s));
    CHECK(seen == std::set<int>({1, 2}));
    CHECK(scheduler.now() == (start + 2s));

    // Don't process cancelled event, but advance clock
    scheduler.cancel(token);
    CHECK(scheduler.step_for(1s));
    CHECK(seen == std::set<int>({1, 2}));
    CHECK(scheduler.now() == (start + 3s));

    // Process until 3 seen ints
    CHECK(scheduler.step_while([&]() { return seen.size() < 3; }));
    CHECK(seen == std::set<int>({1, 2, 4}));
    CHECK(scheduler.now() == (start + 4s));

    // Process the rest
    CHECK(scheduler.step());
    CHECK(seen == std::set<int>({1, 2, 4, 8}));
    CHECK(scheduler.now() == (start + 8s));

    // Process the rest again doesn't advance
    CHECK(!scheduler.step());
    CHECK(seen == std::set<int>({1, 2, 4, 8}));
    CHECK(scheduler.now() == (start + 8s));
}

TEST_SUITE_END();

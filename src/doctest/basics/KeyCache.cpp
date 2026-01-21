#include <xrpl/basics/TaggedCache.h>
#include <xrpl/basics/TaggedCache.ipp>
#include <xrpl/basics/chrono.h>
#include <xrpl/protocol/Protocol.h>

#include <doctest/doctest.h>

using namespace xrpl;

TEST_SUITE_BEGIN("KeyCache");

TEST_CASE("KeyCache operations")
{
    using namespace std::chrono_literals;
    TestStopwatch clock;
    clock.set(0);

    using Key = std::string;
    using Cache = TaggedCache<Key, int, true>;

    beast::Journal j{beast::Journal::getNullSink()};

    SUBCASE("Insert, retrieve, and age item")
    {
        Cache c("test", LedgerIndex(1), 2s, clock, j);

        CHECK_EQ(c.size(), 0);
        CHECK_UNARY(c.insert("one"));
        CHECK_FALSE(c.insert("one"));
        CHECK_EQ(c.size(), 1);
        CHECK_UNARY(c.touch_if_exists("one"));
        ++clock;
        c.sweep();
        CHECK_EQ(c.size(), 1);
        ++clock;
        c.sweep();
        CHECK_EQ(c.size(), 0);
        CHECK_FALSE(c.touch_if_exists("one"));
    }

    SUBCASE("Insert two items, have one expire")
    {
        Cache c("test", LedgerIndex(2), 2s, clock, j);

        CHECK_UNARY(c.insert("one"));
        CHECK_EQ(c.size(), 1);
        CHECK_UNARY(c.insert("two"));
        CHECK_EQ(c.size(), 2);
        ++clock;
        c.sweep();
        CHECK_EQ(c.size(), 2);
        CHECK_UNARY(c.touch_if_exists("two"));
        ++clock;
        c.sweep();
        CHECK_EQ(c.size(), 1);
    }

    SUBCASE("Insert three items (1 over limit), sweep")
    {
        Cache c("test", LedgerIndex(2), 3s, clock, j);

        CHECK_UNARY(c.insert("one"));
        ++clock;
        CHECK_UNARY(c.insert("two"));
        ++clock;
        CHECK_UNARY(c.insert("three"));
        ++clock;
        CHECK_EQ(c.size(), 3);
        c.sweep();
        CHECK_LT(c.size(), 3);
    }
}

TEST_SUITE_END();

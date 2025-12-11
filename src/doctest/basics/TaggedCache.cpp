#include <xrpl/basics/TaggedCache.h>
#include <xrpl/basics/TaggedCache.ipp>
#include <xrpl/basics/chrono.h>
#include <xrpl/protocol/Protocol.h>

#include <doctest/doctest.h>

using namespace xrpl;

TEST_SUITE_BEGIN("TaggedCache");

TEST_CASE("TaggedCache operations")
{
    using namespace std::chrono_literals;

    TestStopwatch clock;
    clock.set(0);

    using Key = LedgerIndex;
    using Value = std::string;
    using Cache = TaggedCache<Key, Value>;

    beast::Journal j{beast::Journal::getNullSink()};

    Cache c("test", 1, 1s, clock, j);

    SUBCASE("Insert, retrieve, and age item")
    {
        CHECK(c.getCacheSize() == 0);
        CHECK(c.getTrackSize() == 0);
        CHECK(!c.insert(1, "one"));
        CHECK(c.getCacheSize() == 1);
        CHECK(c.getTrackSize() == 1);

        {
            std::string s;
            CHECK(c.retrieve(1, s));
            CHECK(s == "one");
        }

        ++clock;
        c.sweep();
        CHECK(c.getCacheSize() == 0);
        CHECK(c.getTrackSize() == 0);
    }

    SUBCASE("Insert item, maintain strong pointer, age it")
    {
        CHECK(!c.insert(2, "two"));
        CHECK(c.getCacheSize() == 1);
        CHECK(c.getTrackSize() == 1);

        {
            auto p = c.fetch(2);
            CHECK(p != nullptr);
            ++clock;
            c.sweep();
            CHECK(c.getCacheSize() == 0);
            CHECK(c.getTrackSize() == 1);
        }

        // Make sure its gone now that our reference is gone
        ++clock;
        c.sweep();
        CHECK(c.getCacheSize() == 0);
        CHECK(c.getTrackSize() == 0);
    }

    SUBCASE("Insert same key/value pair and canonicalize")
    {
        CHECK(!c.insert(3, "three"));

        {
            auto const p1 = c.fetch(3);
            auto p2 = std::make_shared<Value>("three");
            c.canonicalize_replace_client(3, p2);
            CHECK(p1.get() == p2.get());
        }
        ++clock;
        c.sweep();
        CHECK(c.getCacheSize() == 0);
        CHECK(c.getTrackSize() == 0);
    }

    SUBCASE("Put object, keep strong pointer, advance clock, canonicalize")
    {
        // Put an object in
        CHECK(!c.insert(4, "four"));
        CHECK(c.getCacheSize() == 1);
        CHECK(c.getTrackSize() == 1);

        {
            // Keep a strong pointer to it
            auto const p1 = c.fetch(4);
            CHECK(p1 != nullptr);
            CHECK(c.getCacheSize() == 1);
            CHECK(c.getTrackSize() == 1);
            // Advance the clock a lot
            ++clock;
            c.sweep();
            CHECK(c.getCacheSize() == 0);
            CHECK(c.getTrackSize() == 1);
            // Canonicalize a new object with the same key
            auto p2 = std::make_shared<std::string>("four");
            CHECK(c.canonicalize_replace_client(4, p2));
            CHECK(c.getCacheSize() == 1);
            CHECK(c.getTrackSize() == 1);
            // Make sure we get the original object
            CHECK(p1.get() == p2.get());
        }

        ++clock;
        c.sweep();
        CHECK(c.getCacheSize() == 0);
        CHECK(c.getTrackSize() == 0);
    }
}

TEST_SUITE_END();

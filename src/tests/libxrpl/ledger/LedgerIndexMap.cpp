#include <xrpl/ledger/LedgerIndexMap.h>

#include <doctest/doctest.h>

#include <cstdint>
#include <string>
#include <vector>

using namespace xrpl;

TEST_SUITE_BEGIN("LedgerIndexMap");

using TestMap = LedgerIndexMap<int, std::string>;

TEST_CASE("Default empty")
{
    TestMap m;
    CHECK(m.size() == 0);
    CHECK(m.empty());
    CHECK(m.get(42) == nullptr);
    CHECK_FALSE(m.contains(42));
}

TEST_CASE("Operator bracket inserts default")
{
    TestMap m;
    auto& v = m[10];
    CHECK(m.size() == 1);
    CHECK(m.contains(10));
    CHECK(v.empty());
}

TEST_CASE("Operator bracket, rvalue key")
{
    TestMap m;
    int k = 7;
    auto& v1 = m[std::move(k)];
    v1 = "seven";
    CHECK(m.size() == 1);
    auto* got = m.get(7);
    REQUIRE(got != nullptr);
    CHECK(*got == "seven");
}

TEST_CASE("Insert through put")
{
    TestMap m;
    auto& v = m.put(20, "twenty");
    CHECK(v == "twenty");
    auto* got = m.get(20);
    REQUIRE(got != nullptr);
    CHECK(*got == "twenty");
    CHECK(m.size() == 1);
}

TEST_CASE("Overwrite existing key with put")
{
    TestMap m;
    m.put(5, "five");
    CHECK(m.size() == 1);
    m.put(5, "FIVE");
    CHECK(m.size() == 1);
    auto* got = m.get(5);
    REQUIRE(got != nullptr);
    CHECK(*got == "FIVE");
}

TEST_CASE("Once found, one not found")
{
    TestMap m;
    m.put(1, "one");
    CHECK(m.get(1) != nullptr);
    CHECK(m.get(2) == nullptr);
}

TEST_CASE("Try eraseBefore - nothing to do")
{
    TestMap m;
    m.put(10, "a");
    m.put(11, "b");
    m.put(12, "c");
    CHECK(m.eraseBefore(10) == 0);
    CHECK(m.size() == 3);
    CHECK(m.contains(10));
    CHECK(m.contains(11));
    CHECK(m.contains(12));
}

TEST_CASE("eraseBefore - removes several entries")
{
    TestMap m;
    m.put(10, "a");
    m.put(11, "b");
    m.put(12, "c");
    m.put(13, "d");
    CHECK(m.eraseBefore(12) == 2);
    CHECK_FALSE(m.contains(10));
    CHECK_FALSE(m.contains(11));
    CHECK(m.contains(12));
    CHECK(m.contains(13));
    CHECK(m.size() == 2);
}

TEST_CASE("eraseBefore - removes all entries")
{
    TestMap m;
    m.put(1, "x");
    m.put(2, "y");
    CHECK(m.eraseBefore(1000) == 2);
    CHECK(m.size() == 0);
    CHECK(m.empty());
}

TEST_CASE("eraseBefore - same call, second time nothing to do")
{
    TestMap m;
    m.put(10, "a");
    m.put(11, "b");
    m.put(12, "c");
    CHECK(m.eraseBefore(12) == 2);
    CHECK(m.contains(12));
    CHECK(m.eraseBefore(12) == 0);
    CHECK(m.size() == 1);
}

TEST_CASE("eraseBefore - single entry removed")
{
    TestMap m;
    m.put(10, "v1");
    m.put(10, "v2");
    m.put(10, "v3");
    CHECK(m.size() == 1);
    CHECK(m.eraseBefore(11) == 1);
    CHECK(m.size() == 0);
}

TEST_CASE("eraseBefore - outlier still removed in one call")
{
    TestMap m;
    m.put(10, "a");
    m.put(12, "c");
    m.put(11, "b");  // out-of-order insert

    CHECK(m.eraseBefore(12) == 2);  // removes 10 and 11
    CHECK_FALSE(m.contains(10));
    CHECK_FALSE(m.contains(11));
    CHECK(m.contains(12));
    CHECK(m.size() == 1);
}

TEST_CASE("eraseBefore - erase in two steps (one first, then some more)")
{
    TestMap m;
    for (int k : {10, 11, 12, 13})
        m.put(k, std::to_string(k));

    CHECK(m.eraseBefore(11) == 1);
    CHECK_FALSE(m.contains(10));
    CHECK(m.size() == 3);

    CHECK(m.eraseBefore(13) == 2);
    CHECK_FALSE(m.contains(11));
    CHECK_FALSE(m.contains(12));
    CHECK(m.contains(13));
    CHECK(m.size() == 1);
}

TEST_CASE("rehash does not lose entries")
{
    TestMap m;
    for (int k = 0; k < 16; ++k)
        m.put(k, "v" + std::to_string(k));

    m.reserve(64);
    m.rehash(32);

    for (int k = 0; k < 16; ++k)
    {
        auto* v = m.get(k);
        REQUIRE(v != nullptr);
        CHECK(*v == "v" + std::to_string(k));
    }

    CHECK(m.eraseBefore(8) == 8);
    for (int k = 0; k < 8; ++k)
        CHECK_FALSE(m.contains(k));
    for (int k = 8; k < 16; ++k)
        CHECK(m.contains(k));
}

TEST_SUITE_END();

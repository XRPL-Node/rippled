// Issue doctest - converted from src/test/protocol/Issue_test.cpp

#include <xrpl/basics/UnorderedContainers.h>
#include <xrpl/protocol/Book.h>
#include <xrpl/protocol/Issue.h>

#include <doctest/doctest.h>

#include <map>
#include <optional>
#include <set>
#include <unordered_set>

#if BEAST_MSVC
#define STL_SET_HAS_EMPLACE 1
#else
#define STL_SET_HAS_EMPLACE 0
#endif

#ifndef XRPL_ASSETS_ENABLE_STD_HASH
#if BEAST_MAC || BEAST_IOS
#define XRPL_ASSETS_ENABLE_STD_HASH 0
#else
#define XRPL_ASSETS_ENABLE_STD_HASH 1
#endif
#endif

TEST_SUITE_BEGIN("Issue");

namespace {

using namespace xrpl;
using Domain = uint256;

// Comparison, hash tests for uint60 (via base_uint)
template <typename Unsigned>
void
testUnsigned()
{
    Unsigned const u1(1);
    Unsigned const u2(2);
    Unsigned const u3(3);

    CHECK(u1 != u2);
    CHECK(u1 < u2);
    CHECK(u1 <= u2);
    CHECK(u2 <= u2);
    CHECK(u2 == u2);
    CHECK(u2 >= u2);
    CHECK(u3 >= u2);
    CHECK(u3 > u2);

    std::hash<Unsigned> hash;

    CHECK(hash(u1) == hash(u1));
    CHECK(hash(u2) == hash(u2));
    CHECK(hash(u3) == hash(u3));
    CHECK(hash(u1) != hash(u2));
    CHECK(hash(u1) != hash(u3));
    CHECK(hash(u2) != hash(u3));
}

// Comparison, hash tests for Issue
template <class IssueType>
void
testIssue()
{
    Currency const c1(1);
    AccountID const i1(1);
    Currency const c2(2);
    AccountID const i2(2);
    Currency const c3(3);
    AccountID const i3(3);

    CHECK(IssueType(c1, i1) != IssueType(c2, i1));
    CHECK(IssueType(c1, i1) < IssueType(c2, i1));
    CHECK(IssueType(c1, i1) <= IssueType(c2, i1));
    CHECK(IssueType(c2, i1) <= IssueType(c2, i1));
    CHECK(IssueType(c2, i1) == IssueType(c2, i1));
    CHECK(IssueType(c2, i1) >= IssueType(c2, i1));
    CHECK(IssueType(c3, i1) >= IssueType(c2, i1));
    CHECK(IssueType(c3, i1) > IssueType(c2, i1));
    CHECK(IssueType(c1, i1) != IssueType(c1, i2));
    CHECK(IssueType(c1, i1) < IssueType(c1, i2));
    CHECK(IssueType(c1, i1) <= IssueType(c1, i2));
    CHECK(IssueType(c1, i2) <= IssueType(c1, i2));
    CHECK(IssueType(c1, i2) == IssueType(c1, i2));
    CHECK(IssueType(c1, i2) >= IssueType(c1, i2));
    CHECK(IssueType(c1, i3) >= IssueType(c1, i2));
    CHECK(IssueType(c1, i3) > IssueType(c1, i2));

    std::hash<IssueType> hash;

    CHECK(hash(IssueType(c1, i1)) == hash(IssueType(c1, i1)));
    CHECK(hash(IssueType(c1, i2)) == hash(IssueType(c1, i2)));
    CHECK(hash(IssueType(c1, i3)) == hash(IssueType(c1, i3)));
    CHECK(hash(IssueType(c2, i1)) == hash(IssueType(c2, i1)));
    CHECK(hash(IssueType(c2, i2)) == hash(IssueType(c2, i2)));
    CHECK(hash(IssueType(c2, i3)) == hash(IssueType(c2, i3)));
    CHECK(hash(IssueType(c3, i1)) == hash(IssueType(c3, i1)));
    CHECK(hash(IssueType(c3, i2)) == hash(IssueType(c3, i2)));
    CHECK(hash(IssueType(c3, i3)) == hash(IssueType(c3, i3)));
    CHECK(hash(IssueType(c1, i1)) != hash(IssueType(c1, i2)));
    CHECK(hash(IssueType(c1, i1)) != hash(IssueType(c1, i3)));
    CHECK(hash(IssueType(c1, i1)) != hash(IssueType(c2, i1)));
    CHECK(hash(IssueType(c1, i1)) != hash(IssueType(c2, i2)));
    CHECK(hash(IssueType(c1, i1)) != hash(IssueType(c2, i3)));
    CHECK(hash(IssueType(c1, i1)) != hash(IssueType(c3, i1)));
    CHECK(hash(IssueType(c1, i1)) != hash(IssueType(c3, i2)));
    CHECK(hash(IssueType(c1, i1)) != hash(IssueType(c3, i3)));
}

template <class Set>
void
testIssueSet()
{
    Currency const c1(1);
    AccountID const i1(1);
    Currency const c2(2);
    AccountID const i2(2);
    Issue const a1(c1, i1);
    Issue const a2(c2, i2);

    {
        Set c;

        c.insert(a1);
        REQUIRE(c.size() == 1);
        c.insert(a2);
        REQUIRE(c.size() == 2);

        REQUIRE(c.erase(Issue(c1, i2)) == 0);
        REQUIRE(c.erase(Issue(c1, i1)) == 1);
        REQUIRE(c.erase(Issue(c2, i2)) == 1);
        REQUIRE(c.empty());
    }

    {
        Set c;

        c.insert(a1);
        REQUIRE(c.size() == 1);
        c.insert(a2);
        REQUIRE(c.size() == 2);

        REQUIRE(c.erase(Issue(c1, i2)) == 0);
        REQUIRE(c.erase(Issue(c1, i1)) == 1);
        REQUIRE(c.erase(Issue(c2, i2)) == 1);
        REQUIRE(c.empty());

#if STL_SET_HAS_EMPLACE
        c.emplace(c1, i1);
        REQUIRE(c.size() == 1);
        c.emplace(c2, i2);
        REQUIRE(c.size() == 2);
#endif
    }
}

}  // namespace

TEST_CASE("Currency")
{
    testUnsigned<Currency>();
}

TEST_CASE("AccountID")
{
    testUnsigned<AccountID>();
}

TEST_CASE("Issue")
{
    testIssue<Issue>();
}

TEST_CASE("std::set<Issue>")
{
    testIssueSet<std::set<Issue>>();
}

#if XRPL_ASSETS_ENABLE_STD_HASH
TEST_CASE("std::unordered_set<Issue>")
{
    testIssueSet<std::unordered_set<Issue>>();
}
#endif

TEST_CASE("hash_set<Issue>")
{
    testIssueSet<hash_set<Issue>>();
}

TEST_SUITE_END();

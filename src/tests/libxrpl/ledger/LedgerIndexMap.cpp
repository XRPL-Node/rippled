#include <xrpl/ledger/LedgerIndexMap.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

using namespace xrpl;

using TestMap = LedgerIndexMap<int, std::string>;

TEST(LedgerIndexMap, DefaultEmpty)
{
    TestMap m;
    EXPECT_EQ(m.size(), 0);
    EXPECT_TRUE(m.empty());
    EXPECT_EQ(m.get(42), nullptr);
    EXPECT_FALSE(m.contains(42));
}

TEST(LedgerIndexMap, OperatorBracketInsertsDefault)
{
    TestMap m;
    auto& v = m[10];
    EXPECT_EQ(m.size(), 1);
    EXPECT_TRUE(m.contains(10));
    EXPECT_TRUE(v.empty());
}

TEST(LedgerIndexMap, OperatorBracketRvalueKey)
{
    TestMap m;
    int k = 7;
    auto& v1 = m[std::move(k)];
    v1 = "seven";
    EXPECT_EQ(m.size(), 1);
    auto* got = m.get(7);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(*got, "seven");
}

TEST(LedgerIndexMap, InsertThroughPut)
{
    TestMap m;
    auto& v = m.put(20, "twenty");
    EXPECT_EQ(v, "twenty");
    auto* got = m.get(20);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(*got, "twenty");
    EXPECT_EQ(m.size(), 1);
}

TEST(LedgerIndexMap, OverwriteExistingKeyWithPut)
{
    TestMap m;
    m.put(5, "five");
    EXPECT_EQ(m.size(), 1);
    m.put(5, "FIVE");
    EXPECT_EQ(m.size(), 1);
    auto* got = m.get(5);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(*got, "FIVE");
}

TEST(LedgerIndexMap, OnceFoundOneNotFound)
{
    TestMap m;
    m.put(1, "one");
    EXPECT_NE(m.get(1), nullptr);
    EXPECT_EQ(m.get(2), nullptr);
}

TEST(LedgerIndexMap, TryEraseBeforeNothingToDo)
{
    TestMap m;
    m.put(10, "a");
    m.put(11, "b");
    m.put(12, "c");
    EXPECT_EQ(m.eraseBefore(10), 0);
    EXPECT_EQ(m.size(), 3);
    EXPECT_TRUE(m.contains(10));
    EXPECT_TRUE(m.contains(11));
    EXPECT_TRUE(m.contains(12));
}

TEST(LedgerIndexMap, EraseBeforeRemovesSeveralEntries)
{
    TestMap m;
    m.put(10, "a");
    m.put(11, "b");
    m.put(12, "c");
    m.put(13, "d");
    EXPECT_EQ(m.eraseBefore(12), 2);
    EXPECT_FALSE(m.contains(10));
    EXPECT_FALSE(m.contains(11));
    EXPECT_TRUE(m.contains(12));
    EXPECT_TRUE(m.contains(13));
    EXPECT_EQ(m.size(), 2);
}

TEST(LedgerIndexMap, EraseBeforeRemovesAllEntries)
{
    TestMap m;
    m.put(1, "x");
    m.put(2, "y");
    EXPECT_EQ(m.eraseBefore(1000), 2);
    EXPECT_EQ(m.size(), 0);
    EXPECT_TRUE(m.empty());
}

TEST(LedgerIndexMap, EraseBeforeSameCallSecondTimeNothingToDo)
{
    TestMap m;
    m.put(10, "a");
    m.put(11, "b");
    m.put(12, "c");
    EXPECT_EQ(m.eraseBefore(12), 2);
    EXPECT_TRUE(m.contains(12));
    EXPECT_EQ(m.eraseBefore(12), 0);
    EXPECT_EQ(m.size(), 1);
}

TEST(LedgerIndexMap, EraseBeforeSingleEntryRemoved)
{
    TestMap m;
    m.put(10, "v1");
    m.put(10, "v2");
    m.put(10, "v3");
    EXPECT_EQ(m.size(), 1);
    EXPECT_EQ(m.eraseBefore(11), 1);
    EXPECT_EQ(m.size(), 0);
}

TEST(LedgerIndexMap, EraseBeforeOutlierStillRemovedInOneCall)
{
    TestMap m;
    m.put(10, "a");
    m.put(12, "c");
    m.put(11, "b");  // out-of-order insert

    EXPECT_EQ(m.eraseBefore(12), 2);  // removes 10 and 11
    EXPECT_FALSE(m.contains(10));
    EXPECT_FALSE(m.contains(11));
    EXPECT_TRUE(m.contains(12));
    EXPECT_EQ(m.size(), 1);
}

TEST(LedgerIndexMap, EraseBeforeEraseInTwoSteps)
{
    TestMap m;
    for (int k : {10, 11, 12, 13})
        m.put(k, std::to_string(k));

    EXPECT_EQ(m.eraseBefore(11), 1);
    EXPECT_FALSE(m.contains(10));
    EXPECT_EQ(m.size(), 3);

    EXPECT_EQ(m.eraseBefore(13), 2);
    EXPECT_FALSE(m.contains(11));
    EXPECT_FALSE(m.contains(12));
    EXPECT_TRUE(m.contains(13));
    EXPECT_EQ(m.size(), 1);
}

TEST(LedgerIndexMap, RehashDoesNotLoseEntries)
{
    TestMap m;
    for (int k = 0; k < 16; ++k)
        m.put(k, "v" + std::to_string(k));

    m.reserve(64);
    m.rehash(32);

    for (int k = 0; k < 16; ++k)
    {
        auto* v = m.get(k);
        ASSERT_NE(v, nullptr);
        EXPECT_EQ(*v, "v" + std::to_string(k));
    }

    EXPECT_EQ(m.eraseBefore(8), 8);
    for (int k = 0; k < 8; ++k)
        EXPECT_FALSE(m.contains(k));
    for (int k = 8; k < 16; ++k)
        EXPECT_TRUE(m.contains(k));
}

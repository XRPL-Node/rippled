#include <xrpl/beast/hash/xxhasher.h>

#include <doctest/doctest.h>

#include <string>

using namespace beast;

TEST_SUITE_BEGIN("XXHasher");

TEST_CASE("Without seed")
{
    xxhasher hasher{};

    std::string objectToHash{"Hello, xxHash!"};
    hasher(objectToHash.data(), objectToHash.size());

    CHECK(
        static_cast<xxhasher::result_type>(hasher) == 16042857369214894119ULL);
}

TEST_CASE("With seed")
{
    xxhasher hasher{static_cast<std::uint32_t>(102)};

    std::string objectToHash{"Hello, xxHash!"};
    hasher(objectToHash.data(), objectToHash.size());

    CHECK(
        static_cast<xxhasher::result_type>(hasher) == 14440132435660934800ULL);
}

TEST_CASE("With two seeds")
{
    xxhasher hasher{
        static_cast<std::uint32_t>(102), static_cast<std::uint32_t>(103)};

    std::string objectToHash{"Hello, xxHash!"};
    hasher(objectToHash.data(), objectToHash.size());

    CHECK(
        static_cast<xxhasher::result_type>(hasher) == 14440132435660934800ULL);
}

TEST_CASE("Big object with multiple small updates without seed")
{
    xxhasher hasher{};

    std::string objectToHash{"Hello, xxHash!"};
    for (int i = 0; i < 100; i++)
    {
        hasher(objectToHash.data(), objectToHash.size());
    }

    CHECK(
        static_cast<xxhasher::result_type>(hasher) == 15296278154063476002ULL);
}

TEST_CASE("Big object with multiple small updates with seed")
{
    xxhasher hasher{static_cast<std::uint32_t>(103)};

    std::string objectToHash{"Hello, xxHash!"};
    for (int i = 0; i < 100; i++)
    {
        hasher(objectToHash.data(), objectToHash.size());
    }

    CHECK(
        static_cast<xxhasher::result_type>(hasher) == 17285302196561698791ULL);
}

TEST_CASE("Big object with small and big updates without seed")
{
    xxhasher hasher{};

    std::string objectToHash{"Hello, xxHash!"};
    std::string bigObject;
    for (int i = 0; i < 20; i++)
    {
        bigObject += "Hello, xxHash!";
    }
    hasher(objectToHash.data(), objectToHash.size());
    hasher(bigObject.data(), bigObject.size());
    hasher(objectToHash.data(), objectToHash.size());

    CHECK(static_cast<xxhasher::result_type>(hasher) == 1865045178324729219ULL);
}

TEST_CASE("Big object with small and big updates with seed")
{
    xxhasher hasher{static_cast<std::uint32_t>(103)};

    std::string objectToHash{"Hello, xxHash!"};
    std::string bigObject;
    for (int i = 0; i < 20; i++)
    {
        bigObject += "Hello, xxHash!";
    }
    hasher(objectToHash.data(), objectToHash.size());
    hasher(bigObject.data(), bigObject.size());
    hasher(objectToHash.data(), objectToHash.size());

    CHECK(
        static_cast<xxhasher::result_type>(hasher) == 16189862915636005281ULL);
}

TEST_CASE("Big object with one update without seed")
{
    xxhasher hasher{};

    std::string objectToHash;
    for (int i = 0; i < 100; i++)
    {
        objectToHash += "Hello, xxHash!";
    }
    hasher(objectToHash.data(), objectToHash.size());

    CHECK(
        static_cast<xxhasher::result_type>(hasher) == 15296278154063476002ULL);
}

TEST_CASE("Big object with one update with seed")
{
    xxhasher hasher{static_cast<std::uint32_t>(103)};

    std::string objectToHash;
    for (int i = 0; i < 100; i++)
    {
        objectToHash += "Hello, xxHash!";
    }
    hasher(objectToHash.data(), objectToHash.size());

    CHECK(
        static_cast<xxhasher::result_type>(hasher) == 17285302196561698791ULL);
}

TEST_CASE("Operator result type doesn't change the internal state")
{
    SUBCASE("small object")
    {
        xxhasher hasher;

        std::string object{"Hello xxhash"};
        hasher(object.data(), object.size());
        auto xxhashResult1 = static_cast<xxhasher::result_type>(hasher);
        auto xxhashResult2 = static_cast<xxhasher::result_type>(hasher);

        CHECK(xxhashResult1 == xxhashResult2);
    }
    SUBCASE("big object")
    {
        xxhasher hasher;

        std::string object;
        for (int i = 0; i < 100; i++)
        {
            object += "Hello, xxHash!";
        }
        hasher(object.data(), object.size());
        auto xxhashResult1 = hasher.operator xxhasher::result_type();
        auto xxhashResult2 = hasher.operator xxhasher::result_type();

        CHECK(xxhashResult1 == xxhashResult2);
    }
}

TEST_SUITE_END();

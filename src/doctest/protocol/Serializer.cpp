#include <xrpl/protocol/Serializer.h>

#include <doctest/doctest.h>

#include <limits>

using namespace xrpl;

TEST_SUITE_BEGIN("Serializer");

TEST_CASE("Serializer add32/geti32")
{
    std::initializer_list<std::int32_t> const values = {
        std::numeric_limits<std::int32_t>::min(),
        -1,
        0,
        1,
        std::numeric_limits<std::int32_t>::max()};
    for (std::int32_t value : values)
    {
        Serializer s;
        s.add32(value);
        CHECK_EQ(s.size(), 4);
        SerialIter sit(s.slice());
        CHECK_EQ(sit.geti32(), value);
    }
}

TEST_CASE("Serializer add64/geti64")
{
    std::initializer_list<std::int64_t> const values = {
        std::numeric_limits<std::int64_t>::min(),
        -1,
        0,
        1,
        std::numeric_limits<std::int64_t>::max()};
    for (std::int64_t value : values)
    {
        Serializer s;
        s.add64(value);
        CHECK_EQ(s.size(), 8);
        SerialIter sit(s.slice());
        CHECK_EQ(sit.geti64(), value);
    }
}

TEST_SUITE_END();

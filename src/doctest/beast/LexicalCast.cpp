#include <xrpl/beast/core/LexicalCast.h>
#include <xrpl/beast/xor_shift_engine.h>

#include <doctest/doctest.h>

#include <sstream>

using namespace beast;

TEST_SUITE_BEGIN("LexicalCast");

namespace {

template <class IntType>
IntType
nextRandomInt(xor_shift_engine& r)
{
    return static_cast<IntType>(r());
}

template <class IntType>
void
testInteger(IntType in)
{
    std::string s;
    IntType out(in + 1);

    CHECK_UNARY(lexicalCastChecked(s, in));
    CHECK_UNARY(lexicalCastChecked(out, s));
    CHECK_EQ(out, in);
}

template <class IntType>
void
testIntegers(xor_shift_engine& r)
{
    for (int i = 0; i < 1000; ++i)
    {
        IntType const value(nextRandomInt<IntType>(r));
        testInteger(value);
    }

    testInteger(std::numeric_limits<IntType>::min());
    testInteger(std::numeric_limits<IntType>::max());
}

template <class T>
void
tryBadConvert(std::string const& s)
{
    T out;
    CHECK_FALSE(lexicalCastChecked(out, s));
}

template <class T>
bool
tryEdgeCase(std::string const& s)
{
    T ret;

    bool const result = lexicalCastChecked(ret, s);

    if (!result)
        return false;

    return s == std::to_string(ret);
}

template <class T>
void
testThrowConvert(std::string const& s, bool success)
{
    bool result = !success;
    T out;

    try
    {
        out = lexicalCastThrow<T>(s);
        result = true;
    }
    catch (BadLexicalCast const&)
    {
        result = false;
    }

    CHECK_EQ(result, success);
}

}  // namespace

TEST_CASE("random integers")
{
    std::int64_t const seedValue = 50;
    xor_shift_engine r(seedValue);

    SUBCASE("int")
    {
        testIntegers<int>(r);
    }
    SUBCASE("unsigned int")
    {
        testIntegers<unsigned int>(r);
    }
    SUBCASE("short")
    {
        testIntegers<short>(r);
    }
    SUBCASE("unsigned short")
    {
        testIntegers<unsigned short>(r);
    }
    SUBCASE("int32_t")
    {
        testIntegers<std::int32_t>(r);
    }
    SUBCASE("uint32_t")
    {
        testIntegers<std::uint32_t>(r);
    }
    SUBCASE("int64_t")
    {
        testIntegers<std::int64_t>(r);
    }
    SUBCASE("uint64_t")
    {
        testIntegers<std::uint64_t>(r);
    }
}

TEST_CASE("pathologies")
{
    CHECK_THROWS_AS(
        lexicalCastThrow<int>("\xef\xbc\x91\xef\xbc\x90"), BadLexicalCast);
}

TEST_CASE("conversion overflows")
{
    tryBadConvert<std::uint64_t>("99999999999999999999");
    tryBadConvert<std::uint32_t>("4294967300");
    tryBadConvert<std::uint16_t>("75821");
}

TEST_CASE("conversion underflows")
{
    tryBadConvert<std::uint32_t>("-1");

    tryBadConvert<std::int64_t>("-99999999999999999999");
    tryBadConvert<std::int32_t>("-4294967300");
    tryBadConvert<std::int16_t>("-75821");
}

TEST_CASE("conversion edge cases")
{
    CHECK(tryEdgeCase<std::uint64_t>("18446744073709551614"));
    CHECK(tryEdgeCase<std::uint64_t>("18446744073709551615"));
    CHECK_FALSE(tryEdgeCase<std::uint64_t>("18446744073709551616"));

    CHECK(tryEdgeCase<std::int64_t>("9223372036854775806"));
    CHECK(tryEdgeCase<std::int64_t>("9223372036854775807"));
    CHECK_FALSE(tryEdgeCase<std::int64_t>("9223372036854775808"));

    CHECK(tryEdgeCase<std::int64_t>("-9223372036854775807"));
    CHECK(tryEdgeCase<std::int64_t>("-9223372036854775808"));
    CHECK_FALSE(tryEdgeCase<std::int64_t>("-9223372036854775809"));

    CHECK(tryEdgeCase<std::uint32_t>("4294967294"));
    CHECK(tryEdgeCase<std::uint32_t>("4294967295"));
    CHECK_FALSE(tryEdgeCase<std::uint32_t>("4294967296"));

    CHECK(tryEdgeCase<std::int32_t>("2147483646"));
    CHECK(tryEdgeCase<std::int32_t>("2147483647"));
    CHECK_FALSE(tryEdgeCase<std::int32_t>("2147483648"));

    CHECK(tryEdgeCase<std::int32_t>("-2147483647"));
    CHECK(tryEdgeCase<std::int32_t>("-2147483648"));
    CHECK_FALSE(tryEdgeCase<std::int32_t>("-2147483649"));

    CHECK(tryEdgeCase<std::uint16_t>("65534"));
    CHECK(tryEdgeCase<std::uint16_t>("65535"));
    CHECK_FALSE(tryEdgeCase<std::uint16_t>("65536"));

    CHECK(tryEdgeCase<std::int16_t>("32766"));
    CHECK(tryEdgeCase<std::int16_t>("32767"));
    CHECK_FALSE(tryEdgeCase<std::int16_t>("32768"));

    CHECK(tryEdgeCase<std::int16_t>("-32767"));
    CHECK(tryEdgeCase<std::int16_t>("-32768"));
    CHECK_FALSE(tryEdgeCase<std::int16_t>("-32769"));
}

TEST_CASE("throwing conversion")
{
    testThrowConvert<std::uint64_t>("99999999999999999999", false);
    testThrowConvert<std::uint64_t>("9223372036854775806", true);

    testThrowConvert<std::uint32_t>("4294967290", true);
    testThrowConvert<std::uint32_t>("42949672900", false);
    testThrowConvert<std::uint32_t>("429496729000", false);
    testThrowConvert<std::uint32_t>("4294967290000", false);

    testThrowConvert<std::int32_t>("5294967295", false);
    testThrowConvert<std::int32_t>("-2147483644", true);

    testThrowConvert<std::int16_t>("66666", false);
    testThrowConvert<std::int16_t>("-5711", true);
}

TEST_CASE("zero conversion")
{
    SUBCASE("signed")
    {
        std::int32_t out;

        CHECK(lexicalCastChecked(out, "-0"));
        CHECK(lexicalCastChecked(out, "0"));
        CHECK(lexicalCastChecked(out, "+0"));
    }

    SUBCASE("unsigned")
    {
        std::uint32_t out;

        CHECK_FALSE(lexicalCastChecked(out, "-0"));
        CHECK(lexicalCastChecked(out, "0"));
        CHECK(lexicalCastChecked(out, "+0"));
    }
}

TEST_CASE("entire range")
{
    std::int32_t i = std::numeric_limits<std::int16_t>::min();
    std::string const empty("");

    while (i <= std::numeric_limits<std::int16_t>::max())
    {
        std::int16_t j = static_cast<std::int16_t>(i);

        auto actual = std::to_string(j);

        auto result = lexicalCast(j, empty);

        CHECK_EQ(result, actual);

        if (result == actual)
        {
            auto number = lexicalCast<std::int16_t>(result);
            CHECK_EQ(number, j);
        }

        i++;
    }
}

TEST_SUITE_END();

#include <xrpl/json/json_forwards.h>
#include <xrpl/protocol/Issue.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/STAmount.h>
#include <xrpl/protocol/STNumber.h>

#include <doctest/doctest.h>

#include <limits>

using namespace xrpl;

TEST_SUITE_BEGIN("STNumber");

namespace {

void
testCombo(Number number)
{
    STNumber const before{sfNumber, number};
    CHECK_EQ(number, before);
    Serializer s;
    before.add(s);
    CHECK_EQ(s.size(), 12);
    SerialIter sit(s.slice());
    STNumber const after{sit, sfNumber};
    CHECK_UNARY(after.isEquivalent(before));
    CHECK_EQ(number, after);
}

}  // namespace

TEST_CASE("STNumber default constructor")
{
    static_assert(!std::is_convertible_v<STNumber*, Number*>);

    STNumber const stnum{sfNumber};
    CHECK_EQ(stnum.getSType(), STI_NUMBER);
    CHECK_EQ(stnum.getText(), "0");
    CHECK_UNARY(stnum.isDefault());
    CHECK_EQ(stnum.value(), Number{0});
}

TEST_CASE("STNumber mantissa serialization")
{
    std::initializer_list<std::int64_t> const mantissas = {
        std::numeric_limits<std::int64_t>::min(),
        -1,
        0,
        1,
        std::numeric_limits<std::int64_t>::max()};
    for (std::int64_t mantissa : mantissas)
        testCombo(Number{mantissa});
}

TEST_CASE("STNumber exponent serialization")
{
    std::initializer_list<std::int32_t> const exponents = {
        Number::minExponent, -1, 0, 1, Number::maxExponent - 1};
    for (std::int32_t exponent : exponents)
        testCombo(Number{123, exponent});
}

TEST_CASE("STNumber multiplication with STAmount")
{
    STAmount const strikePrice{noIssue(), 100};
    STNumber const factor{sfNumber, 100};
    auto const iouValue = strikePrice.iou();
    IOUAmount totalValue{iouValue * factor};
    STAmount const totalAmount{totalValue, strikePrice.issue()};
    CHECK_EQ(totalAmount, Number{10'000});
}

TEST_CASE("numberFromJson integer values")
{
    CHECK_EQ(numberFromJson(sfNumber, Json::Value(42)), STNumber(sfNumber, 42));
    CHECK_EQ(
        numberFromJson(sfNumber, Json::Value(-42)), STNumber(sfNumber, -42));
    CHECK_EQ(numberFromJson(sfNumber, Json::UInt(42)), STNumber(sfNumber, 42));
}

TEST_CASE("numberFromJson string values")
{
    CHECK_EQ(numberFromJson(sfNumber, "-123"), STNumber(sfNumber, -123));
    CHECK_EQ(numberFromJson(sfNumber, "123"), STNumber(sfNumber, 123));
    CHECK_EQ(numberFromJson(sfNumber, "-123"), STNumber(sfNumber, -123));

    CHECK_EQ(
        numberFromJson(sfNumber, "3.14"), STNumber(sfNumber, Number(314, -2)));
    CHECK_EQ(
        numberFromJson(sfNumber, "-3.14"),
        STNumber(sfNumber, -Number(314, -2)));
    CHECK_EQ(numberFromJson(sfNumber, "3.14e2"), STNumber(sfNumber, 314));
    CHECK_EQ(numberFromJson(sfNumber, "-3.14e2"), STNumber(sfNumber, -314));

    CHECK_EQ(numberFromJson(sfNumber, "1000e-2"), STNumber(sfNumber, 10));
    CHECK_EQ(numberFromJson(sfNumber, "-1000e-2"), STNumber(sfNumber, -10));
}

TEST_CASE("numberFromJson zero values")
{
    CHECK_EQ(numberFromJson(sfNumber, "0"), STNumber(sfNumber, 0));
    CHECK_EQ(numberFromJson(sfNumber, "0.0"), STNumber(sfNumber, 0));
    CHECK_EQ(numberFromJson(sfNumber, "0.000"), STNumber(sfNumber, 0));
    CHECK_EQ(numberFromJson(sfNumber, "-0"), STNumber(sfNumber, 0));
    CHECK_EQ(numberFromJson(sfNumber, "-0.0"), STNumber(sfNumber, 0));
    CHECK_EQ(numberFromJson(sfNumber, "-0.000"), STNumber(sfNumber, 0));
    CHECK_EQ(numberFromJson(sfNumber, "0e6"), STNumber(sfNumber, 0));
    CHECK_EQ(numberFromJson(sfNumber, "0.0e6"), STNumber(sfNumber, 0));
    CHECK_EQ(numberFromJson(sfNumber, "0.000e6"), STNumber(sfNumber, 0));
    CHECK_EQ(numberFromJson(sfNumber, "-0e6"), STNumber(sfNumber, 0));
    CHECK_EQ(numberFromJson(sfNumber, "-0.0e6"), STNumber(sfNumber, 0));
    CHECK_EQ(numberFromJson(sfNumber, "-0.000e6"), STNumber(sfNumber, 0));
}

TEST_CASE("numberFromJson int limits")
{
    constexpr auto imin = std::numeric_limits<int>::min();
    CHECK_EQ(
        numberFromJson(sfNumber, imin), STNumber(sfNumber, Number(imin, 0)));
    CHECK_EQ(
        numberFromJson(sfNumber, std::to_string(imin)),
        STNumber(sfNumber, Number(imin, 0)));

    constexpr auto imax = std::numeric_limits<int>::max();
    CHECK_EQ(
        numberFromJson(sfNumber, imax), STNumber(sfNumber, Number(imax, 0)));
    CHECK_EQ(
        numberFromJson(sfNumber, std::to_string(imax)),
        STNumber(sfNumber, Number(imax, 0)));

    constexpr auto umax = std::numeric_limits<unsigned int>::max();
    CHECK_EQ(
        numberFromJson(sfNumber, umax), STNumber(sfNumber, Number(umax, 0)));
    CHECK_EQ(
        numberFromJson(sfNumber, std::to_string(umax)),
        STNumber(sfNumber, Number(umax, 0)));
}

TEST_CASE("numberFromJson invalid values")
{
    SUBCASE("empty string")
    {
        CHECK_THROWS_AS(numberFromJson(sfNumber, ""), std::runtime_error);
    }

    SUBCASE("just e")
    {
        CHECK_THROWS_AS(numberFromJson(sfNumber, "e"), std::runtime_error);
    }

    SUBCASE("trailing e")
    {
        CHECK_THROWS_AS(numberFromJson(sfNumber, "1e"), std::runtime_error);
    }

    SUBCASE("leading e")
    {
        CHECK_THROWS_AS(numberFromJson(sfNumber, "e2"), std::runtime_error);
    }

    SUBCASE("null json")
    {
        CHECK_THROWS_AS(
            numberFromJson(sfNumber, Json::Value()), std::runtime_error);
    }

    SUBCASE("very large number")
    {
        CHECK_THROWS_AS(
            numberFromJson(
                sfNumber,
                "1234567890123456789012345678901234567890123456789012345678"
                "9012345678901234567890123456789012345678901234567890123456"
                "78901234567890123456789012345678901234567890"),
            std::bad_cast);
    }

    SUBCASE("leading zeros")
    {
        CHECK_THROWS_AS(numberFromJson(sfNumber, "001"), std::runtime_error);
        CHECK_THROWS_AS(numberFromJson(sfNumber, "000.0"), std::runtime_error);
    }

    SUBCASE("dangling dot")
    {
        CHECK_THROWS_AS(numberFromJson(sfNumber, ".1"), std::runtime_error);
        CHECK_THROWS_AS(numberFromJson(sfNumber, "1."), std::runtime_error);
        CHECK_THROWS_AS(numberFromJson(sfNumber, "1.e3"), std::runtime_error);
    }
}

TEST_SUITE_END();

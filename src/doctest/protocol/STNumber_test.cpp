#include <xrpl/json/json_forwards.h>
#include <xrpl/protocol/Issue.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/STAmount.h>
#include <xrpl/protocol/STNumber.h>

#include <doctest/doctest.h>

#include <limits>
#include <ostream>
#include <stdexcept>

using xrpl::IOUAmount;
using xrpl::noIssue;
using xrpl::Number;
using xrpl::numberFromJson;
using xrpl::SerialIter;
using xrpl::Serializer;
using xrpl::sfNumber;
using xrpl::STAmount;
using xrpl::STI_NUMBER;
using xrpl::STNumber;

namespace xrpl {

TEST_SUITE_BEGIN("protocol");

static void
testCombo(Number number)
{
    STNumber const before{sfNumber, number};
    CHECK(number == before);
    Serializer s;
    before.add(s);
    CHECK(s.size() == 12);
    SerialIter sit(s.slice());
    STNumber const after{sit, sfNumber};
    CHECK(after.isEquivalent(before));
    CHECK(number == after);
}

TEST_CASE("STNumber_test")
{
    static_assert(!std::is_convertible_v<STNumber*, Number*>);

    SUBCASE("Default construction")
    {
        STNumber const stnum{sfNumber};
        CHECK(stnum.getSType() == STI_NUMBER);
        CHECK(stnum.getText() == "0");
        CHECK(stnum.isDefault() == true);
        CHECK(stnum.value() == Number{0});
    }

    SUBCASE("Mantissa tests")
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

    SUBCASE("Exponent tests")
    {
        std::initializer_list<std::int32_t> const exponents = {
            Number::minExponent, -1, 0, 1, Number::maxExponent - 1};
        for (std::int32_t exponent : exponents)
            testCombo(Number{123, exponent});
    }

    SUBCASE("STAmount multiplication")
    {
        STAmount const strikePrice{noIssue(), 100};
        STNumber const factor{sfNumber, 100};
        auto const iouValue = strikePrice.iou();
        IOUAmount totalValue{iouValue * factor};
        STAmount const totalAmount{totalValue, strikePrice.issue()};
        CHECK(totalAmount == Number{10'000});
    }

    SUBCASE("JSON parsing - integers")
    {
        CHECK(
            numberFromJson(sfNumber, Json::Value(42)) ==
            STNumber(sfNumber, 42));
        CHECK(
            numberFromJson(sfNumber, Json::Value(-42)) ==
            STNumber(sfNumber, -42));

        CHECK(
            numberFromJson(sfNumber, Json::UInt(42)) ==
            STNumber(sfNumber, 42));

        CHECK(
            numberFromJson(sfNumber, "-123") == STNumber(sfNumber, -123));

        CHECK(numberFromJson(sfNumber, "123") == STNumber(sfNumber, 123));
        CHECK(numberFromJson(sfNumber, "-123") == STNumber(sfNumber, -123));
    }

    SUBCASE("JSON parsing - decimals")
    {
        CHECK(
            numberFromJson(sfNumber, "3.14") ==
            STNumber(sfNumber, Number(314, -2)));
        CHECK(
            numberFromJson(sfNumber, "-3.14") ==
            STNumber(sfNumber, -Number(314, -2)));
        CHECK(
            numberFromJson(sfNumber, "3.14e2") == STNumber(sfNumber, 314));
        CHECK(
            numberFromJson(sfNumber, "-3.14e2") == STNumber(sfNumber, -314));

        CHECK(numberFromJson(sfNumber, "1000e-2") == STNumber(sfNumber, 10));
        CHECK(
            numberFromJson(sfNumber, "-1000e-2") == STNumber(sfNumber, -10));
    }

    SUBCASE("JSON parsing - zeros")
    {
        CHECK(numberFromJson(sfNumber, "0") == STNumber(sfNumber, 0));
        CHECK(numberFromJson(sfNumber, "0.0") == STNumber(sfNumber, 0));
        CHECK(numberFromJson(sfNumber, "0.000") == STNumber(sfNumber, 0));
        CHECK(numberFromJson(sfNumber, "-0") == STNumber(sfNumber, 0));
        CHECK(numberFromJson(sfNumber, "-0.0") == STNumber(sfNumber, 0));
        CHECK(numberFromJson(sfNumber, "-0.000") == STNumber(sfNumber, 0));
        CHECK(numberFromJson(sfNumber, "0e6") == STNumber(sfNumber, 0));
        CHECK(numberFromJson(sfNumber, "0.0e6") == STNumber(sfNumber, 0));
        CHECK(numberFromJson(sfNumber, "0.000e6") == STNumber(sfNumber, 0));
        CHECK(numberFromJson(sfNumber, "-0e6") == STNumber(sfNumber, 0));
        CHECK(numberFromJson(sfNumber, "-0.0e6") == STNumber(sfNumber, 0));
        CHECK(numberFromJson(sfNumber, "-0.000e6") == STNumber(sfNumber, 0));
    }

    SUBCASE("JSON parsing - limits")
    {
        constexpr auto imin = std::numeric_limits<int>::min();
        CHECK(
            numberFromJson(sfNumber, imin) ==
            STNumber(sfNumber, Number(imin, 0)));
        CHECK(
            numberFromJson(sfNumber, std::to_string(imin)) ==
            STNumber(sfNumber, Number(imin, 0)));

        constexpr auto imax = std::numeric_limits<int>::max();
        CHECK(
            numberFromJson(sfNumber, imax) ==
            STNumber(sfNumber, Number(imax, 0)));
        CHECK(
            numberFromJson(sfNumber, std::to_string(imax)) ==
            STNumber(sfNumber, Number(imax, 0)));

        constexpr auto umax = std::numeric_limits<unsigned int>::max();
        CHECK(
            numberFromJson(sfNumber, umax) ==
            STNumber(sfNumber, Number(umax, 0)));
        CHECK(
            numberFromJson(sfNumber, std::to_string(umax)) ==
            STNumber(sfNumber, Number(umax, 0)));
    }

    SUBCASE("JSON parsing - error cases")
    {
        // Empty string
        CHECK_THROWS_AS(
            numberFromJson(sfNumber, ""), std::runtime_error);

        // Just 'e'
        CHECK_THROWS_AS(
            numberFromJson(sfNumber, "e"), std::runtime_error);

        // Incomplete exponent
        CHECK_THROWS_AS(
            numberFromJson(sfNumber, "1e"), std::runtime_error);

        // Invalid exponent
        CHECK_THROWS_AS(
            numberFromJson(sfNumber, "e2"), std::runtime_error);

        // Null JSON value
        CHECK_THROWS_AS(
            numberFromJson(sfNumber, Json::Value()), std::runtime_error);

        // Too large number
        CHECK_THROWS_AS(
            numberFromJson(
                sfNumber,
                "1234567890123456789012345678901234567890123456789012345678"
                "9012345678901234567890123456789012345678901234567890123456"
                "78901234567890123456789012345678901234567890"),
            std::bad_cast);

        // Leading zeros not allowed
        CHECK_THROWS_AS(
            numberFromJson(sfNumber, "001"), std::runtime_error);

        CHECK_THROWS_AS(
            numberFromJson(sfNumber, "000.0"), std::runtime_error);

        // Dangling dot not allowed
        CHECK_THROWS_AS(
            numberFromJson(sfNumber, ".1"), std::runtime_error);

        CHECK_THROWS_AS(
            numberFromJson(sfNumber, "1."), std::runtime_error);

        CHECK_THROWS_AS(
            numberFromJson(sfNumber, "1.e3"), std::runtime_error);
    }
}

TEST_SUITE_END();

}  // namespace xrpl

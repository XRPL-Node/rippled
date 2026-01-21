#include <xrpl/protocol/IOUAmount.h>

#include <doctest/doctest.h>

using namespace xrpl;

TEST_SUITE_BEGIN("IOUAmount");

TEST_CASE("zero")
{
    IOUAmount const z(0, 0);

    CHECK_EQ(z.mantissa(), 0);
    CHECK_EQ(z.exponent(), -100);
    CHECK_FALSE(z);
    CHECK_EQ(z.signum(), 0);
    CHECK_EQ(z, beast::zero);

    CHECK_EQ((z + z), z);
    CHECK_EQ((z - z), z);
    CHECK_EQ(z, -z);

    IOUAmount const zz(beast::zero);
    CHECK_EQ(z, zz);

    // https://github.com/XRPLF/rippled/issues/5170
    IOUAmount const zzz{};
    CHECK_EQ(zzz, beast::zero);
}

TEST_CASE("signum")
{
    IOUAmount const neg(-1, 0);
    CHECK_LT(neg.signum(), 0);

    IOUAmount const zer(0, 0);
    CHECK_EQ(zer.signum(), 0);

    IOUAmount const pos(1, 0);
    CHECK_GT(pos.signum(), 0);
}

TEST_CASE("beast::Zero Comparisons")
{
    using beast::zero;

    {
        IOUAmount z(zero);
        CHECK_EQ(z, zero);
        CHECK_GE(z, zero);
        CHECK_LE(z, zero);
        CHECK_FALSE(z != zero);
        CHECK_FALSE(z > zero);
        CHECK_FALSE(z < zero);
    }

    {
        IOUAmount const neg(-2, 0);
        CHECK_LT(neg, zero);
        CHECK_LE(neg, zero);
        CHECK_NE(neg, zero);
        CHECK_FALSE(neg == zero);
    }

    {
        IOUAmount const pos(2, 0);
        CHECK_GT(pos, zero);
        CHECK_GE(pos, zero);
        CHECK_NE(pos, zero);
        CHECK_FALSE(pos == zero);
    }
}

TEST_CASE("IOU Comparisons")
{
    IOUAmount const n(-2, 0);
    IOUAmount const z(0, 0);
    IOUAmount const p(2, 0);

    CHECK_EQ(z, z);
    CHECK_GE(z, z);
    CHECK_LE(z, z);
    CHECK_EQ(z, -z);
    CHECK_FALSE(z > z);
    CHECK_FALSE(z < z);
    CHECK_FALSE(z != z);
    CHECK_FALSE(z != -z);

    CHECK_LT(n, z);
    CHECK_LE(n, z);
    CHECK_NE(n, z);
    CHECK_FALSE(n > z);
    CHECK_FALSE(n >= z);
    CHECK_FALSE(n == z);

    CHECK_GT(p, z);
    CHECK_GE(p, z);
    CHECK_NE(p, z);
    CHECK_FALSE(p < z);
    CHECK_FALSE(p <= z);
    CHECK_FALSE(p == z);

    CHECK_LT(n, p);
    CHECK_LE(n, p);
    CHECK_NE(n, p);
    CHECK_FALSE(n > p);
    CHECK_FALSE(n >= p);
    CHECK_FALSE(n == p);

    CHECK_GT(p, n);
    CHECK_GE(p, n);
    CHECK_NE(p, n);
    CHECK_FALSE(p < n);
    CHECK_FALSE(p <= n);
    CHECK_FALSE(p == n);

    CHECK_GT(p, -p);
    CHECK_GE(p, -p);
    CHECK_NE(p, -p);

    CHECK_LT(n, -n);
    CHECK_LE(n, -n);
    CHECK_NE(n, -n);
}

TEST_CASE("IOU strings")
{
    CHECK_EQ(to_string(IOUAmount(-2, 0)), "-2");
    CHECK_EQ(to_string(IOUAmount(0, 0)), "0");
    CHECK_EQ(to_string(IOUAmount(2, 0)), "2");
    CHECK_EQ(to_string(IOUAmount(25, -3)), "0.025");
    CHECK_EQ(to_string(IOUAmount(-25, -3)), "-0.025");
    CHECK_EQ(to_string(IOUAmount(25, 1)), "250");
    CHECK_EQ(to_string(IOUAmount(-25, 1)), "-250");
    CHECK_EQ(to_string(IOUAmount(2, 20)), "2000000000000000e5");
    CHECK_EQ(to_string(IOUAmount(-2, -20)), "-2000000000000000e-35");
}

TEST_CASE("mulRatio")
{
    /* The range for the mantissa when normalized */
    constexpr std::int64_t minMantissa = 1000000000000000ull;
    constexpr std::int64_t maxMantissa = 9999999999999999ull;
    /* The range for the exponent when normalized */
    constexpr int minExponent = -96;
    constexpr int maxExponent = 80;
    constexpr auto maxUInt = std::numeric_limits<std::uint32_t>::max();

    {
        // multiply by a number that would overflow the mantissa, then
        // divide by the same number, and check we didn't lose any value
        IOUAmount bigMan(maxMantissa, 0);
        CHECK_EQ(bigMan, mulRatio(bigMan, maxUInt, maxUInt, true));
        // rounding mode shouldn't matter as the result is exact
        CHECK_EQ(bigMan, mulRatio(bigMan, maxUInt, maxUInt, false));
    }
    {
        // Similar test as above, but for negative values
        IOUAmount bigMan(-maxMantissa, 0);
        CHECK_EQ(bigMan, mulRatio(bigMan, maxUInt, maxUInt, true));
        // rounding mode shouldn't matter as the result is exact
        CHECK_EQ(bigMan, mulRatio(bigMan, maxUInt, maxUInt, false));
    }

    {
        // small amounts
        IOUAmount tiny(minMantissa, minExponent);
        // Round up should give the smallest allowable number
        CHECK_EQ(tiny, mulRatio(tiny, 1, maxUInt, true));
        CHECK_EQ(tiny, mulRatio(tiny, maxUInt - 1, maxUInt, true));
        // rounding down should be zero
        CHECK_EQ(beast::zero, mulRatio(tiny, 1, maxUInt, false));
        CHECK_EQ(beast::zero, mulRatio(tiny, maxUInt - 1, maxUInt, false));

        // tiny negative numbers
        IOUAmount tinyNeg(-minMantissa, minExponent);
        // Round up should give zero
        CHECK_EQ(beast::zero, mulRatio(tinyNeg, 1, maxUInt, true));
        CHECK_EQ(beast::zero, mulRatio(tinyNeg, maxUInt - 1, maxUInt, true));
        // rounding down should be tiny
        CHECK_EQ(tinyNeg, mulRatio(tinyNeg, 1, maxUInt, false));
        CHECK_EQ(tinyNeg, mulRatio(tinyNeg, maxUInt - 1, maxUInt, false));
    }

    {  // rounding
        {
            IOUAmount one(1, 0);
            auto const rup = mulRatio(one, maxUInt - 1, maxUInt, true);
            auto const rdown = mulRatio(one, maxUInt - 1, maxUInt, false);
            CHECK_EQ(rup.mantissa() - rdown.mantissa(), 1);
        }
        {
            IOUAmount big(maxMantissa, maxExponent);
            auto const rup = mulRatio(big, maxUInt - 1, maxUInt, true);
            auto const rdown = mulRatio(big, maxUInt - 1, maxUInt, false);
            CHECK_EQ(rup.mantissa() - rdown.mantissa(), 1);
        }

        {
            IOUAmount negOne(-1, 0);
            auto const rup = mulRatio(negOne, maxUInt - 1, maxUInt, true);
            auto const rdown = mulRatio(negOne, maxUInt - 1, maxUInt, false);
            CHECK_EQ(rup.mantissa() - rdown.mantissa(), 1);
        }
    }

    {
        // division by zero
        IOUAmount one(1, 0);
        CHECK_THROWS([&] { mulRatio(one, 1, 0, true); }());
    }

    {
        // overflow
        IOUAmount big(maxMantissa, maxExponent);
        CHECK_THROWS([&] { mulRatio(big, 2, 0, true); }());
    }
}

TEST_SUITE_END();

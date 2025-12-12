#include <xrpl/protocol/IOUAmount.h>

#include <doctest/doctest.h>

using xrpl::IOUAmount;

namespace ripple {

TEST_SUITE_BEGIN("basics");

TEST_CASE("IOUAmount")
{
    SUBCASE("zero")
    {
        IOUAmount const z(0, 0);

        CHECK(z.mantissa() == 0);
        CHECK(z.exponent() == -100);
        CHECK_FALSE(z);
        CHECK(z.signum() == 0);
        CHECK(z == beast::zero);

        CHECK((z + z) == z);
        CHECK((z - z) == z);
        CHECK(z == -z);

        IOUAmount const zz(beast::zero);
        CHECK(z == zz);

        // https://github.com/XRPLF/rippled/issues/5170
        IOUAmount const zzz{};
        CHECK(zzz == beast::zero);
        // CHECK(zzz == zz);
    }

    SUBCASE("signum")
    {
        IOUAmount const neg(-1, 0);
        CHECK(neg.signum() < 0);

        IOUAmount const zer(0, 0);
        CHECK(zer.signum() == 0);

        IOUAmount const pos(1, 0);
        CHECK(pos.signum() > 0);
    }

    SUBCASE("beast::Zero Comparisons")
    {
        using beast::zero;

        {
            IOUAmount z(zero);
            CHECK(z == zero);
            CHECK(z >= zero);
            CHECK(z <= zero);
            CHECK(!(z != zero));
            CHECK(!(z > zero));
            CHECK(!(z < zero));
        }

        {
            IOUAmount const neg(-2, 0);
            CHECK(neg < zero);
            CHECK(neg <= zero);
            CHECK(neg != zero);
            CHECK(!(neg == zero));
        }

        {
            IOUAmount const pos(2, 0);
            CHECK(pos > zero);
            CHECK(pos >= zero);
            CHECK(pos != zero);
            CHECK(!(pos == zero));
        }
    }

    SUBCASE("IOU Comparisons")
    {
        IOUAmount const n(-2, 0);
        IOUAmount const z(0, 0);
        IOUAmount const p(2, 0);

        CHECK(z == z);
        CHECK(z >= z);
        CHECK(z <= z);
        CHECK(z == -z);
        CHECK(!(z > z));
        CHECK(!(z < z));
        CHECK(!(z != z));
        CHECK(!(z != -z));

        CHECK(n < z);
        CHECK(n <= z);
        CHECK(n != z);
        CHECK(!(n > z));
        CHECK(!(n >= z));
        CHECK(!(n == z));

        CHECK(p > z);
        CHECK(p >= z);
        CHECK(p != z);
        CHECK(!(p < z));
        CHECK(!(p <= z));
        CHECK(!(p == z));

        CHECK(n < p);
        CHECK(n <= p);
        CHECK(n != p);
        CHECK(!(n > p));
        CHECK(!(n >= p));
        CHECK(!(n == p));

        CHECK(p > n);
        CHECK(p >= n);
        CHECK(p != n);
        CHECK(!(p < n));
        CHECK(!(p <= n));
        CHECK(!(p == n));

        CHECK(p > -p);
        CHECK(p >= -p);
        CHECK(p != -p);

        CHECK(n < -n);
        CHECK(n <= -n);
        CHECK(n != -n);
    }

    SUBCASE("IOU strings")
    {
        CHECK(to_string(IOUAmount(-2, 0)) == "-2");
        CHECK(to_string(IOUAmount(0, 0)) == "0");
        CHECK(to_string(IOUAmount(2, 0)) == "2");
        CHECK(to_string(IOUAmount(25, -3)) == "0.025");
        CHECK(to_string(IOUAmount(-25, -3)) == "-0.025");
        CHECK(to_string(IOUAmount(25, 1)) == "250");
        CHECK(to_string(IOUAmount(-25, 1)) == "-250");
        CHECK(to_string(IOUAmount(2, 20)) == "2000000000000000e5");
        CHECK(to_string(IOUAmount(-2, -20)) == "-2000000000000000e-35");
    }

    SUBCASE("mulRatio")
    {
        /* The range for the mantissa when normalized */
        constexpr std::int64_t minMantissa = 1000000000000000ull;
        constexpr std::int64_t maxMantissa = 9999999999999999ull;
        // log(2,maxMantissa) ~ 53.15
        /* The range for the exponent when normalized */
        constexpr int minExponent = -96;
        constexpr int maxExponent = 80;
        constexpr auto maxUInt = std::numeric_limits<std::uint32_t>::max();

        {
            // multiply by a number that would overflow the mantissa, then
            // divide by the same number, and check we didn't lose any value
            IOUAmount bigMan(maxMantissa, 0);
            CHECK(bigMan == mulRatio(bigMan, maxUInt, maxUInt, true));
            // rounding mode shouldn't matter as the result is exact
            CHECK(bigMan == mulRatio(bigMan, maxUInt, maxUInt, false));
        }
        {
            // Similar test as above, but for negative values
            IOUAmount bigMan(-maxMantissa, 0);
            CHECK(bigMan == mulRatio(bigMan, maxUInt, maxUInt, true));
            // rounding mode shouldn't matter as the result is exact
            CHECK(bigMan == mulRatio(bigMan, maxUInt, maxUInt, false));
        }

        {
            // small amounts
            IOUAmount tiny(minMantissa, minExponent);
            // Round up should give the smallest allowable number
            CHECK(tiny == mulRatio(tiny, 1, maxUInt, true));
            CHECK(tiny == mulRatio(tiny, maxUInt - 1, maxUInt, true));
            // rounding down should be zero
            CHECK(beast::zero == mulRatio(tiny, 1, maxUInt, false));
            CHECK(
                beast::zero == mulRatio(tiny, maxUInt - 1, maxUInt, false));

            // tiny negative numbers
            IOUAmount tinyNeg(-minMantissa, minExponent);
            // Round up should give zero
            CHECK(beast::zero == mulRatio(tinyNeg, 1, maxUInt, true));
            CHECK(
                beast::zero == mulRatio(tinyNeg, maxUInt - 1, maxUInt, true));
            // rounding down should be tiny
            CHECK(tinyNeg == mulRatio(tinyNeg, 1, maxUInt, false));
            CHECK(
                tinyNeg == mulRatio(tinyNeg, maxUInt - 1, maxUInt, false));
        }

        {  // rounding
            {
                IOUAmount one(1, 0);
                auto const rup = mulRatio(one, maxUInt - 1, maxUInt, true);
                auto const rdown = mulRatio(one, maxUInt - 1, maxUInt, false);
                CHECK(rup.mantissa() - rdown.mantissa() == 1);
            }
            {
                IOUAmount big(maxMantissa, maxExponent);
                auto const rup = mulRatio(big, maxUInt - 1, maxUInt, true);
                auto const rdown = mulRatio(big, maxUInt - 1, maxUInt, false);
                CHECK(rup.mantissa() - rdown.mantissa() == 1);
            }

            {
                IOUAmount negOne(-1, 0);
                auto const rup = mulRatio(negOne, maxUInt - 1, maxUInt, true);
                auto const rdown =
                    mulRatio(negOne, maxUInt - 1, maxUInt, false);
                CHECK(rup.mantissa() - rdown.mantissa() == 1);
            }
        }

        {
            // division by zero
            IOUAmount one(1, 0);
            CHECK_THROWS(mulRatio(one, 1, 0, true));
        }

        {
            // overflow
            IOUAmount big(maxMantissa, maxExponent);
            CHECK_THROWS(mulRatio(big, 2, 0, true));
        }
    }
}

TEST_SUITE_END();

}  // namespace ripple

#include <xrpl/protocol/XRPAmount.h>

#include <doctest/doctest.h>

using namespace xrpl;

TEST_SUITE_BEGIN("XRPAmount");

TEST_CASE("signum")
{
    for (auto i : {-1, 0, 1})
    {
        XRPAmount const x(i);

        if (i < 0)
            CHECK_LT(x.signum(), 0);
        else if (i > 0)
            CHECK_GT(x.signum(), 0);
        else
            CHECK_EQ(x.signum(), 0);
    }
}

TEST_CASE("beast::Zero Comparisons")
{
    using beast::zero;

    for (auto i : {-1, 0, 1})
    {
        XRPAmount const x(i);

        CHECK_EQ((i == 0), (x == zero));
        CHECK_EQ((i != 0), (x != zero));
        CHECK_EQ((i < 0), (x < zero));
        CHECK_EQ((i > 0), (x > zero));
        CHECK_EQ((i <= 0), (x <= zero));
        CHECK_EQ((i >= 0), (x >= zero));

        CHECK_EQ((0 == i), (zero == x));
        CHECK_EQ((0 != i), (zero != x));
        CHECK_EQ((0 < i), (zero < x));
        CHECK_EQ((0 > i), (zero > x));
        CHECK_EQ((0 <= i), (zero <= x));
        CHECK_EQ((0 >= i), (zero >= x));
    }
}

TEST_CASE("XRP Comparisons")
{
    for (auto i : {-1, 0, 1})
    {
        XRPAmount const x(i);

        for (auto j : {-1, 0, 1})
        {
            XRPAmount const y(j);

            CHECK_EQ((i == j), (x == y));
            CHECK_EQ((i != j), (x != y));
            CHECK_EQ((i < j), (x < y));
            CHECK_EQ((i > j), (x > y));
            CHECK_EQ((i <= j), (x <= y));
            CHECK_EQ((i >= j), (x >= y));
        }
    }
}

TEST_CASE("Addition & Subtraction")
{
    for (auto i : {-1, 0, 1})
    {
        XRPAmount const x(i);

        for (auto j : {-1, 0, 1})
        {
            XRPAmount const y(j);

            CHECK_EQ(XRPAmount(i + j), (x + y));
            CHECK_EQ(XRPAmount(i - j), (x - y));

            CHECK_EQ((x + y), (y + x));  // addition is commutative
        }
    }
}

TEST_CASE("decimalXRP")
{
    // Tautology
    CHECK_EQ(DROPS_PER_XRP.decimalXRP(), 1);

    XRPAmount test{1};
    CHECK_EQ(test.decimalXRP(), 0.000001);

    test = -test;
    CHECK_EQ(test.decimalXRP(), -0.000001);

    test = 100'000'000;
    CHECK_EQ(test.decimalXRP(), 100);

    test = -test;
    CHECK_EQ(test.decimalXRP(), -100);
}

TEST_CASE("functions")
{
    // Explicitly test every defined function for the XRPAmount class
    // since some of them are templated, but not used anywhere else.
    auto make = [&](auto x) -> XRPAmount { return XRPAmount{x}; };

    XRPAmount defaulted;
    (void)defaulted;
    XRPAmount test{0};
    CHECK_EQ(test.drops(), 0);

    test = make(beast::zero);
    CHECK_EQ(test.drops(), 0);

    test = beast::zero;
    CHECK_EQ(test.drops(), 0);

    test = make(100);
    CHECK_EQ(test.drops(), 100);

    test = make(100u);
    CHECK_EQ(test.drops(), 100);

    XRPAmount const targetSame{200u};
    test = make(targetSame);
    CHECK_EQ(test.drops(), 200);
    CHECK_EQ(test, targetSame);
    CHECK_LT(test, XRPAmount{1000});
    CHECK_GT(test, XRPAmount{100});

    test = std::int64_t(200);
    CHECK_EQ(test.drops(), 200);
    test = std::uint32_t(300);
    CHECK_EQ(test.drops(), 300);

    test = targetSame;
    CHECK_EQ(test.drops(), 200);
    auto testOther = test.dropsAs<std::uint32_t>();
    CHECK_UNARY(testOther);
    CHECK_EQ(*testOther, 200);
    test = std::numeric_limits<std::uint64_t>::max();
    testOther = test.dropsAs<std::uint32_t>();
    CHECK_FALSE(testOther);
    test = -1;
    testOther = test.dropsAs<std::uint32_t>();
    CHECK_FALSE(testOther);

    test = targetSame * 2;
    CHECK_EQ(test.drops(), 400);
    test = 3 * targetSame;
    CHECK_EQ(test.drops(), 600);
    test = 20;
    CHECK_EQ(test.drops(), 20);

    test += targetSame;
    CHECK_EQ(test.drops(), 220);

    test -= targetSame;
    CHECK_EQ(test.drops(), 20);

    test *= 5;
    CHECK_EQ(test.drops(), 100);
    test = 50;
    CHECK_EQ(test.drops(), 50);
    test -= 39;
    CHECK_EQ(test.drops(), 11);

    // legal with signed
    test = -test;
    CHECK_EQ(test.drops(), -11);
    CHECK_EQ(test.signum(), -1);
    CHECK_EQ(to_string(test), "-11");

    CHECK_UNARY(test);
    test = 0;
    CHECK_FALSE(test);
    CHECK_EQ(test.signum(), 0);
    test = targetSame;
    CHECK_EQ(test.signum(), 1);
    CHECK_EQ(to_string(test), "200");
}

TEST_CASE("mulRatio")
{
    constexpr auto maxUInt32 = std::numeric_limits<std::uint32_t>::max();
    constexpr auto maxXRP = std::numeric_limits<XRPAmount::value_type>::max();
    constexpr auto minXRP = std::numeric_limits<XRPAmount::value_type>::min();

    {
        // multiply by a number that would overflow then divide by the same
        // number, and check we didn't lose any value
        XRPAmount big(maxXRP);
        CHECK_EQ(big, mulRatio(big, maxUInt32, maxUInt32, true));
        // rounding mode shouldn't matter as the result is exact
        CHECK_EQ(big, mulRatio(big, maxUInt32, maxUInt32, false));

        // multiply and divide by values that would overflow if done
        // naively, and check that it gives the correct answer
        big -= 0xf;  // Subtract a little so it's divisable by 4
        CHECK_EQ(mulRatio(big, 3, 4, false).value(), (big.value() / 4) * 3);
        CHECK_EQ(mulRatio(big, 3, 4, true).value(), (big.value() / 4) * 3);
        CHECK_NE((big.value() * 3) / 4, (big.value() / 4) * 3);
    }

    {
        // Similar test as above, but for negative values
        XRPAmount big(minXRP);
        CHECK_EQ(big, mulRatio(big, maxUInt32, maxUInt32, true));
        // rounding mode shouldn't matter as the result is exact
        CHECK_EQ(big, mulRatio(big, maxUInt32, maxUInt32, false));

        // multiply and divide by values that would overflow if done
        // naively, and check that it gives the correct answer
        CHECK_EQ(mulRatio(big, 3, 4, false).value(), (big.value() / 4) * 3);
        CHECK_EQ(mulRatio(big, 3, 4, true).value(), (big.value() / 4) * 3);
        CHECK_NE((big.value() * 3) / 4, (big.value() / 4) * 3);
    }

    {
        // small amounts
        XRPAmount tiny(1);
        // Round up should give the smallest allowable number
        CHECK_EQ(tiny, mulRatio(tiny, 1, maxUInt32, true));
        // rounding down should be zero
        CHECK_EQ(beast::zero, mulRatio(tiny, 1, maxUInt32, false));
        CHECK_EQ(beast::zero, mulRatio(tiny, maxUInt32 - 1, maxUInt32, false));

        // tiny negative numbers
        XRPAmount tinyNeg(-1);
        // Round up should give zero
        CHECK_EQ(beast::zero, mulRatio(tinyNeg, 1, maxUInt32, true));
        CHECK_EQ(
            beast::zero, mulRatio(tinyNeg, maxUInt32 - 1, maxUInt32, true));
        // rounding down should be tiny
        CHECK_EQ(tinyNeg, mulRatio(tinyNeg, maxUInt32 - 1, maxUInt32, false));
    }

    {  // rounding
        {
            XRPAmount one(1);
            auto const rup = mulRatio(one, maxUInt32 - 1, maxUInt32, true);
            auto const rdown = mulRatio(one, maxUInt32 - 1, maxUInt32, false);
            CHECK_EQ(rup.drops() - rdown.drops(), 1);
        }

        {
            XRPAmount big(maxXRP);
            auto const rup = mulRatio(big, maxUInt32 - 1, maxUInt32, true);
            auto const rdown = mulRatio(big, maxUInt32 - 1, maxUInt32, false);
            CHECK_EQ(rup.drops() - rdown.drops(), 1);
        }

        {
            XRPAmount negOne(-1);
            auto const rup = mulRatio(negOne, maxUInt32 - 1, maxUInt32, true);
            auto const rdown =
                mulRatio(negOne, maxUInt32 - 1, maxUInt32, false);
            CHECK_EQ(rup.drops() - rdown.drops(), 1);
        }
    }

    {
        // division by zero
        XRPAmount one(1);
        CHECK_THROWS([&] { mulRatio(one, 1, 0, true); }());
    }

    {
        // overflow
        XRPAmount big(maxXRP);
        CHECK_THROWS([&] { mulRatio(big, 2, 1, true); }());
    }

    {
        // underflow
        XRPAmount bigNegative(minXRP + 10);
        CHECK_EQ(mulRatio(bigNegative, 2, 1, true), minXRP);
    }
}

TEST_SUITE_END();

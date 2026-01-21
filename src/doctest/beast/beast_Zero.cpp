#include <xrpl/beast/utility/Zero.h>

#include <doctest/doctest.h>

namespace beast {

struct adl_tester
{
};

int
signum(adl_tester)
{
    return 0;
}

namespace inner_adl_test {

struct adl_tester2
{
};

int
signum(adl_tester2)
{
    return 0;
}

}  // namespace inner_adl_test

}  // namespace beast

using namespace beast;

TEST_SUITE_BEGIN("Zero");

namespace {

struct IntegerWrapper
{
    int value;

    IntegerWrapper(int v) : value(v)
    {
    }

    int
    signum() const
    {
        return value;
    }
};

void
test_lhs_zero(IntegerWrapper x)
{
    CHECK_EQ((x >= zero), (x.signum() >= 0));
    CHECK_EQ((x > zero), (x.signum() > 0));
    CHECK_EQ((x == zero), (x.signum() == 0));
    CHECK_EQ((x != zero), (x.signum() != 0));
    CHECK_EQ((x < zero), (x.signum() < 0));
    CHECK_EQ((x <= zero), (x.signum() <= 0));
}

void
test_rhs_zero(IntegerWrapper x)
{
    CHECK_EQ((zero >= x), (0 >= x.signum()));
    CHECK_EQ((zero > x), (0 > x.signum()));
    CHECK_EQ((zero == x), (0 == x.signum()));
    CHECK_EQ((zero != x), (0 != x.signum()));
    CHECK_EQ((zero < x), (0 < x.signum()));
    CHECK_EQ((zero <= x), (0 <= x.signum()));
}

}  // namespace

TEST_CASE("lhs zero")
{
    test_lhs_zero(-7);
    test_lhs_zero(0);
    test_lhs_zero(32);
}

TEST_CASE("rhs zero")
{
    test_rhs_zero(-4);
    test_rhs_zero(0);
    test_rhs_zero(64);
}

TEST_CASE("ADL")
{
    CHECK_EQ(adl_tester{}, zero);
    CHECK_EQ(inner_adl_test::adl_tester2{}, zero);
}

TEST_SUITE_END();

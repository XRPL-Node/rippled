#include <xrpl/basics/Number.h>
#include <xrpl/protocol/IOUAmount.h>
#include <xrpl/protocol/STAmount.h>

#include <doctest/doctest.h>

#include <sstream>
#include <tuple>

using namespace xrpl;

TEST_SUITE_BEGIN("Number");

TEST_CASE("zero")
{
    Number const z{0, 0};

    CHECK_EQ(z.mantissa(), 0);
    CHECK_EQ(z.exponent(), Number{}.exponent());

    CHECK_EQ((z + z), z);
    CHECK_EQ((z - z), z);
    CHECK_EQ(z, -z);
}

TEST_CASE("limits")
{
    bool caught = false;
    try
    {
        Number x{10'000'000'000'000'000, 32768};
    }
    catch (std::overflow_error const&)
    {
        caught = true;
    }
    CHECK_UNARY(caught);
    Number x{10'000'000'000'000'000, 32767};
    CHECK_EQ(x, Number{1'000'000'000'000'000, 32768});
    Number z{1'000'000'000'000'000, -32769};
    CHECK_EQ(z, Number{});
    Number y{1'000'000'000'000'001'500, 32000};
    CHECK_EQ(y, Number{1'000'000'000'000'002, 32003});
    Number m{std::numeric_limits<std::int64_t>::min()};
    CHECK_EQ(m, Number{-9'223'372'036'854'776, 3});
    Number M{std::numeric_limits<std::int64_t>::max()};
    CHECK_EQ(M, Number{9'223'372'036'854'776, 3});
    caught = false;
    try
    {
        Number q{99'999'999'999'999'999, 32767};
    }
    catch (std::overflow_error const&)
    {
        caught = true;
    }
    CHECK_UNARY(caught);
}

TEST_CASE("add")
{
    using Case = std::tuple<Number, Number, Number>;
    Case c[]{
        {Number{1'000'000'000'000'000, -15},
         Number{6'555'555'555'555'555, -29},
         Number{1'000'000'000'000'066, -15}},
        {Number{-1'000'000'000'000'000, -15},
         Number{-6'555'555'555'555'555, -29},
         Number{-1'000'000'000'000'066, -15}},
        {Number{-1'000'000'000'000'000, -15},
         Number{6'555'555'555'555'555, -29},
         Number{-9'999'999'999'999'344, -16}},
        {Number{-6'555'555'555'555'555, -29},
         Number{1'000'000'000'000'000, -15},
         Number{9'999'999'999'999'344, -16}},
        {Number{}, Number{5}, Number{5}},
        {Number{5'555'555'555'555'555, -32768},
         Number{-5'555'555'555'555'554, -32768},
         Number{0}},
        {Number{-9'999'999'999'999'999, -31},
         Number{1'000'000'000'000'000, -15},
         Number{9'999'999'999'999'990, -16}}};
    for (auto const& [x, y, z] : c)
        CHECK_EQ(x + y, z);
    bool caught = false;
    try
    {
        Number{9'999'999'999'999'999, 32768} +
            Number{5'000'000'000'000'000, 32767};
    }
    catch (std::overflow_error const&)
    {
        caught = true;
    }
    CHECK_UNARY(caught);
}

TEST_CASE("sub")
{
    using Case = std::tuple<Number, Number, Number>;
    Case c[]{
        {Number{1'000'000'000'000'000, -15},
         Number{6'555'555'555'555'555, -29},
         Number{9'999'999'999'999'344, -16}},
        {Number{6'555'555'555'555'555, -29},
         Number{1'000'000'000'000'000, -15},
         Number{-9'999'999'999'999'344, -16}},
        {Number{1'000'000'000'000'000, -15},
         Number{1'000'000'000'000'000, -15},
         Number{0}},
        {Number{1'000'000'000'000'000, -15},
         Number{1'000'000'000'000'001, -15},
         Number{-1'000'000'000'000'000, -30}},
        {Number{1'000'000'000'000'001, -15},
         Number{1'000'000'000'000'000, -15},
         Number{1'000'000'000'000'000, -30}}};
    for (auto const& [x, y, z] : c)
        CHECK_EQ(x - y, z);
}

TEST_CASE("mul")
{
    using Case = std::tuple<Number, Number, Number>;
    saveNumberRoundMode save{Number::setround(Number::to_nearest)};
    {
        Case c[]{
            {Number{7}, Number{8}, Number{56}},
            {Number{1414213562373095, -15},
             Number{1414213562373095, -15},
             Number{2000000000000000, -15}},
            {Number{-1414213562373095, -15},
             Number{1414213562373095, -15},
             Number{-2000000000000000, -15}},
            {Number{-1414213562373095, -15},
             Number{-1414213562373095, -15},
             Number{2000000000000000, -15}},
            {Number{3214285714285706, -15},
             Number{3111111111111119, -15},
             Number{1000000000000000, -14}},
            {Number{1000000000000000, -32768},
             Number{1000000000000000, -32768},
             Number{0}}};
        for (auto const& [x, y, z] : c)
            CHECK_EQ(x * y, z);
    }
    Number::setround(Number::towards_zero);
    {
        Case c[]{
            {Number{7}, Number{8}, Number{56}},
            {Number{1414213562373095, -15},
             Number{1414213562373095, -15},
             Number{1999999999999999, -15}}};
        for (auto const& [x, y, z] : c)
            CHECK_EQ(x * y, z);
    }
    bool caught = false;
    try
    {
        Number{9'999'999'999'999'999, 32768} *
            Number{5'000'000'000'000'000, 32767};
    }
    catch (std::overflow_error const&)
    {
        caught = true;
    }
    CHECK_UNARY(caught);
}

TEST_CASE("div")
{
    using Case = std::tuple<Number, Number, Number>;
    saveNumberRoundMode save{Number::setround(Number::to_nearest)};
    {
        Case c[]{
            {Number{1}, Number{2}, Number{5, -1}},
            {Number{1}, Number{10}, Number{1, -1}},
            {Number{1}, Number{-10}, Number{-1, -1}},
            {Number{0}, Number{100}, Number{0}},
            {Number{1414213562373095, -10},
             Number{1414213562373095, -10},
             Number{1}},
            {Number{9'999'999'999'999'999},
             Number{1'000'000'000'000'000},
             Number{9'999'999'999'999'999, -15}},
            {Number{2}, Number{3}, Number{6'666'666'666'666'667, -16}},
            {Number{-2}, Number{3}, Number{-6'666'666'666'666'667, -16}}};
        for (auto const& [x, y, z] : c)
            CHECK_EQ(x / y, z);
    }
    bool caught = false;
    try
    {
        Number{1000000000000000, -15} / Number{0};
    }
    catch (std::overflow_error const&)
    {
        caught = true;
    }
    CHECK_UNARY(caught);
}

TEST_CASE("root")
{
    using Case = std::tuple<Number, unsigned, Number>;
    Case c[]{
        {Number{2}, 2, Number{1414213562373095, -15}},
        {Number{2'000'000}, 2, Number{1414213562373095, -12}},
        {Number{2, -30}, 2, Number{1414213562373095, -30}},
        {Number{-27}, 3, Number{-3}},
        {Number{1}, 5, Number{1}},
        {Number{-1}, 0, Number{1}},
        {Number{5, -1}, 0, Number{0}},
        {Number{0}, 5, Number{0}},
        {Number{5625, -4}, 2, Number{75, -2}}};
    for (auto const& [x, y, z] : c)
        CHECK_EQ(root(x, y), z);
    bool caught = false;
    try
    {
        (void)root(Number{-2}, 0);
    }
    catch (std::overflow_error const&)
    {
        caught = true;
    }
    CHECK_UNARY(caught);
    caught = false;
    try
    {
        (void)root(Number{-2}, 4);
    }
    catch (std::overflow_error const&)
    {
        caught = true;
    }
    CHECK_UNARY(caught);
}

TEST_CASE("power1")
{
    using Case = std::tuple<Number, unsigned, Number>;
    Case c[]{
        {Number{64}, 0, Number{1}},
        {Number{64}, 1, Number{64}},
        {Number{64}, 2, Number{4096}},
        {Number{-64}, 2, Number{4096}},
        {Number{64}, 3, Number{262144}},
        {Number{-64}, 3, Number{-262144}}};
    for (auto const& [x, y, z] : c)
        CHECK_EQ(power(x, y), z);
}

TEST_CASE("power2")
{
    using Case = std::tuple<Number, unsigned, unsigned, Number>;
    Case c[]{
        {Number{1}, 3, 7, Number{1}},
        {Number{-1}, 1, 0, Number{1}},
        {Number{-1, -1}, 1, 0, Number{0}},
        {Number{16}, 0, 5, Number{1}},
        {Number{34}, 3, 3, Number{34}},
        {Number{4}, 3, 2, Number{8}}};
    for (auto const& [x, n, d, z] : c)
        CHECK_EQ(power(x, n, d), z);
    bool caught = false;
    try
    {
        (void)power(Number{7}, 0, 0);
    }
    catch (std::overflow_error const&)
    {
        caught = true;
    }
    CHECK_UNARY(caught);
    caught = false;
    try
    {
        (void)power(Number{7}, 1, 0);
    }
    catch (std::overflow_error const&)
    {
        caught = true;
    }
    CHECK_UNARY(caught);
    caught = false;
    try
    {
        (void)power(Number{-1, -1}, 3, 2);
    }
    catch (std::overflow_error const&)
    {
        caught = true;
    }
    CHECK_UNARY(caught);
}

TEST_CASE("conversions")
{
    IOUAmount x{5, 6};
    Number y = x;
    CHECK_EQ(y, Number{5, 6});
    IOUAmount z{y};
    CHECK_EQ(x, z);
    XRPAmount xrp{500};
    STAmount st = xrp;
    Number n = st;
    CHECK_EQ(XRPAmount{n}, xrp);
    IOUAmount x0{0, 0};
    Number y0 = x0;
    CHECK_EQ(y0, Number{0});
    IOUAmount z0{y0};
    CHECK_EQ(x0, z0);
    XRPAmount xrp0{0};
    Number n0 = xrp0;
    CHECK_EQ(n0, Number{0});
    XRPAmount xrp1{n0};
    CHECK_EQ(xrp1, xrp0);
}

TEST_CASE("squelch")
{
    Number limit{1, -6};
    CHECK_EQ(squelch(Number{2, -6}, limit), Number{2, -6});
    CHECK_EQ(squelch(Number{1, -6}, limit), Number{1, -6});
    CHECK_EQ(squelch(Number{9, -7}, limit), Number{0});
    CHECK_EQ(squelch(Number{-2, -6}, limit), Number{-2, -6});
    CHECK_EQ(squelch(Number{-1, -6}, limit), Number{-1, -6});
    CHECK_EQ(squelch(Number{-9, -7}, limit), Number{0});
}

TEST_CASE("toString")
{
    CHECK_EQ(to_string(Number(-2, 0)), "-2");
    CHECK_EQ(to_string(Number(0, 0)), "0");
    CHECK_EQ(to_string(Number(2, 0)), "2");
    CHECK_EQ(to_string(Number(25, -3)), "0.025");
    CHECK_EQ(to_string(Number(-25, -3)), "-0.025");
    CHECK_EQ(to_string(Number(25, 1)), "250");
    CHECK_EQ(to_string(Number(-25, 1)), "-250");
    CHECK_EQ(to_string(Number(2, 20)), "2000000000000000e5");
    CHECK_EQ(to_string(Number(-2, -20)), "-2000000000000000e-35");
}

TEST_CASE("relationals")
{
    CHECK_FALSE(Number{100} < Number{10});
    CHECK_GT(Number{100}, Number{10});
    CHECK_GE(Number{100}, Number{10});
    CHECK_FALSE(Number{100} <= Number{10});
}

TEST_CASE("stream")
{
    Number x{100};
    std::ostringstream os;
    os << x;
    CHECK_EQ(os.str(), to_string(x));
}

TEST_CASE("inc_dec")
{
    Number x{100};
    Number y = +x;
    CHECK_EQ(x, y);
    CHECK_EQ(x++, y);
    CHECK_EQ(x, Number{101});
    CHECK_EQ(x--, Number{101});
    CHECK_EQ(x, y);
}

TEST_CASE("toSTAmount")
{
    NumberSO stNumberSO{true};
    Issue const issue;
    Number const n{7'518'783'80596, -5};
    saveNumberRoundMode const save{Number::setround(Number::to_nearest)};
    auto res2 = STAmount{issue, n.mantissa(), n.exponent()};
    CHECK_EQ(res2, STAmount{7518784});

    Number::setround(Number::towards_zero);
    res2 = STAmount{issue, n.mantissa(), n.exponent()};
    CHECK_EQ(res2, STAmount{7518783});

    Number::setround(Number::downward);
    res2 = STAmount{issue, n.mantissa(), n.exponent()};
    CHECK_EQ(res2, STAmount{7518783});

    Number::setround(Number::upward);
    res2 = STAmount{issue, n.mantissa(), n.exponent()};
    CHECK_EQ(res2, STAmount{7518784});
}

TEST_CASE("truncate")
{
    CHECK_EQ(Number(25, +1).truncate(), Number(250, 0));
    CHECK_EQ(Number(25, 0).truncate(), Number(25, 0));
    CHECK_EQ(Number(25, -1).truncate(), Number(2, 0));
    CHECK_EQ(Number(25, -2).truncate(), Number(0, 0));
    CHECK_EQ(Number(99, -2).truncate(), Number(0, 0));

    CHECK_EQ(Number(-25, +1).truncate(), Number(-250, 0));
    CHECK_EQ(Number(-25, 0).truncate(), Number(-25, 0));
    CHECK_EQ(Number(-25, -1).truncate(), Number(-2, 0));
    CHECK_EQ(Number(-25, -2).truncate(), Number(0, 0));
    CHECK_EQ(Number(-99, -2).truncate(), Number(0, 0));

    CHECK_EQ(Number(0, 0).truncate(), Number(0, 0));
    CHECK_EQ(Number(0, 30000).truncate(), Number(0, 0));
    CHECK_EQ(Number(0, -30000).truncate(), Number(0, 0));
    CHECK_EQ(Number(100, -30000).truncate(), Number(0, 0));
    CHECK_EQ(Number(100, -30000).truncate(), Number(0, 0));
    CHECK_EQ(Number(-100, -30000).truncate(), Number(0, 0));
    CHECK_EQ(Number(-100, -30000).truncate(), Number(0, 0));
}

TEST_SUITE_END();

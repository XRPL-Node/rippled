#include <xrpl/basics/Number.h>
#include <xrpl/protocol/IOUAmount.h>
#include <xrpl/protocol/STAmount.h>

#include <doctest/doctest.h>

#include <sstream>
#include <tuple>

using xrpl::IOUAmount;
using xrpl::Issue;
using xrpl::Number;
using xrpl::NumberRoundModeGuard;
using xrpl::NumberSO;
using xrpl::saveNumberRoundMode;
using xrpl::STAmount;
using xrpl::XRPAmount;

namespace ripple {

TEST_SUITE_BEGIN("basics");

TEST_CASE("Number - zero")
{
    Number const z{0, 0};

    CHECK(z.mantissa() == 0);
    CHECK(z.exponent() == Number{}.exponent());

    CHECK((z + z) == z);
    CHECK((z - z) == z);
    CHECK(z == -z);
}

TEST_CASE("Number - test_limits")
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
    CHECK(caught);
    Number x{10'000'000'000'000'000, 32767};
    CHECK((x == Number{1'000'000'000'000'000, 32768}));
    Number z{1'000'000'000'000'000, -32769};
    CHECK(z == Number{});
    Number y{1'000'000'000'000'001'500, 32000};
    CHECK((y == Number{1'000'000'000'000'002, 32003}));
    Number m{std::numeric_limits<std::int64_t>::min()};
    CHECK((m == Number{-9'223'372'036'854'776, 3}));
    Number M{std::numeric_limits<std::int64_t>::max()};
    CHECK((M == Number{9'223'372'036'854'776, 3}));
    caught = false;
    try
    {
        Number q{99'999'999'999'999'999, 32767};
    }
    catch (std::overflow_error const&)
    {
        caught = true;
    }
    CHECK(caught);
}

TEST_CASE("Number - test_add")
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
        CHECK(x + y == z);
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
    CHECK(caught);
}

TEST_CASE("Number - test_sub")
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
        CHECK(x - y == z);
}

TEST_CASE("Number - test_mul")
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
            CHECK(x * y == z);
    }
    Number::setround(Number::towards_zero);
    {
        Case c[]{
            {Number{7}, Number{8}, Number{56}},
            {Number{1414213562373095, -15},
             Number{1414213562373095, -15},
             Number{1999999999999999, -15}},
            {Number{-1414213562373095, -15},
             Number{1414213562373095, -15},
             Number{-1999999999999999, -15}},
            {Number{-1414213562373095, -15},
             Number{-1414213562373095, -15},
             Number{1999999999999999, -15}},
            {Number{3214285714285706, -15},
             Number{3111111111111119, -15},
             Number{9999999999999999, -15}},
            {Number{1000000000000000, -32768},
             Number{1000000000000000, -32768},
             Number{0}}};
        for (auto const& [x, y, z] : c)
            CHECK(x * y == z);
    }
    Number::setround(Number::downward);
    {
        Case c[]{
            {Number{7}, Number{8}, Number{56}},
            {Number{1414213562373095, -15},
             Number{1414213562373095, -15},
             Number{1999999999999999, -15}},
            {Number{-1414213562373095, -15},
             Number{1414213562373095, -15},
             Number{-2000000000000000, -15}},
            {Number{-1414213562373095, -15},
             Number{-1414213562373095, -15},
             Number{1999999999999999, -15}},
            {Number{3214285714285706, -15},
             Number{3111111111111119, -15},
             Number{9999999999999999, -15}},
            {Number{1000000000000000, -32768},
             Number{1000000000000000, -32768},
             Number{0}}};
        for (auto const& [x, y, z] : c)
            CHECK(x * y == z);
    }
    Number::setround(Number::upward);
    {
        Case c[]{
            {Number{7}, Number{8}, Number{56}},
            {Number{1414213562373095, -15},
             Number{1414213562373095, -15},
             Number{2000000000000000, -15}},
            {Number{-1414213562373095, -15},
             Number{1414213562373095, -15},
             Number{-1999999999999999, -15}},
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
            CHECK(x * y == z);
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
    CHECK(caught);
}

TEST_CASE("Number - test_div")
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
            CHECK(x / y == z);
    }
    Number::setround(Number::towards_zero);
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
            {Number{2}, Number{3}, Number{6'666'666'666'666'666, -16}},
            {Number{-2}, Number{3}, Number{-6'666'666'666'666'666, -16}}};
        for (auto const& [x, y, z] : c)
            CHECK(x / y == z);
    }
    Number::setround(Number::downward);
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
            {Number{2}, Number{3}, Number{6'666'666'666'666'666, -16}},
            {Number{-2}, Number{3}, Number{-6'666'666'666'666'667, -16}}};
        for (auto const& [x, y, z] : c)
            CHECK(x / y == z);
    }
    Number::setround(Number::upward);
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
            {Number{-2}, Number{3}, Number{-6'666'666'666'666'666, -16}}};
        for (auto const& [x, y, z] : c)
            CHECK(x / y == z);
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
    CHECK(caught);
}

TEST_CASE("Number - test_root")
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
        CHECK((root(x, y) == z));
    bool caught = false;
    try
    {
        (void)root(Number{-2}, 0);
    }
    catch (std::overflow_error const&)
    {
        caught = true;
    }
    CHECK(caught);
    caught = false;
    try
    {
        (void)root(Number{-2}, 4);
    }
    catch (std::overflow_error const&)
    {
        caught = true;
    }
    CHECK(caught);
}

TEST_CASE("Number - test_power1")
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
        CHECK((power(x, y) == z));
}

TEST_CASE("Number - test_power2")
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
        CHECK((power(x, n, d) == z));
    bool caught = false;
    try
    {
        (void)power(Number{7}, 0, 0);
    }
    catch (std::overflow_error const&)
    {
        caught = true;
    }
    CHECK(caught);
    caught = false;
    try
    {
        (void)power(Number{7}, 1, 0);
    }
    catch (std::overflow_error const&)
    {
        caught = true;
    }
    CHECK(caught);
    caught = false;
    try
    {
        (void)power(Number{-1, -1}, 3, 2);
    }
    catch (std::overflow_error const&)
    {
        caught = true;
    }
    CHECK(caught);
}

TEST_CASE("Number - testConversions")
{
    IOUAmount x{5, 6};
    Number y = x;
    CHECK((y == Number{5, 6}));
    IOUAmount z{y};
    CHECK(x == z);
    XRPAmount xrp{500};
    STAmount st = xrp;
    Number n = st;
    CHECK(XRPAmount{n} == xrp);
    IOUAmount x0{0, 0};
    Number y0 = x0;
    CHECK((y0 == Number{0}));
    IOUAmount z0{y0};
    CHECK(x0 == z0);
    XRPAmount xrp0{0};
    Number n0 = xrp0;
    CHECK(n0 == Number{0});
    XRPAmount xrp1{n0};
    CHECK(xrp1 == xrp0);
}

TEST_CASE("Number - test_to_integer")
{
    using Case = std::tuple<Number, std::int64_t>;
    saveNumberRoundMode save{Number::setround(Number::to_nearest)};
    {
        Case c[]{
            {Number{0}, 0},
            {Number{1}, 1},
            {Number{2}, 2},
            {Number{3}, 3},
            {Number{-1}, -1},
            {Number{-2}, -2},
            {Number{-3}, -3},
            {Number{10}, 10},
            {Number{99}, 99},
            {Number{1155}, 1155},
            {Number{9'999'999'999'999'999, 0}, 9'999'999'999'999'999},
            {Number{9'999'999'999'999'999, 1}, 99'999'999'999'999'990},
            {Number{9'999'999'999'999'999, 2}, 999'999'999'999'999'900},
            {Number{-9'999'999'999'999'999, 2}, -999'999'999'999'999'900},
            {Number{15, -1}, 2},
            {Number{14, -1}, 1},
            {Number{16, -1}, 2},
            {Number{25, -1}, 2},
            {Number{6, -1}, 1},
            {Number{5, -1}, 0},
            {Number{4, -1}, 0},
            {Number{-15, -1}, -2},
            {Number{-14, -1}, -1},
            {Number{-16, -1}, -2},
            {Number{-25, -1}, -2},
            {Number{-6, -1}, -1},
            {Number{-5, -1}, 0},
            {Number{-4, -1}, 0}};
        for (auto const& [x, y] : c)
        {
            auto j = static_cast<std::int64_t>(x);
            CHECK(j == y);
        }
    }
    auto prev_mode = Number::setround(Number::towards_zero);
    CHECK(prev_mode == Number::to_nearest);
    {
        Case c[]{
            {Number{0}, 0},
            {Number{1}, 1},
            {Number{2}, 2},
            {Number{3}, 3},
            {Number{-1}, -1},
            {Number{-2}, -2},
            {Number{-3}, -3},
            {Number{10}, 10},
            {Number{99}, 99},
            {Number{1155}, 1155},
            {Number{9'999'999'999'999'999, 0}, 9'999'999'999'999'999},
            {Number{9'999'999'999'999'999, 1}, 99'999'999'999'999'990},
            {Number{9'999'999'999'999'999, 2}, 999'999'999'999'999'900},
            {Number{-9'999'999'999'999'999, 2}, -999'999'999'999'999'900},
            {Number{15, -1}, 1},
            {Number{14, -1}, 1},
            {Number{16, -1}, 1},
            {Number{25, -1}, 2},
            {Number{6, -1}, 0},
            {Number{5, -1}, 0},
            {Number{4, -1}, 0},
            {Number{-15, -1}, -1},
            {Number{-14, -1}, -1},
            {Number{-16, -1}, -1},
            {Number{-25, -1}, -2},
            {Number{-6, -1}, 0},
            {Number{-5, -1}, 0},
            {Number{-4, -1}, 0}};
        for (auto const& [x, y] : c)
        {
            auto j = static_cast<std::int64_t>(x);
            CHECK(j == y);
        }
    }
    prev_mode = Number::setround(Number::downward);
    CHECK(prev_mode == Number::towards_zero);
    {
        Case c[]{
            {Number{0}, 0},
            {Number{1}, 1},
            {Number{2}, 2},
            {Number{3}, 3},
            {Number{-1}, -1},
            {Number{-2}, -2},
            {Number{-3}, -3},
            {Number{10}, 10},
            {Number{99}, 99},
            {Number{1155}, 1155},
            {Number{9'999'999'999'999'999, 0}, 9'999'999'999'999'999},
            {Number{9'999'999'999'999'999, 1}, 99'999'999'999'999'990},
            {Number{9'999'999'999'999'999, 2}, 999'999'999'999'999'900},
            {Number{-9'999'999'999'999'999, 2}, -999'999'999'999'999'900},
            {Number{15, -1}, 1},
            {Number{14, -1}, 1},
            {Number{16, -1}, 1},
            {Number{25, -1}, 2},
            {Number{6, -1}, 0},
            {Number{5, -1}, 0},
            {Number{4, -1}, 0},
            {Number{-15, -1}, -2},
            {Number{-14, -1}, -2},
            {Number{-16, -1}, -2},
            {Number{-25, -1}, -3},
            {Number{-6, -1}, -1},
            {Number{-5, -1}, -1},
            {Number{-4, -1}, -1}};
        for (auto const& [x, y] : c)
        {
            auto j = static_cast<std::int64_t>(x);
            CHECK(j == y);
        }
    }
    prev_mode = Number::setround(Number::upward);
    CHECK(prev_mode == Number::downward);
    {
        Case c[]{
            {Number{0}, 0},
            {Number{1}, 1},
            {Number{2}, 2},
            {Number{3}, 3},
            {Number{-1}, -1},
            {Number{-2}, -2},
            {Number{-3}, -3},
            {Number{10}, 10},
            {Number{99}, 99},
            {Number{1155}, 1155},
            {Number{9'999'999'999'999'999, 0}, 9'999'999'999'999'999},
            {Number{9'999'999'999'999'999, 1}, 99'999'999'999'999'990},
            {Number{9'999'999'999'999'999, 2}, 999'999'999'999'999'900},
            {Number{-9'999'999'999'999'999, 2}, -999'999'999'999'999'900},
            {Number{15, -1}, 2},
            {Number{14, -1}, 2},
            {Number{16, -1}, 2},
            {Number{25, -1}, 3},
            {Number{6, -1}, 1},
            {Number{5, -1}, 1},
            {Number{4, -1}, 1},
            {Number{-15, -1}, -1},
            {Number{-14, -1}, -1},
            {Number{-16, -1}, -1},
            {Number{-25, -1}, -2},
            {Number{-6, -1}, 0},
            {Number{-5, -1}, 0},
            {Number{-4, -1}, 0}};
        for (auto const& [x, y] : c)
        {
            auto j = static_cast<std::int64_t>(x);
            CHECK(j == y);
        }
    }
    bool caught = false;
    try
    {
        (void)static_cast<std::int64_t>(Number{9223372036854776, 3});
    }
    catch (std::overflow_error const&)
    {
        caught = true;
    }
    CHECK(caught);
}

TEST_CASE("Number - test_squelch")
{
    Number limit{1, -6};
    CHECK((squelch(Number{2, -6}, limit) == Number{2, -6}));
    CHECK((squelch(Number{1, -6}, limit) == Number{1, -6}));
    CHECK((squelch(Number{9, -7}, limit) == Number{0}));
    CHECK((squelch(Number{-2, -6}, limit) == Number{-2, -6}));
    CHECK((squelch(Number{-1, -6}, limit) == Number{-1, -6}));
    CHECK((squelch(Number{-9, -7}, limit) == Number{0}));
}

TEST_CASE("Number - testToString")
{
    CHECK(to_string(Number(-2, 0)) == "-2");
    CHECK(to_string(Number(0, 0)) == "0");
    CHECK(to_string(Number(2, 0)) == "2");
    CHECK(to_string(Number(25, -3)) == "0.025");
    CHECK(to_string(Number(-25, -3)) == "-0.025");
    CHECK(to_string(Number(25, 1)) == "250");
    CHECK(to_string(Number(-25, 1)) == "-250");
    CHECK(to_string(Number(2, 20)) == "2000000000000000e5");
    CHECK(to_string(Number(-2, -20)) == "-2000000000000000e-35");
}

TEST_CASE("Number - test_relationals")
{
    CHECK(!(Number{100} < Number{10}));
    CHECK(Number{100} > Number{10});
    CHECK(Number{100} >= Number{10});
    CHECK(!(Number{100} <= Number{10}));
}

TEST_CASE("Number - test_stream")
{
    Number x{100};
    std::ostringstream os;
    os << x;
    CHECK(os.str() == to_string(x));
}

TEST_CASE("Number - test_inc_dec")
{
    Number x{100};
    Number y = +x;
    CHECK(x == y);
    CHECK(x++ == y);
    CHECK(x == Number{101});
    CHECK(x-- == Number{101});
    CHECK(x == y);
}

TEST_CASE("Number - test_toSTAmount")
{
    NumberSO stNumberSO{true};
    Issue const issue;
    Number const n{7'518'783'80596, -5};
    saveNumberRoundMode const save{Number::setround(Number::to_nearest)};
    auto res2 = STAmount{issue, n.mantissa(), n.exponent()};
    CHECK(res2 == STAmount{7518784});

    Number::setround(Number::towards_zero);
    res2 = STAmount{issue, n.mantissa(), n.exponent()};
    CHECK(res2 == STAmount{7518783});

    Number::setround(Number::downward);
    res2 = STAmount{issue, n.mantissa(), n.exponent()};
    CHECK(res2 == STAmount{7518783});

    Number::setround(Number::upward);
    res2 = STAmount{issue, n.mantissa(), n.exponent()};
    CHECK(res2 == STAmount{7518784});
}

TEST_CASE("Number - test_truncate")
{
    CHECK(Number(25, +1).truncate() == Number(250, 0));
    CHECK(Number(25, 0).truncate() == Number(25, 0));
    CHECK(Number(25, -1).truncate() == Number(2, 0));
    CHECK(Number(25, -2).truncate() == Number(0, 0));
    CHECK(Number(99, -2).truncate() == Number(0, 0));

    CHECK(Number(-25, +1).truncate() == Number(-250, 0));
    CHECK(Number(-25, 0).truncate() == Number(-25, 0));
    CHECK(Number(-25, -1).truncate() == Number(-2, 0));
    CHECK(Number(-25, -2).truncate() == Number(0, 0));
    CHECK(Number(-99, -2).truncate() == Number(0, 0));

    CHECK(Number(0, 0).truncate() == Number(0, 0));
    CHECK(Number(0, 30000).truncate() == Number(0, 0));
    CHECK(Number(0, -30000).truncate() == Number(0, 0));
    CHECK(Number(100, -30000).truncate() == Number(0, 0));
    CHECK(Number(100, -30000).truncate() == Number(0, 0));
    CHECK(Number(-100, -30000).truncate() == Number(0, 0));
    CHECK(Number(-100, -30000).truncate() == Number(0, 0));
}

TEST_CASE("Number - Rounding")
{
    // Test that rounding works as expected.

    using NumberRoundings = std::map<Number::rounding_mode, std::int64_t>;

    std::map<Number, NumberRoundings> const expected{
        // Positive numbers
        {Number{13, -1},
         {{Number::to_nearest, 1},
          {Number::towards_zero, 1},
          {Number::downward, 1},
          {Number::upward, 2}}},
        {Number{23, -1},
         {{Number::to_nearest, 2},
          {Number::towards_zero, 2},
          {Number::downward, 2},
          {Number::upward, 3}}},
        {Number{15, -1},
         {{Number::to_nearest, 2},
          {Number::towards_zero, 1},
          {Number::downward, 1},
          {Number::upward, 2}}},
        {Number{25, -1},
         {{Number::to_nearest, 2},
          {Number::towards_zero, 2},
          {Number::downward, 2},
          {Number::upward, 3}}},
        {Number{152, -2},
         {{Number::to_nearest, 2},
          {Number::towards_zero, 1},
          {Number::downward, 1},
          {Number::upward, 2}}},
        {Number{252, -2},
         {{Number::to_nearest, 3},
          {Number::towards_zero, 2},
          {Number::downward, 2},
          {Number::upward, 3}}},
        {Number{17, -1},
         {{Number::to_nearest, 2},
          {Number::towards_zero, 1},
          {Number::downward, 1},
          {Number::upward, 2}}},
        {Number{27, -1},
         {{Number::to_nearest, 3},
          {Number::towards_zero, 2},
          {Number::downward, 2},
          {Number::upward, 3}}},

        // Negative numbers
        {Number{-13, -1},
         {{Number::to_nearest, -1},
          {Number::towards_zero, -1},
          {Number::downward, -2},
          {Number::upward, -1}}},
        {Number{-23, -1},
         {{Number::to_nearest, -2},
          {Number::towards_zero, -2},
          {Number::downward, -3},
          {Number::upward, -2}}},
        {Number{-15, -1},
         {{Number::to_nearest, -2},
          {Number::towards_zero, -1},
          {Number::downward, -2},
          {Number::upward, -1}}},
        {Number{-25, -1},
         {{Number::to_nearest, -2},
          {Number::towards_zero, -2},
          {Number::downward, -3},
          {Number::upward, -2}}},
        {Number{-152, -2},
         {{Number::to_nearest, -2},
          {Number::towards_zero, -1},
          {Number::downward, -2},
          {Number::upward, -1}}},
        {Number{-252, -2},
         {{Number::to_nearest, -3},
          {Number::towards_zero, -2},
          {Number::downward, -3},
          {Number::upward, -2}}},
        {Number{-17, -1},
         {{Number::to_nearest, -2},
          {Number::towards_zero, -1},
          {Number::downward, -2},
          {Number::upward, -1}}},
        {Number{-27, -1},
         {{Number::to_nearest, -3},
          {Number::towards_zero, -2},
          {Number::downward, -3},
          {Number::upward, -2}}},
    };

    for (auto const& [num, roundings] : expected)
    {
        for (auto const& [mode, val] : roundings)
        {
            NumberRoundModeGuard g{mode};
            auto const res = static_cast<std::int64_t>(num);
            auto const message = to_string(num) + " with mode " + std::to_string(mode) +
                    " expected " + std::to_string(val) + " got " +
                    std::to_string(res);
            CHECK_MESSAGE(res == val, message);
        }
    }
}

TEST_SUITE_END();

}  // namespace ripple

#include <xrpl/protocol/Quality.h>

#include <doctest/doctest.h>

#include <type_traits>

using namespace xrpl;

TEST_SUITE_BEGIN("Quality");

namespace {

// Create a raw, non-integral amount from mantissa and exponent
STAmount
raw(std::uint64_t mantissa, int exponent)
{
    return STAmount(Issue{Currency(3), AccountID(3)}, mantissa, exponent);
}

template <class Integer>
STAmount
amount(Integer integer, std::enable_if_t<std::is_signed<Integer>::value>* = 0)
{
    static_assert(std::is_integral<Integer>::value, "");
    return STAmount(integer, false);
}

template <class Integer>
STAmount
amount(Integer integer, std::enable_if_t<!std::is_signed<Integer>::value>* = 0)
{
    static_assert(std::is_integral<Integer>::value, "");
    if (integer < 0)
        return STAmount(-integer, true);
    return STAmount(integer, false);
}

template <class In, class Out>
Amounts
amounts(In in, Out out)
{
    return Amounts(amount(in), amount(out));
}

template <class In1, class Out1, class Int, class In2, class Out2>
void
ceil_in(
    Quality const& q,
    In1 in,
    Out1 out,
    Int limit,
    In2 in_expected,
    Out2 out_expected)
{
    auto expect_result(amounts(in_expected, out_expected));
    auto actual_result(q.ceil_in(amounts(in, out), amount(limit)));

    CHECK(actual_result == expect_result);
}

template <class In1, class Out1, class Int, class In2, class Out2>
void
ceil_out(
    Quality const& q,
    In1 in,
    Out1 out,
    Int limit,
    In2 in_expected,
    Out2 out_expected)
{
    auto const expect_result(amounts(in_expected, out_expected));
    auto const actual_result(q.ceil_out(amounts(in, out), amount(limit)));

    CHECK(actual_result == expect_result);
}

}  // namespace

TEST_CASE("comparisons")
{
    STAmount const amount1(noIssue(), 231);
    STAmount const amount2(noIssue(), 462);
    STAmount const amount3(noIssue(), 924);

    Quality const q11(Amounts(amount1, amount1));
    Quality const q12(Amounts(amount1, amount2));
    Quality const q13(Amounts(amount1, amount3));
    Quality const q21(Amounts(amount2, amount1));
    Quality const q31(Amounts(amount3, amount1));

    CHECK(q11 == q11);
    CHECK(q11 < q12);
    CHECK(q12 < q13);
    CHECK(q31 < q21);
    CHECK(q21 < q11);
    CHECK(q11 >= q11);
    CHECK(q12 >= q11);
    CHECK(q13 >= q12);
    CHECK(q21 >= q31);
    CHECK(q11 >= q21);
    CHECK(q12 > q11);
    CHECK(q13 > q12);
    CHECK(q21 > q31);
    CHECK(q11 > q21);
    CHECK(q11 <= q11);
    CHECK(q11 <= q12);
    CHECK(q12 <= q13);
    CHECK(q31 <= q21);
    CHECK(q21 <= q11);
    CHECK(q31 != q21);
}

TEST_CASE("composition")
{
    STAmount const amount1(noIssue(), 231);
    STAmount const amount2(noIssue(), 462);
    STAmount const amount3(noIssue(), 924);

    Quality const q11(Amounts(amount1, amount1));
    Quality const q12(Amounts(amount1, amount2));
    Quality const q13(Amounts(amount1, amount3));
    Quality const q21(Amounts(amount2, amount1));
    Quality const q31(Amounts(amount3, amount1));

    CHECK(composed_quality(q12, q21) == q11);

    Quality const q13_31(composed_quality(q13, q31));
    Quality const q31_13(composed_quality(q31, q13));

    CHECK(q13_31 == q31_13);
    CHECK(q13_31 == q11);
}

TEST_CASE("operations")
{
    Quality const q11(
        Amounts(STAmount(noIssue(), 731), STAmount(noIssue(), 731)));

    Quality qa(q11);
    Quality qb(q11);

    CHECK(qa == qb);
    CHECK(++qa != q11);
    CHECK(qa != qb);
    CHECK(--qb != q11);
    CHECK(qa != qb);
    CHECK(qb < qa);
    CHECK(qb++ < qa);
    CHECK(qb++ < qa);
    CHECK(qb++ == qa);
    CHECK(qa < qb);
}

TEST_CASE("ceil_in")
{
    SUBCASE("1 in, 1 out")
    {
        Quality q(Amounts(amount(1), amount(1)));

        ceil_in(q, 1, 1, 1, 1, 1);    // 1 in, 1 out, limit 1 -> 1 in, 1 out
        ceil_in(q, 10, 10, 5, 5, 5);  // 10 in, 10 out, limit 5 -> 5 in, 5 out
        ceil_in(q, 5, 5, 10, 5, 5);   // 5 in, 5 out, limit 10 -> 5 in, 5 out
    }

    SUBCASE("1 in, 2 out")
    {
        Quality q(Amounts(amount(1), amount(2)));

        ceil_in(
            q, 40, 80, 40, 40, 80);  // 40 in, 80 out, limit 40 -> 40 in, 80 out
        ceil_in(
            q, 40, 80, 20, 20, 40);  // 40 in, 80 out, limit 20 -> 20 in, 40 out
        ceil_in(
            q, 40, 80, 60, 40, 80);  // 40 in, 80 out, limit 60 -> 40 in, 80 out
    }

    SUBCASE("2 in, 1 out")
    {
        Quality q(Amounts(amount(2), amount(1)));

        ceil_in(
            q, 40, 20, 20, 20, 10);  // 40 in, 20 out, limit 20 -> 20 in, 10 out
        ceil_in(
            q, 40, 20, 40, 40, 20);  // 40 in, 20 out, limit 40 -> 40 in, 20 out
        ceil_in(
            q, 40, 20, 50, 40, 20);  // 40 in, 20 out, limit 50 -> 40 in, 20 out
    }
}

TEST_CASE("ceil_out")
{
    SUBCASE("1 in, 1 out")
    {
        Quality q(Amounts(amount(1), amount(1)));

        ceil_out(q, 1, 1, 1, 1, 1);    // 1 in, 1 out, limit 1 -> 1 in, 1 out
        ceil_out(q, 10, 10, 5, 5, 5);  // 10 in, 10 out, limit 5 -> 5 in, 5 out
        ceil_out(
            q, 10, 10, 20, 10, 10);  // 10 in, 10 out, limit 20 -> 10 in, 10 out
    }

    SUBCASE("1 in, 2 out")
    {
        Quality q(Amounts(amount(1), amount(2)));

        ceil_out(
            q, 40, 80, 40, 20, 40);  // 40 in, 80 out, limit 40 -> 20 in, 40 out
        ceil_out(
            q, 40, 80, 80, 40, 80);  // 40 in, 80 out, limit 80 -> 40 in, 80 out
        ceil_out(
            q,
            40,
            80,
            100,
            40,
            80);  // 40 in, 80 out, limit 100 -> 40 in, 80 out
    }

    SUBCASE("2 in, 1 out")
    {
        Quality q(Amounts(amount(2), amount(1)));

        ceil_out(
            q, 40, 20, 20, 40, 20);  // 40 in, 20 out, limit 20 -> 40 in, 20 out
        ceil_out(
            q, 40, 20, 40, 40, 20);  // 40 in, 20 out, limit 40 -> 40 in, 20 out
        ceil_out(
            q, 40, 20, 10, 20, 10);  // 40 in, 20 out, limit 10 -> 20 in, 10 out
    }
}

TEST_CASE("raw")
{
    Quality q(0x5d048191fb9130daull);  // 126836389.7680090
    Amounts const value(
        amount(349469768),                             // 349.469768 XRP
        raw(2755280000000000ull, -15));                // 2.75528
    STAmount const limit(raw(4131113916555555, -16));  // .4131113916555555
    Amounts const result(q.ceil_out(value, limit));
    CHECK(result.in != beast::zero);
}

TEST_CASE("round")
{
    Quality q(0x59148191fb913522ull);  // 57719.63525051682
    CHECK(q.round(3).rate().getText() == "57800");
    CHECK(q.round(4).rate().getText() == "57720");
    CHECK(q.round(5).rate().getText() == "57720");
    CHECK(q.round(6).rate().getText() == "57719.7");
    CHECK(q.round(7).rate().getText() == "57719.64");
    CHECK(q.round(8).rate().getText() == "57719.636");
    CHECK(q.round(9).rate().getText() == "57719.6353");
    CHECK(q.round(10).rate().getText() == "57719.63526");
    CHECK(q.round(11).rate().getText() == "57719.635251");
    CHECK(q.round(12).rate().getText() == "57719.6352506");
    CHECK(q.round(13).rate().getText() == "57719.63525052");
    CHECK(q.round(14).rate().getText() == "57719.635250517");
    CHECK(q.round(15).rate().getText() == "57719.6352505169");
    CHECK(q.round(16).rate().getText() == "57719.63525051682");
}

TEST_SUITE_END();

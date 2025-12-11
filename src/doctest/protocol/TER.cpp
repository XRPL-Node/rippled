#include <xrpl/protocol/TER.h>

#include <doctest/doctest.h>

#include <string>
#include <tuple>
#include <type_traits>

using namespace xrpl;

TEST_SUITE_BEGIN("TER");

TEST_CASE("transResultInfo")
{
    for (auto i = -400; i < 400; ++i)
    {
        TER t = TER::fromInt(i);
        auto inRange = isTelLocal(t) || isTemMalformed(t) || isTefFailure(t) ||
            isTerRetry(t) || isTesSuccess(t) || isTecClaim(t);

        std::string token, text;
        auto good = transResultInfo(t, token, text);
        CHECK((inRange || !good));
        CHECK(transToken(t) == (good ? token : "-"));
        CHECK(transHuman(t) == (good ? text : "-"));

        auto code = transCode(token);
        CHECK(good == !!code);
        CHECK((!code || *code == t));
    }
}

TEST_CASE("conversion")
{
    // Lambda that verifies assignability and convertibility.
    auto isConvertable = [](auto from, auto to) {
        using From_t = std::decay_t<decltype(from)>;
        using To_t = std::decay_t<decltype(to)>;
        static_assert(std::is_convertible<From_t, To_t>::value, "Convert err");
        static_assert(
            std::is_constructible<To_t, From_t>::value, "Construct err");
        static_assert(
            std::is_assignable<To_t&, From_t const&>::value, "Assign err");
    };

    // Verify the right types convert to NotTEC.
    NotTEC const notTec;
    isConvertable(telLOCAL_ERROR, notTec);
    isConvertable(temMALFORMED, notTec);
    isConvertable(tefFAILURE, notTec);
    isConvertable(terRETRY, notTec);
    isConvertable(tesSUCCESS, notTec);
    isConvertable(notTec, notTec);

    // Lambda that verifies types and not assignable or convertible.
    auto notConvertible = [](auto from, auto to) {
        using To_t = std::decay_t<decltype(to)>;
        using From_t = std::decay_t<decltype(from)>;
        static_assert(!std::is_convertible<From_t, To_t>::value, "Convert err");
        static_assert(
            !std::is_constructible<To_t, From_t>::value, "Construct err");
        static_assert(
            !std::is_assignable<To_t&, From_t const&>::value, "Assign err");
    };

    // Verify types that shouldn't convert to NotTEC.
    TER const ter;
    notConvertible(tecCLAIM, notTec);
    notConvertible(ter, notTec);
    notConvertible(4, notTec);

    // Verify the right types convert to TER.
    isConvertable(telLOCAL_ERROR, ter);
    isConvertable(temMALFORMED, ter);
    isConvertable(tefFAILURE, ter);
    isConvertable(terRETRY, ter);
    isConvertable(tesSUCCESS, ter);
    isConvertable(tecCLAIM, ter);
    isConvertable(notTec, ter);
    isConvertable(ter, ter);

    // Verify that you can't convert from int to ter.
    notConvertible(4, ter);
}

TEST_CASE("comparison")
{
    // Test comparison operators on TER types
    auto checkComparable = [](auto lhs, auto rhs) {
        CHECK((lhs == rhs) == (TERtoInt(lhs) == TERtoInt(rhs)));
        CHECK((lhs != rhs) == (TERtoInt(lhs) != TERtoInt(rhs)));
        CHECK((lhs < rhs) == (TERtoInt(lhs) < TERtoInt(rhs)));
        CHECK((lhs <= rhs) == (TERtoInt(lhs) <= TERtoInt(rhs)));
        CHECK((lhs > rhs) == (TERtoInt(lhs) > TERtoInt(rhs)));
        CHECK((lhs >= rhs) == (TERtoInt(lhs) >= TERtoInt(rhs)));
    };

    // Test various TER type comparisons
    checkComparable(telLOCAL_ERROR, temMALFORMED);
    checkComparable(tefFAILURE, terRETRY);
    checkComparable(tesSUCCESS, tecCLAIM);
    checkComparable(NotTEC{telLOCAL_ERROR}, TER{tecCLAIM});
    checkComparable(tesSUCCESS, tesSUCCESS);
}

TEST_SUITE_END();

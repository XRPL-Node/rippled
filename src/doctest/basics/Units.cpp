#include <xrpl/protocol/SystemParameters.h>
#include <xrpl/protocol/Units.h>

#include <doctest/doctest.h>

using namespace xrpl;

TEST_SUITE_BEGIN("Units");

TEST_CASE("Initial XRP")
{
    CHECK(INITIAL_XRP.drops() == 100'000'000'000'000'000);
    CHECK(INITIAL_XRP == XRPAmount{100'000'000'000'000'000});
}

TEST_CASE("Types")
{
    using FeeLevel32 = FeeLevel<std::uint32_t>;

    SUBCASE("XRPAmount with uint32 FeeLevel")
    {
        XRPAmount x{100};
        CHECK(x.drops() == 100);
        CHECK((std::is_same_v<decltype(x)::unit_type, unit::dropTag>));
        auto y = 4u * x;
        CHECK(y.value() == 400);
        CHECK((std::is_same_v<decltype(y)::unit_type, unit::dropTag>));

        auto z = 4 * y;
        CHECK(z.value() == 1600);
        CHECK((std::is_same_v<decltype(z)::unit_type, unit::dropTag>));

        FeeLevel32 f{10};
        FeeLevel32 baseFee{100};

        auto drops = mulDiv(baseFee, x, f);

        CHECK(drops);
        CHECK(drops.value() == 1000);
        CHECK((std::is_same_v<
               std::remove_reference_t<decltype(*drops)>::unit_type,
               unit::dropTag>));

        CHECK((std::is_same_v<
               std::remove_reference_t<decltype(*drops)>,
               XRPAmount>));
    }

    SUBCASE("XRPAmount with uint64 FeeLevel")
    {
        XRPAmount x{100};
        CHECK(x.value() == 100);
        CHECK((std::is_same_v<decltype(x)::unit_type, unit::dropTag>));
        auto y = 4u * x;
        CHECK(y.value() == 400);
        CHECK((std::is_same_v<decltype(y)::unit_type, unit::dropTag>));

        FeeLevel64 f{10};
        FeeLevel64 baseFee{100};

        auto drops = mulDiv(baseFee, x, f);

        CHECK(drops);
        CHECK(drops.value() == 1000);
        CHECK((std::is_same_v<
               std::remove_reference_t<decltype(*drops)>::unit_type,
               unit::dropTag>));
        CHECK((std::is_same_v<
               std::remove_reference_t<decltype(*drops)>,
               XRPAmount>));
    }

    SUBCASE("FeeLevel64 operations")
    {
        FeeLevel64 x{1024};
        CHECK(x.value() == 1024);
        CHECK((std::is_same_v<decltype(x)::unit_type, unit::feelevelTag>));
        std::uint64_t m = 4;
        auto y = m * x;
        CHECK(y.value() == 4096);
        CHECK((std::is_same_v<decltype(y)::unit_type, unit::feelevelTag>));

        XRPAmount basefee{10};
        FeeLevel64 referencefee{256};

        auto drops = mulDiv(x, basefee, referencefee);

        CHECK(drops);
        CHECK(drops.value() == 40);
        CHECK((std::is_same_v<
               std::remove_reference_t<decltype(*drops)>::unit_type,
               unit::dropTag>));
        CHECK((std::is_same_v<
               std::remove_reference_t<decltype(*drops)>,
               XRPAmount>));
    }
}

TEST_CASE("Json")
{
    using FeeLevel32 = FeeLevel<std::uint32_t>;

    SUBCASE("FeeLevel32 max")
    {
        FeeLevel32 x{std::numeric_limits<std::uint32_t>::max()};
        auto y = x.jsonClipped();
        CHECK(y.type() == Json::uintValue);
        CHECK(y == Json::Value{x.fee()});
    }

    SUBCASE("FeeLevel32 min")
    {
        FeeLevel32 x{std::numeric_limits<std::uint32_t>::min()};
        auto y = x.jsonClipped();
        CHECK(y.type() == Json::uintValue);
        CHECK(y == Json::Value{x.fee()});
    }

    SUBCASE("FeeLevel64 max")
    {
        FeeLevel64 x{std::numeric_limits<std::uint64_t>::max()};
        auto y = x.jsonClipped();
        CHECK(y.type() == Json::uintValue);
        CHECK(y == Json::Value{std::numeric_limits<std::uint32_t>::max()});
    }

    SUBCASE("FeeLevel64 min")
    {
        FeeLevel64 x{std::numeric_limits<std::uint64_t>::min()};
        auto y = x.jsonClipped();
        CHECK(y.type() == Json::uintValue);
        CHECK(y == Json::Value{0});
    }

    SUBCASE("FeeLevelDouble max")
    {
        FeeLevelDouble x{std::numeric_limits<double>::max()};
        auto y = x.jsonClipped();
        CHECK(y.type() == Json::realValue);
        CHECK(y == Json::Value{std::numeric_limits<double>::max()});
    }

    SUBCASE("FeeLevelDouble min")
    {
        FeeLevelDouble x{std::numeric_limits<double>::min()};
        auto y = x.jsonClipped();
        CHECK(y.type() == Json::realValue);
        CHECK(y == Json::Value{std::numeric_limits<double>::min()});
    }

    SUBCASE("XRPAmount max")
    {
        XRPAmount x{std::numeric_limits<std::int64_t>::max()};
        auto y = x.jsonClipped();
        CHECK(y.type() == Json::intValue);
        CHECK(y == Json::Value{std::numeric_limits<std::int32_t>::max()});
    }

    SUBCASE("XRPAmount min")
    {
        XRPAmount x{std::numeric_limits<std::int64_t>::min()};
        auto y = x.jsonClipped();
        CHECK(y.type() == Json::intValue);
        CHECK(y == Json::Value{std::numeric_limits<std::int32_t>::min()});
    }
}

TEST_CASE("Functions")
{
    using FeeLevel32 = FeeLevel<std::uint32_t>;

    SUBCASE("FeeLevel64 functions")
    {
        auto make = [&](auto x) -> FeeLevel64 { return x; };
        auto explicitmake = [&](auto x) -> FeeLevel64 { return FeeLevel64{x}; };

        [[maybe_unused]] FeeLevel64 defaulted;
        FeeLevel64 test{0};
        CHECK(test.fee() == 0);

        test = explicitmake(beast::zero);
        CHECK(test.fee() == 0);

        test = beast::zero;
        CHECK(test.fee() == 0);

        test = explicitmake(100u);
        CHECK(test.fee() == 100);

        FeeLevel64 const targetSame{200u};
        FeeLevel32 const targetOther{300u};
        test = make(targetSame);
        CHECK(test.fee() == 200);
        CHECK(test == targetSame);
        CHECK(test < FeeLevel64{1000});
        CHECK(test > FeeLevel64{100});
        test = make(targetOther);
        CHECK(test.fee() == 300);
        CHECK(test == targetOther);

        test = std::uint64_t(200);
        CHECK(test.fee() == 200);
        test = std::uint32_t(300);
        CHECK(test.fee() == 300);

        test = targetSame;
        CHECK(test.fee() == 200);
        test = targetOther.fee();
        CHECK(test.fee() == 300);
        CHECK(test == targetOther);

        test = targetSame * 2;
        CHECK(test.fee() == 400);
        test = 3 * targetSame;
        CHECK(test.fee() == 600);
        test = targetSame / 10;
        CHECK(test.fee() == 20);

        test += targetSame;
        CHECK(test.fee() == 220);

        test -= targetSame;
        CHECK(test.fee() == 20);

        test++;
        CHECK(test.fee() == 21);
        ++test;
        CHECK(test.fee() == 22);
        test--;
        CHECK(test.fee() == 21);
        --test;
        CHECK(test.fee() == 20);

        test *= 5;
        CHECK(test.fee() == 100);
        test /= 2;
        CHECK(test.fee() == 50);
        test %= 13;
        CHECK(test.fee() == 11);

        CHECK(test);
        test = 0;
        CHECK(!test);
        CHECK(test.signum() == 0);
        test = targetSame;
        CHECK(test.signum() == 1);
        CHECK(to_string(test) == "200");
    }

    SUBCASE("FeeLevelDouble functions")
    {
        auto make = [&](auto x) -> FeeLevelDouble { return x; };
        auto explicitmake = [&](auto x) -> FeeLevelDouble {
            return FeeLevelDouble{x};
        };

        [[maybe_unused]] FeeLevelDouble defaulted;
        FeeLevelDouble test{0};
        CHECK(test.fee() == 0);

        test = explicitmake(beast::zero);
        CHECK(test.fee() == 0);

        test = beast::zero;
        CHECK(test.fee() == 0);

        test = explicitmake(100.0);
        CHECK(test.fee() == 100);

        FeeLevelDouble const targetSame{200.0};
        FeeLevel64 const targetOther{300};
        test = make(targetSame);
        CHECK(test.fee() == 200);
        CHECK(test == targetSame);
        CHECK(test < FeeLevelDouble{1000.0});
        CHECK(test > FeeLevelDouble{100.0});
        test = targetOther.fee();
        CHECK(test.fee() == 300);
        CHECK(test == targetOther);

        test = 200.0;
        CHECK(test.fee() == 200);
        test = std::uint64_t(300);
        CHECK(test.fee() == 300);

        test = targetSame;
        CHECK(test.fee() == 200);

        test = targetSame * 2;
        CHECK(test.fee() == 400);
        test = 3 * targetSame;
        CHECK(test.fee() == 600);
        test = targetSame / 10;
        CHECK(test.fee() == 20);

        test += targetSame;
        CHECK(test.fee() == 220);

        test -= targetSame;
        CHECK(test.fee() == 20);

        test++;
        CHECK(test.fee() == 21);
        ++test;
        CHECK(test.fee() == 22);
        test--;
        CHECK(test.fee() == 21);
        --test;
        CHECK(test.fee() == 20);

        test *= 5;
        CHECK(test.fee() == 100);
        test /= 2;
        CHECK(test.fee() == 50);

        // legal with signed
        test = -test;
        CHECK(test.fee() == -50);
        CHECK(test.signum() == -1);
        CHECK(to_string(test) == "-50.000000");

        CHECK(test);
        test = 0;
        CHECK(!test);
        CHECK(test.signum() == 0);
        test = targetSame;
        CHECK(test.signum() == 1);
        CHECK(to_string(test) == "200.000000");
    }
}

TEST_SUITE_END();

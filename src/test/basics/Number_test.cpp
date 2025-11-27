#include <xrpl/basics/Number.h>
#include <xrpl/beast/unit_test.h>
#include <xrpl/protocol/IOUAmount.h>
#include <xrpl/protocol/STAmount.h>
#include <xrpl/protocol/SystemParameters.h>

#include <sstream>
#include <tuple>

namespace ripple {

class Number_test : public beast::unit_test::suite
{
public:
    void
    testZero()
    {
        testcase << "zero " << to_string(Number::getMantissaScale());

        for (Number const& z : {Number{0, 0}, Number{0}})
        {
            BEAST_EXPECT(z.mantissa() == 0);
            BEAST_EXPECT(z.exponent() == Number{}.exponent());

            BEAST_EXPECT((z + z) == z);
            BEAST_EXPECT((z - z) == z);
            BEAST_EXPECT(z == -z);
        }
    }

    void
    test_limits()
    {
        auto const scale = Number::getMantissaScale();
        testcase << "test_limits " << to_string(scale);
        bool caught = false;
        auto const minMantissa = Number::minMantissa();
        try
        {
            Number x =
                Number{false, minMantissa * 10, 32768, Number::normalized{}};
        }
        catch (std::overflow_error const&)
        {
            caught = true;
        }
        BEAST_EXPECT(caught);

        auto test = [this](auto const& x, auto const& y) {
            auto const result = x == y;
            std::stringstream ss;
            ss << x << " == " << y << " -> " << (result ? "true" : "false");
            BEAST_EXPECTS(result, ss.str());
        };

        test(
            Number{false, minMantissa * 10, 32767, Number::normalized{}},
            Number{false, minMantissa, 32768, Number::normalized{}});
        test(
            Number{false, minMantissa, -32769, Number::normalized{}}, Number{});
        test(
            Number{false, minMantissa, 32000, Number::normalized{}} * 1'000 +
                Number{false, 1'500, 32000, Number::normalized{}},
            Number{false, minMantissa + 2, 32003, Number::normalized{}});
        // 9,223,372,036,854,775,808

        test(
            Number{std::numeric_limits<std::int64_t>::min()},
            Number{
                scale == MantissaRange::small
                    ? -9'223'372'036'854'776
                    : std::numeric_limits<std::int64_t>::min(),
                18 - Number::mantissaLog()});
        test(
            Number{std::numeric_limits<std::int64_t>::max()},
            Number{
                scale == MantissaRange::small
                    ? 9'223'372'036'854'776
                    : std::numeric_limits<std::int64_t>::max(),
                18 - Number::mantissaLog()});
        caught = false;
        try
        {
            Number q =
                Number{false, minMantissa, 32767, Number::normalized{}} * 100;
        }
        catch (std::overflow_error const&)
        {
            caught = true;
        }
        BEAST_EXPECT(caught);
    }

    void
    test_add()
    {
        auto const scale = Number::getMantissaScale();
        testcase << "test_add " << to_string(scale);

        using Case = std::tuple<Number, Number, Number>;
        auto const cSmall = std::to_array<Case>(
            {{Number{1'000'000'000'000'000, -15},
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
             {Number{5}, Number{}, Number{5}},
             {Number{5'555'555'555'555'555, -32768},
              Number{-5'555'555'555'555'554, -32768},
              Number{0}},
             {Number{-9'999'999'999'999'999, -31},
              Number{1'000'000'000'000'000, -15},
              Number{9'999'999'999'999'990, -16}}});
        auto const cLarge = std::to_array<Case>(
            // Note that items with extremely large mantissas need to be
            // calculated, because otherwise they overflow uint64. Items from C
            // with larger mantissa
            {{Number{1'000'000'000'000'000, -15},
              Number{6'555'555'555'555'555, -29},
              Number{1'000'000'000'000'065'556, -18}},
             {Number{-1'000'000'000'000'000, -15},
              Number{-6'555'555'555'555'555, -29},
              Number{-1'000'000'000'000'065'556, -18}},
             {Number{-1'000'000'000'000'000, -15},
              Number{6'555'555'555'555'555, -29},
              Number{
                  true,
                  9'999'999'999'999'344'444ULL,
                  -19,
                  Number::normalized{}}},
             {Number{-6'555'555'555'555'555, -29},
              Number{1'000'000'000'000'000, -15},
              Number{
                  false,
                  9'999'999'999'999'344'444ULL,
                  -19,
                  Number::normalized{}}},
             {Number{}, Number{5}, Number{5}},
             {Number{5}, Number{}, Number{5}},
             {Number{5'555'555'555'555'555'000, -32768},
              Number{-5'555'555'555'555'554'000, -32768},
              Number{0}},
             {Number{-9'999'999'999'999'999, -31},
              Number{1'000'000'000'000'000, -15},
              Number{9'999'999'999'999'990, -16}},
             // Items from cSmall expanded for the larger mantissa
             {Number{1'000'000'000'000'000'000, -18},
              Number{6'555'555'555'555'555'555, -35},
              Number{1'000'000'000'000'000'066, -18}},
             {Number{-1'000'000'000'000'000'000, -18},
              Number{-6'555'555'555'555'555'555, -35},
              Number{-1'000'000'000'000'000'066, -18}},
             {Number{-1'000'000'000'000'000'000, -18},
              Number{6'555'555'555'555'555'555, -35},
              Number{
                  true,
                  9'999'999'999'999'999'344ULL,
                  -19,
                  Number::normalized{}}},
             {Number{-6'555'555'555'555'555'555, -35},
              Number{1'000'000'000'000'000'000, -18},
              Number{
                  false,
                  9'999'999'999'999'999'344ULL,
                  -19,
                  Number::normalized{}}},
             {Number{}, Number{5}, Number{5}},
             {Number{5'555'555'555'555'555'555, -32768},
              Number{-5'555'555'555'555'555'554, -32768},
              Number{0}},
             {Number{
                  true,
                  9'999'999'999'999'999'999ULL,
                  -37,
                  Number::normalized{}},
              Number{1'000'000'000'000'000'000, -18},
              Number{
                  false,
                  9'999'999'999'999'999'990ULL,
                  -19,
                  Number::normalized{}}}});
        auto test = [this](auto const& c) {
            for (auto const& [x, y, z] : c)
            {
                auto const result = x + y;
                std::stringstream ss;
                ss << x << " + " << y << " = " << result << ". Expected: " << z;
                BEAST_EXPECTS(result == z, ss.str());
            }
        };
        if (scale == MantissaRange::small)
            test(cSmall);
        else
            test(cLarge);
        {
            bool caught = false;
            try
            {
                Number{
                    false, Number::maxMantissa(), 32768, Number::normalized{}} +
                    Number{
                        false,
                        Number::minMantissa(),
                        32767,
                        Number::normalized{}} *
                        5;
            }
            catch (std::overflow_error const&)
            {
                caught = true;
            }
            BEAST_EXPECT(caught);
        }
    }

    void
    test_sub()
    {
        auto const scale = Number::getMantissaScale();
        testcase << "test_sub " << to_string(scale);

        using Case = std::tuple<Number, Number, Number>;
        auto const cSmall = std::to_array<Case>(
            {{Number{1'000'000'000'000'000, -15},
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
              Number{1'000'000'000'000'000, -30}}});
        auto const cLarge = std::to_array<Case>(
            // Note that items with extremely large mantissas need to be
            // calculated, because otherwise they overflow uint64. Items from C
            // with larger mantissa
            {{Number{1'000'000'000'000'000, -15},
              Number{6'555'555'555'555'555, -29},
              Number{
                  false,
                  9'999'999'999'999'344'444ULL,
                  -19,
                  Number::normalized{}}},
             {Number{6'555'555'555'555'555, -29},
              Number{1'000'000'000'000'000, -15},
              Number{
                  true,
                  9'999'999'999'999'344'444ULL,
                  -19,
                  Number::normalized{}}},
             {Number{1'000'000'000'000'000, -15},
              Number{1'000'000'000'000'000, -15},
              Number{0}},
             {Number{1'000'000'000'000'000, -15},
              Number{1'000'000'000'000'001, -15},
              Number{-1'000'000'000'000'000, -30}},
             {Number{1'000'000'000'000'001, -15},
              Number{1'000'000'000'000'000, -15},
              Number{1'000'000'000'000'000, -30}},
             // Items from cSmall expanded for the larger mantissa
             {Number{1'000'000'000'000'000'000, -18},
              Number{6'555'555'555'555'555'555, -32},
              Number{
                  false,
                  9'999'999'999'999'344'444ULL,
                  -19,
                  Number::normalized{}}},
             {Number{6'555'555'555'555'555'555, -32},
              Number{1'000'000'000'000'000'000, -18},
              Number{
                  true,
                  9'999'999'999'999'344'444ULL,
                  -19,
                  Number::normalized{}}},
             {Number{1'000'000'000'000'000'000, -18},
              Number{1'000'000'000'000'000'000, -18},
              Number{0}},
             {Number{1'000'000'000'000'000'000, -18},
              Number{1'000'000'000'000'000'001, -18},
              Number{-1'000'000'000'000'000'000, -36}},
             {Number{1'000'000'000'000'000'001, -18},
              Number{1'000'000'000'000'000'000, -18},
              Number{1'000'000'000'000'000'000, -36}}});
        auto test = [this](auto const& c) {
            for (auto const& [x, y, z] : c)
            {
                auto const result = x - y;
                std::stringstream ss;
                ss << x << " - " << y << " = " << result << ". Expected: " << z;
                BEAST_EXPECTS(result == z, ss.str());
            }
        };
        if (scale == MantissaRange::small)
            test(cSmall);
        else
            test(cLarge);
    }

    void
    test_mul()
    {
        auto const scale = Number::getMantissaScale();
        testcase << "test_mul " << to_string(scale);

        using Case = std::tuple<Number, Number, Number>;
        auto test = [this](auto const& c) {
            for (auto const& [x, y, z] : c)
            {
                auto const result = x * y;
                std::stringstream ss;
                ss << x << " * " << y << " = " << result << ". Expected: " << z;
                BEAST_EXPECTS(result == z, ss.str());
            }
        };
        auto tests = [&](auto const& cSmall, auto const& cLarge) {
            if (scale == MantissaRange::small)
                test(cSmall);
            else
                test(cLarge);
        };
        auto const maxMantissa = Number::maxMantissa();
        auto const maxInt64 = std::numeric_limits<std::int64_t>::max();

        saveNumberRoundMode save{Number::setround(Number::to_nearest)};
        {
            auto const cSmall = std::to_array<Case>({
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
                 Number{0}},
                // Maximum mantissa range
                {Number{9'999'999'999'999'999, 0},
                 Number{9'999'999'999'999'999, 0},
                 Number{9'999'999'999'999'998, 16}},
            });
            auto const cLarge = std::to_array<Case>({
                // Note that items with extremely large mantissas need to be
                // calculated, because otherwise they overflow uint64. Items
                // from C with larger mantissa
                {Number{7}, Number{8}, Number{56}},
                {Number{1414213562373095, -15},
                 Number{1414213562373095, -15},
                 Number{1999999999999999862, -18}},
                {Number{-1414213562373095, -15},
                 Number{1414213562373095, -15},
                 Number{-1999999999999999862, -18}},
                {Number{-1414213562373095, -15},
                 Number{-1414213562373095, -15},
                 Number{1999999999999999862, -18}},
                {Number{3214285714285706, -15},
                 Number{3111111111111119, -15},
                 Number{
                     false,
                     9'999'999'999'999'999'579ULL,
                     -18,
                     Number::normalized{}}},
                {Number{1000000000000000000, -32768},
                 Number{1000000000000000000, -32768},
                 Number{0}},
                // Items from cSmall expanded for the larger mantissa,
                // except duplicates. Sadly, it looks like sqrt(2)^2 != 2
                // with higher precision
                {Number{1414213562373095049, -18},
                 Number{1414213562373095049, -18},
                 Number{2000000000000000001, -18}},
                {Number{-1414213562373095048, -18},
                 Number{1414213562373095048, -18},
                 Number{-1999999999999999998, -18}},
                {Number{-1414213562373095048, -18},
                 Number{-1414213562373095049, -18},
                 Number{1999999999999999999, -18}},
                {Number{3214285714285714278, -18},
                 Number{3111111111111111119, -18},
                 Number{10, 0}},
                // Maximum mantissa range - rounds up to 1e19
                {Number{false, maxMantissa, 0, Number::normalized{}},
                 Number{false, maxMantissa, 0, Number::normalized{}},
                 Number{1, 38}},
                // Maximum int64 range
                {Number{maxInt64, 0},
                 Number{maxInt64, 0},
                 Number{85'070'591'730'234'615'85, 19}},
            });
            tests(cSmall, cLarge);
        }
        Number::setround(Number::towards_zero);
        testcase << "test_mul " << to_string(Number::getMantissaScale())
                 << " towards_zero";
        {
            auto const cSmall = std::to_array<Case>(
                {{Number{7}, Number{8}, Number{56}},
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
                  Number{0}}});
            auto const cLarge = std::to_array<Case>(
                // Note that items with extremely large mantissas need to be
                // calculated, because otherwise they overflow uint64. Items
                // from C with larger mantissa
                {
                    {Number{7}, Number{8}, Number{56}},
                    {Number{1414213562373095, -15},
                     Number{1414213562373095, -15},
                     Number{1999999999999999861, -18}},
                    {Number{-1414213562373095, -15},
                     Number{1414213562373095, -15},
                     Number{-1999999999999999861, -18}},
                    {Number{-1414213562373095, -15},
                     Number{-1414213562373095, -15},
                     Number{1999999999999999861, -18}},
                    {Number{3214285714285706, -15},
                     Number{3111111111111119, -15},
                     Number{
                         false,
                         9999999999999999579ULL,
                         -18,
                         Number::normalized{}}},
                    {Number{1000000000000000000, -32768},
                     Number{1000000000000000000, -32768},
                     Number{0}},
                    // Items from cSmall expanded for the larger mantissa,
                    // except duplicates. Sadly, it looks like sqrt(2)^2 != 2
                    // with higher precision
                    {Number{1414213562373095049, -18},
                     Number{1414213562373095049, -18},
                     Number{2, 0}},
                    {Number{-1414213562373095048, -18},
                     Number{1414213562373095048, -18},
                     Number{-1999999999999999997, -18}},
                    {Number{-1414213562373095048, -18},
                     Number{-1414213562373095049, -18},
                     Number{1999999999999999999, -18}},
                    {Number{3214285714285714278, -18},
                     Number{3111111111111111119, -18},
                     Number{10, 0}},
                    // Maximum mantissa range - rounds down to maxMantissa/10e1
                    // 99'999'999'999'999'999'800'000'000'000'000'000'100
                    {Number{false, maxMantissa, 0, Number::normalized{}},
                     Number{false, maxMantissa, 0, Number::normalized{}},
                     Number{
                         false,
                         maxMantissa / 10 - 1,
                         20,
                         Number::normalized{}}},
                    // Maximum int64 range
                    // 85'070'591'730'234'615'847'396'907'784'232'501'249
                    {Number{maxInt64, 0},
                     Number{maxInt64, 0},
                     Number{85'070'591'730'234'615'84, 19}},
                });
            tests(cSmall, cLarge);
        }
        Number::setround(Number::downward);
        testcase << "test_mul " << to_string(Number::getMantissaScale())
                 << " downward";
        {
            auto const cSmall = std::to_array<Case>(
                {{Number{7}, Number{8}, Number{56}},
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
                  Number{0}}});
            auto const cLarge = std::to_array<Case>(
                // Note that items with extremely large mantissas need to be
                // calculated, because otherwise they overflow uint64. Items
                // from C with larger mantissa
                {
                    {Number{7}, Number{8}, Number{56}},
                    {Number{1414213562373095, -15},
                     Number{1414213562373095, -15},
                     Number{1999999999999999861, -18}},
                    {Number{-1414213562373095, -15},
                     Number{1414213562373095, -15},
                     Number{-1999999999999999862, -18}},
                    {Number{-1414213562373095, -15},
                     Number{-1414213562373095, -15},
                     Number{1999999999999999861, -18}},
                    {Number{3214285714285706, -15},
                     Number{3111111111111119, -15},
                     Number{
                         false,
                         9'999'999'999'999'999'579ULL,
                         -18,
                         Number::normalized{}}},
                    {Number{1000000000000000000, -32768},
                     Number{1000000000000000000, -32768},
                     Number{0}},
                    // Items from cSmall expanded for the larger mantissa,
                    // except duplicates. Sadly, it looks like sqrt(2)^2 != 2
                    // with higher precision
                    {Number{1414213562373095049, -18},
                     Number{1414213562373095049, -18},
                     Number{2, 0}},
                    {Number{-1414213562373095048, -18},
                     Number{1414213562373095048, -18},
                     Number{-1999999999999999998, -18}},
                    {Number{-1414213562373095048, -18},
                     Number{-1414213562373095049, -18},
                     Number{1999999999999999999, -18}},
                    {Number{3214285714285714278, -18},
                     Number{3111111111111111119, -18},
                     Number{10, 0}},
                    // Maximum mantissa range - rounds down to maxMantissa/10e1
                    // 99'999'999'999'999'999'800'000'000'000'000'000'100
                    {Number{false, maxMantissa, 0, Number::normalized{}},
                     Number{false, maxMantissa, 0, Number::normalized{}},
                     Number{
                         false,
                         maxMantissa / 10 - 1,
                         20,
                         Number::normalized{}}},
                    // Maximum int64 range
                    // 85'070'591'730'234'615'847'396'907'784'232'501'249
                    {Number{maxInt64, 0},
                     Number{maxInt64, 0},
                     Number{85'070'591'730'234'615'84, 19}},
                });
            tests(cSmall, cLarge);
        }
        Number::setround(Number::upward);
        testcase << "test_mul " << to_string(Number::getMantissaScale())
                 << " upward";
        {
            auto const cSmall = std::to_array<Case>(
                {{Number{7}, Number{8}, Number{56}},
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
                  Number{0}}});
            auto const cLarge = std::to_array<Case>(
                // Note that items with extremely large mantissas need to be
                // calculated, because otherwise they overflow uint64. Items
                // from C with larger mantissa
                {
                    {Number{7}, Number{8}, Number{56}},
                    {Number{1414213562373095, -15},
                     Number{1414213562373095, -15},
                     Number{1999999999999999862, -18}},
                    {Number{-1414213562373095, -15},
                     Number{1414213562373095, -15},
                     Number{-1999999999999999861, -18}},
                    {Number{-1414213562373095, -15},
                     Number{-1414213562373095, -15},
                     Number{1999999999999999862, -18}},
                    {Number{3214285714285706, -15},
                     Number{3111111111111119, -15},
                     Number{999999999999999958, -17}},
                    {Number{1000000000000000000, -32768},
                     Number{1000000000000000000, -32768},
                     Number{0}},
                    // Items from cSmall expanded for the larger mantissa,
                    // except duplicates. Sadly, it looks like sqrt(2)^2 != 2
                    // with higher precision
                    {Number{1414213562373095049, -18},
                     Number{1414213562373095049, -18},
                     Number{2000000000000000001, -18}},
                    {Number{-1414213562373095048, -18},
                     Number{1414213562373095048, -18},
                     Number{-1999999999999999997, -18}},
                    {Number{-1414213562373095048, -18},
                     Number{-1414213562373095049, -18},
                     Number{2, 0}},
                    {Number{3214285714285714278, -18},
                     Number{3111111111111111119, -18},
                     Number{1000000000000000001, -17}},
                    // Maximum mantissa range - rounds up to minMantissa*10
                    // 1e19*1e19=1e38
                    {Number{false, maxMantissa, 0, Number::normalized{}},
                     Number{false, maxMantissa, 0, Number::normalized{}},
                     Number{1, 38}},
                    // Maximum int64 range
                    // 85'070'591'730'234'615'847'396'907'784'232'501'249
                    {Number{maxInt64, 0},
                     Number{maxInt64, 0},
                     Number{85'070'591'730'234'615'85, 19}},
                });
            tests(cSmall, cLarge);
        }
        testcase << "test_mul " << to_string(Number::getMantissaScale())
                 << " overflow";
        {
            bool caught = false;
            try
            {
                Number{false, maxMantissa, 32768, Number::normalized{}} *
                    Number{
                        false,
                        Number::minMantissa() * 5,
                        32767,
                        Number::normalized{}};
            }
            catch (std::overflow_error const&)
            {
                caught = true;
            }
            BEAST_EXPECT(caught);
        }
    }

    void
    test_div()
    {
        auto const scale = Number::getMantissaScale();
        testcase << "test_div " << to_string(scale);

        using Case = std::tuple<Number, Number, Number>;
        auto test = [this](auto const& c) {
            for (auto const& [x, y, z] : c)
            {
                auto const result = x / y;
                std::stringstream ss;
                ss << x << " / " << y << " = " << result << ". Expected: " << z;
                BEAST_EXPECTS(result == z, ss.str());
            }
        };
        auto const maxMantissa = Number::maxMantissa();
        auto tests = [&](auto const& cSmall, auto const& cLarge) {
            if (scale == MantissaRange::small)
                test(cSmall);
            else
                test(cLarge);
        };
        saveNumberRoundMode save{Number::setround(Number::to_nearest)};
        {
            auto const cSmall = std::to_array<Case>(
                {{Number{1}, Number{2}, Number{5, -1}},
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
                 {Number{-2}, Number{3}, Number{-6'666'666'666'666'667, -16}},
                 {Number{1}, Number{7}, Number{1'428'571'428'571'428, -16}}});
            auto const cLarge = std::to_array<Case>(
                // Note that items with extremely large mantissas need to be
                // calculated, because otherwise they overflow uint64. Items
                // from C with larger mantissa
                {{Number{1}, Number{2}, Number{5, -1}},
                 {Number{1}, Number{10}, Number{1, -1}},
                 {Number{1}, Number{-10}, Number{-1, -1}},
                 {Number{0}, Number{100}, Number{0}},
                 {Number{1414213562373095, -10},
                  Number{1414213562373095, -10},
                  Number{1}},
                 {Number{9'999'999'999'999'999},
                  Number{1'000'000'000'000'000},
                  Number{9'999'999'999'999'999, -15}},
                 {Number{2}, Number{3}, Number{6'666'666'666'666'666'667, -19}},
                 {Number{-2},
                  Number{3},
                  Number{-6'666'666'666'666'666'667, -19}},
                 {Number{1}, Number{7}, Number{1'428'571'428'571'428'571, -19}},
                 // Items from cSmall expanded for the larger mantissa, except
                 // duplicates.
                 {Number{1414213562373095049, -13},
                  Number{1414213562373095049, -13},
                  Number{1}},
                 {Number{false, maxMantissa, 0, Number::normalized{}},
                  Number{1'000'000'000'000'000'000},
                  Number{false, maxMantissa, -18, Number::normalized{}}}});
            tests(cSmall, cLarge);
        }
        testcase << "test_div " << to_string(Number::getMantissaScale())
                 << " towards_zero";
        Number::setround(Number::towards_zero);
        {
            auto const cSmall = std::to_array<Case>(
                {{Number{1}, Number{2}, Number{5, -1}},
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
                 {Number{-2}, Number{3}, Number{-6'666'666'666'666'666, -16}},
                 {Number{1}, Number{7}, Number{1'428'571'428'571'428, -16}}});
            auto const cLarge = std::to_array<Case>(
                // Note that items with extremely large mantissas need to be
                // calculated, because otherwise they overflow uint64. Items
                // from C with larger mantissa
                {{Number{1}, Number{2}, Number{5, -1}},
                 {Number{1}, Number{10}, Number{1, -1}},
                 {Number{1}, Number{-10}, Number{-1, -1}},
                 {Number{0}, Number{100}, Number{0}},
                 {Number{1414213562373095, -10},
                  Number{1414213562373095, -10},
                  Number{1}},
                 {Number{9'999'999'999'999'999},
                  Number{1'000'000'000'000'000},
                  Number{9'999'999'999'999'999, -15}},
                 {Number{2}, Number{3}, Number{6'666'666'666'666'666'666, -19}},
                 {Number{-2},
                  Number{3},
                  Number{-6'666'666'666'666'666'666, -19}},
                 {Number{1}, Number{7}, Number{1'428'571'428'571'428'571, -19}},
                 // Items from cSmall expanded for the larger mantissa, except
                 // duplicates.
                 {Number{1414213562373095049, -13},
                  Number{1414213562373095049, -13},
                  Number{1}},
                 {Number{false, maxMantissa, 0, Number::normalized{}},
                  Number{1'000'000'000'000'000'000},
                  Number{false, maxMantissa, -18, Number::normalized{}}}});
            tests(cSmall, cLarge);
        }
        testcase << "test_div " << to_string(Number::getMantissaScale())
                 << " downward";
        Number::setround(Number::downward);
        {
            auto const cSmall = std::to_array<Case>(
                {{Number{1}, Number{2}, Number{5, -1}},
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
                 {Number{-2}, Number{3}, Number{-6'666'666'666'666'667, -16}},
                 {Number{1}, Number{7}, Number{1'428'571'428'571'428, -16}}});
            auto const cLarge = std::to_array<Case>(
                // Note that items with extremely large mantissas need to be
                // calculated, because otherwise they overflow uint64. Items
                // from C with larger mantissa
                {{Number{1}, Number{2}, Number{5, -1}},
                 {Number{1}, Number{10}, Number{1, -1}},
                 {Number{1}, Number{-10}, Number{-1, -1}},
                 {Number{0}, Number{100}, Number{0}},
                 {Number{1414213562373095, -10},
                  Number{1414213562373095, -10},
                  Number{1}},
                 {Number{9'999'999'999'999'999},
                  Number{1'000'000'000'000'000},
                  Number{9'999'999'999'999'999, -15}},
                 {Number{2}, Number{3}, Number{6'666'666'666'666'666'666, -19}},
                 {Number{-2},
                  Number{3},
                  Number{-6'666'666'666'666'666'667, -19}},
                 {Number{1}, Number{7}, Number{1'428'571'428'571'428'571, -19}},
                 // Items from cSmall expanded for the larger mantissa, except
                 // duplicates.
                 {Number{1414213562373095049, -13},
                  Number{1414213562373095049, -13},
                  Number{1}},
                 {Number{false, maxMantissa, 0, Number::normalized{}},
                  Number{1'000'000'000'000'000'000},
                  Number{false, maxMantissa, -18, Number::normalized{}}}});
            tests(cSmall, cLarge);
        }
        testcase << "test_div " << to_string(Number::getMantissaScale())
                 << " upward";
        Number::setround(Number::upward);
        {
            auto const cSmall = std::to_array<Case>(
                {{Number{1}, Number{2}, Number{5, -1}},
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
                 {Number{-2}, Number{3}, Number{-6'666'666'666'666'666, -16}},
                 {Number{1}, Number{7}, Number{1'428'571'428'571'429, -16}}});
            auto const cLarge = std::to_array<Case>(
                // Note that items with extremely large mantissas need to be
                // calculated, because otherwise they overflow uint64. Items
                // from C with larger mantissa
                {{Number{1}, Number{2}, Number{5, -1}},
                 {Number{1}, Number{10}, Number{1, -1}},
                 {Number{1}, Number{-10}, Number{-1, -1}},
                 {Number{0}, Number{100}, Number{0}},
                 {Number{1414213562373095, -10},
                  Number{1414213562373095, -10},
                  Number{1}},
                 {Number{9'999'999'999'999'999},
                  Number{1'000'000'000'000'000},
                  Number{9'999'999'999'999'999, -15}},
                 {Number{2}, Number{3}, Number{6'666'666'666'666'666'667, -19}},
                 {Number{-2},
                  Number{3},
                  Number{-6'666'666'666'666'666'666, -19}},
                 {Number{1}, Number{7}, Number{1'428'571'428'571'428'572, -19}},
                 // Items from cSmall expanded for the larger mantissa, except
                 // duplicates.
                 {Number{1414213562373095049, -13},
                  Number{1414213562373095049, -13},
                  Number{1}},
                 {Number{false, maxMantissa, 0, Number::normalized{}},
                  Number{1'000'000'000'000'000'000},
                  Number{false, maxMantissa, -18, Number::normalized{}}}});
            tests(cSmall, cLarge);
        }
        testcase << "test_div " << to_string(Number::getMantissaScale())
                 << " overflow";
        bool caught = false;
        try
        {
            Number{1000000000000000, -15} / Number{0};
        }
        catch (std::overflow_error const&)
        {
            caught = true;
        }
        BEAST_EXPECT(caught);
    }

    void
    test_root()
    {
        auto const scale = Number::getMantissaScale();
        testcase << "test_root " << to_string(scale);

        using Case = std::tuple<Number, unsigned, Number>;
        auto test = [this](auto const& c) {
            for (auto const& [x, y, z] : c)
            {
                auto const result = root(x, y);
                std::stringstream ss;
                ss << "root(" << x << ", " << y << ") = " << result
                   << ". Expected: " << z;
                BEAST_EXPECTS(result == z, ss.str());
            }
        };
        /*
        auto tests = [&](auto const& cSmall, auto const& cLarge) {
            test(cSmall);
            if (scale != MantissaRange::small)
                test(cLarge);
        };
        */

        auto const cSmall = std::to_array<Case>(
            {{Number{2}, 2, Number{1414213562373095049, -18}},
             {Number{2'000'000}, 2, Number{1414213562373095049, -15}},
             {Number{2, -30}, 2, Number{1414213562373095049, -33}},
             {Number{-27}, 3, Number{-3}},
             {Number{1}, 5, Number{1}},
             {Number{-1}, 0, Number{1}},
             {Number{5, -1}, 0, Number{0}},
             {Number{0}, 5, Number{0}},
             {Number{5625, -4}, 2, Number{75, -2}}});
        auto const cLarge = std::to_array<Case>({
            {Number{false, Number::maxMantissa() - 9, -1, Number::normalized{}},
             2,
             Number{false, 999'999'999'999'999'999, -9, Number::normalized{}}},
            {Number{false, Number::maxMantissa() - 9, 0, Number::normalized{}},
             2,
             Number{
                 false, 3'162'277'660'168'379'330, -9, Number::normalized{}}},
        });
        test(cSmall);
        if (Number::getMantissaScale() != MantissaRange::small)
        {
            NumberRoundModeGuard mg(Number::towards_zero);
            test(cLarge);
        }
        bool caught = false;
        try
        {
            (void)root(Number{-2}, 0);
        }
        catch (std::overflow_error const&)
        {
            caught = true;
        }
        BEAST_EXPECT(caught);
        caught = false;
        try
        {
            (void)root(Number{-2}, 4);
        }
        catch (std::overflow_error const&)
        {
            caught = true;
        }
        BEAST_EXPECT(caught);
    }

    void
    test_root2()
    {
        auto const scale = Number::getMantissaScale();
        testcase << "test_root2 " << to_string(scale);

        auto test = [this](auto const& c) {
            for (auto const& x : c)
            {
                auto const expected = root(x, 2);
                auto const result = root2(x);
                std::stringstream ss;
                ss << "root2(" << x << ") = " << result
                   << ". Expected: " << expected;
                BEAST_EXPECTS(result == expected, ss.str());
            }
        };

        auto const cSmall = std::to_array<Number>(
            {Number{2},
             Number{2'000'000},
             Number{2, -30},
             Number{27},
             Number{1},
             Number{5, -1},
             Number{0},
             Number{5625, -4}});
        test(cSmall);
        bool caught = false;
        try
        {
            (void)root2(Number{-2});
        }
        catch (std::overflow_error const&)
        {
            caught = true;
        }
        BEAST_EXPECT(caught);
    }

    void
    test_power1()
    {
        testcase << "test_power1 " << to_string(Number::getMantissaScale());
        using Case = std::tuple<Number, unsigned, Number>;
        Case c[]{
            {Number{64}, 0, Number{1}},
            {Number{64}, 1, Number{64}},
            {Number{64}, 2, Number{4096}},
            {Number{-64}, 2, Number{4096}},
            {Number{64}, 3, Number{262144}},
            {Number{-64}, 3, Number{-262144}},
            {Number{64},
             11,
             Number{false, 7378697629483820646ULL, 1, Number::normalized{}}},
            {Number{-64},
             11,
             Number{true, 7378697629483820646ULL, 1, Number::normalized{}}}};
        for (auto const& [x, y, z] : c)
            BEAST_EXPECT((power(x, y) == z));
    }

    void
    test_power2()
    {
        testcase << "test_power2 " << to_string(Number::getMantissaScale());
        using Case = std::tuple<Number, unsigned, unsigned, Number>;
        Case c[]{
            {Number{1}, 3, 7, Number{1}},
            {Number{-1}, 1, 0, Number{1}},
            {Number{-1, -1}, 1, 0, Number{0}},
            {Number{16}, 0, 5, Number{1}},
            {Number{34}, 3, 3, Number{34}},
            {Number{4}, 3, 2, Number{8}}};
        for (auto const& [x, n, d, z] : c)
            BEAST_EXPECT((power(x, n, d) == z));
        bool caught = false;
        try
        {
            (void)power(Number{7}, 0, 0);
        }
        catch (std::overflow_error const&)
        {
            caught = true;
        }
        BEAST_EXPECT(caught);
        caught = false;
        try
        {
            (void)power(Number{7}, 1, 0);
        }
        catch (std::overflow_error const&)
        {
            caught = true;
        }
        BEAST_EXPECT(caught);
        caught = false;
        try
        {
            (void)power(Number{-1, -1}, 3, 2);
        }
        catch (std::overflow_error const&)
        {
            caught = true;
        }
        BEAST_EXPECT(caught);
    }

    void
    testConversions()
    {
        testcase << "testConversions " << to_string(Number::getMantissaScale());

        IOUAmount x{5, 6};
        Number y = x;
        BEAST_EXPECT((y == Number{5, 6}));
        IOUAmount z{y};
        BEAST_EXPECT(x == z);
        XRPAmount xrp{500};
        STAmount st = xrp;
        Number n = st;
        BEAST_EXPECT(XRPAmount{n} == xrp);
        IOUAmount x0{0, 0};
        Number y0 = x0;
        BEAST_EXPECT((y0 == Number{0}));
        IOUAmount z0{y0};
        BEAST_EXPECT(x0 == z0);
        XRPAmount xrp0{0};
        Number n0 = xrp0;
        BEAST_EXPECT(n0 == Number{0});
        XRPAmount xrp1{n0};
        BEAST_EXPECT(xrp1 == xrp0);
    }

    void
    test_to_integer()
    {
        testcase << "test_to_integer " << to_string(Number::getMantissaScale());
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
                BEAST_EXPECT(j == y);
            }
        }
        auto prev_mode = Number::setround(Number::towards_zero);
        BEAST_EXPECT(prev_mode == Number::to_nearest);
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
                BEAST_EXPECT(j == y);
            }
        }
        prev_mode = Number::setround(Number::downward);
        BEAST_EXPECT(prev_mode == Number::towards_zero);
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
                BEAST_EXPECT(j == y);
            }
        }
        prev_mode = Number::setround(Number::upward);
        BEAST_EXPECT(prev_mode == Number::downward);
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
                BEAST_EXPECT(j == y);
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
        BEAST_EXPECT(caught);
    }

    void
    test_squelch()
    {
        testcase << "test_squelch " << to_string(Number::getMantissaScale());
        Number limit{1, -6};
        BEAST_EXPECT((squelch(Number{2, -6}, limit) == Number{2, -6}));
        BEAST_EXPECT((squelch(Number{1, -6}, limit) == Number{1, -6}));
        BEAST_EXPECT((squelch(Number{9, -7}, limit) == Number{0}));
        BEAST_EXPECT((squelch(Number{-2, -6}, limit) == Number{-2, -6}));
        BEAST_EXPECT((squelch(Number{-1, -6}, limit) == Number{-1, -6}));
        BEAST_EXPECT((squelch(Number{-9, -7}, limit) == Number{0}));
    }

    void
    testToString()
    {
        auto const scale = Number::getMantissaScale();
        testcase << "testToString " << to_string(scale);

        auto test = [this](Number const& n, std::string const& expected) {
            auto const result = to_string(n);
            std::stringstream ss;
            ss << "to_string(" << result << "). Expected: " << expected;
            BEAST_EXPECTS(result == expected, ss.str());
        };

        test(Number(-2, 0), "-2");
        test(Number(0, 0), "0");
        test(Number(2, 0), "2");
        test(Number(25, -3), "0.025");
        test(Number(-25, -3), "-0.025");
        test(Number(25, 1), "250");
        test(Number(-25, 1), "-250");
        switch (scale)
        {
            case MantissaRange::small:
                test(Number(2, 20), "2000000000000000e5");
                test(Number(-2, -20), "-2000000000000000e-35");
                // Test the edges
                // ((exponent < -(25)) || (exponent > -(5)))))
                test(Number(2, -10), "0.0000000002");
                test(Number(2, -11), "2000000000000000e-26");

                test(Number(-2, 10), "-20000000000");
                test(Number(-2, 11), "-2000000000000000e-4");

                test(Number::min(), "1000000000000000e-32768");
                test(Number::max(), "9999999999999999e32768");
                test(Number::lowest(), "-9999999999999999e32768");
                {
                    NumberRoundModeGuard mg(Number::towards_zero);

                    auto const maxMantissa = Number::maxMantissa();
                    BEAST_EXPECT(maxMantissa == 9'999'999'999'999'999);
                    test(
                        Number{
                            false,
                            maxMantissa * 1000 + 999,
                            -3,
                            Number::normalized()},
                        "9999999999999999");
                    test(
                        Number{
                            true,
                            maxMantissa * 1000 + 999,
                            -3,
                            Number::normalized()},
                        "-9999999999999999");

                    test(
                        Number{std::numeric_limits<std::int64_t>::max(), -3},
                        "9223372036854775");
                    test(
                        -(Number{std::numeric_limits<std::int64_t>::max(), -3}),
                        "-9223372036854775");
                }
                break;
            case MantissaRange::large:
                test(Number(2, 20), "2000000000000000000e2");
                test(Number(-2, -20), "-2000000000000000000e-38");
                // Test the edges
                // ((exponent < -(28)) || (exponent > -(8)))))
                test(Number(2, -10), "0.0000000002");
                test(Number(2, -11), "2000000000000000000e-29");

                test(Number(-2, 10), "-20000000000");
                test(Number(-2, 11), "-2000000000000000000e-7");

                test(Number::min(), "1000000000000000000e-32768");
                test(Number::max(), "9223372036854775807e32768");
                test(Number::lowest(), "-9223372036854775807e32768");
                {
                    NumberRoundModeGuard mg(Number::towards_zero);

                    auto const maxMantissa = Number::maxMantissa();
                    BEAST_EXPECT(maxMantissa == 9'999'999'999'999'999'999ULL);
                    test(
                        Number{false, maxMantissa, 0, Number::normalized{}},
                        "9999999999999999990");
                    test(
                        Number{true, maxMantissa, 0, Number::normalized{}},
                        "-9999999999999999990");

                    test(
                        Number{std::numeric_limits<std::int64_t>::max(), 0},
                        "9223372036854775807");
                    test(
                        -(Number{std::numeric_limits<std::int64_t>::max(), 0}),
                        "-9223372036854775807");
                }
                break;
            default:
                BEAST_EXPECT(false);
        }
    }

    void
    test_relationals()
    {
        testcase << "test_relationals "
                 << to_string(Number::getMantissaScale());
        BEAST_EXPECT(!(Number{100} < Number{10}));
        BEAST_EXPECT(Number{100} > Number{10});
        BEAST_EXPECT(Number{100} >= Number{10});
        BEAST_EXPECT(!(Number{100} <= Number{10}));
    }

    void
    test_stream()
    {
        testcase << "test_stream " << to_string(Number::getMantissaScale());
        Number x{100};
        std::ostringstream os;
        os << x;
        BEAST_EXPECT(os.str() == to_string(x));
    }

    void
    test_inc_dec()
    {
        testcase << "test_inc_dec " << to_string(Number::getMantissaScale());
        Number x{100};
        Number y = +x;
        BEAST_EXPECT(x == y);
        BEAST_EXPECT(x++ == y);
        BEAST_EXPECT(x == Number{101});
        BEAST_EXPECT(x-- == Number{101});
        BEAST_EXPECT(x == y);
    }

    void
    test_toSTAmount()
    {
        NumberSO stNumberSO{true};
        Issue const issue;
        Number const n{7'518'783'80596, -5};
        saveNumberRoundMode const save{Number::setround(Number::to_nearest)};
        auto res2 = STAmount{issue, n};
        BEAST_EXPECT(res2 == STAmount{7518784});

        Number::setround(Number::towards_zero);
        res2 = STAmount{issue, n};
        BEAST_EXPECT(res2 == STAmount{7518783});

        Number::setround(Number::downward);
        res2 = STAmount{issue, n};
        BEAST_EXPECT(res2 == STAmount{7518783});

        Number::setround(Number::upward);
        res2 = STAmount{issue, n};
        BEAST_EXPECT(res2 == STAmount{7518784});
    }

    void
    test_truncate()
    {
        BEAST_EXPECT(Number(25, +1).truncate() == Number(250, 0));
        BEAST_EXPECT(Number(25, 0).truncate() == Number(25, 0));
        BEAST_EXPECT(Number(25, -1).truncate() == Number(2, 0));
        BEAST_EXPECT(Number(25, -2).truncate() == Number(0, 0));
        BEAST_EXPECT(Number(99, -2).truncate() == Number(0, 0));

        BEAST_EXPECT(Number(-25, +1).truncate() == Number(-250, 0));
        BEAST_EXPECT(Number(-25, 0).truncate() == Number(-25, 0));
        BEAST_EXPECT(Number(-25, -1).truncate() == Number(-2, 0));
        BEAST_EXPECT(Number(-25, -2).truncate() == Number(0, 0));
        BEAST_EXPECT(Number(-99, -2).truncate() == Number(0, 0));

        BEAST_EXPECT(Number(0, 0).truncate() == Number(0, 0));
        BEAST_EXPECT(Number(0, 30000).truncate() == Number(0, 0));
        BEAST_EXPECT(Number(0, -30000).truncate() == Number(0, 0));
        BEAST_EXPECT(Number(100, -30000).truncate() == Number(0, 0));
        BEAST_EXPECT(Number(100, -30000).truncate() == Number(0, 0));
        BEAST_EXPECT(Number(-100, -30000).truncate() == Number(0, 0));
        BEAST_EXPECT(Number(-100, -30000).truncate() == Number(0, 0));
    }

    void
    testInt64()
    {
        auto const scale = Number::getMantissaScale();
        testcase << "std::int64_t " << to_string(scale);

        // Control case
        BEAST_EXPECT(Number::maxMantissa() > 10);
        Number ten{10};
        BEAST_EXPECT(ten.exponent() <= 0);

        if (scale == MantissaRange::small)
        {
            BEAST_EXPECT(
                std::numeric_limits<std::int64_t>::max() > INITIAL_XRP.drops());
            BEAST_EXPECT(Number::maxMantissa() < INITIAL_XRP.drops());
            Number const initalXrp{INITIAL_XRP};
            BEAST_EXPECT(initalXrp.exponent() > 0);

            Number const maxInt64{std::numeric_limits<std::int64_t>::max()};
            BEAST_EXPECT(maxInt64.exponent() > 0);
            // 85'070'591'730'234'615'865'843'651'857'942'052'864 - 38 digits
            BEAST_EXPECT(
                (power(maxInt64, 2) == Number{85'070'591'730'234'62, 22}));

            Number const max =
                Number{false, Number::maxMantissa(), 0, Number::normalized{}};
            BEAST_EXPECT(max.exponent() <= 0);
            // 99'999'999'999'999'980'000'000'000'000'001 - 32 digits
            BEAST_EXPECT((power(max, 2) == Number{99'999'999'999'999'98, 16}));
        }
        else
        {
            BEAST_EXPECT(
                std::numeric_limits<std::int64_t>::max() > INITIAL_XRP.drops());
            BEAST_EXPECT(Number::maxMantissa() > INITIAL_XRP.drops());
            Number const initalXrp{INITIAL_XRP};
            BEAST_EXPECT(initalXrp.exponent() <= 0);

            Number const maxInt64{std::numeric_limits<std::int64_t>::max()};
            BEAST_EXPECT(maxInt64.exponent() <= 0);
            // 85'070'591'730'234'615'847'396'907'784'232'501'249 - 38 digits
            BEAST_EXPECT(
                (power(maxInt64, 2) == Number{85'070'591'730'234'615'85, 19}));

            NumberRoundModeGuard mg(Number::towards_zero);

            auto const maxMantissa = Number::maxMantissa();
            Number const max =
                Number{false, maxMantissa, 0, Number::normalized{}};
            BEAST_EXPECT(max.mantissa() == maxMantissa / 10);
            BEAST_EXPECT(max.exponent() == 1);
            // 99'999'999'999'999'999'800'000'000'000'000'000'100 - also 38
            // digits
            BEAST_EXPECT((
                power(max, 2) ==
                Number{false, maxMantissa / 10 - 1, 20, Number::normalized{}}));
        }
    }

    void
    run() override
    {
        for (auto const scale : {MantissaRange::small, MantissaRange::large})
        {
            NumberMantissaScaleGuard sg(scale);
            testZero();
            test_limits();
            testToString();
            test_add();
            test_sub();
            test_mul();
            test_div();
            test_root();
            test_root2();
            test_power1();
            test_power2();
            testConversions();
            test_to_integer();
            test_squelch();
            test_relationals();
            test_stream();
            test_inc_dec();
            test_toSTAmount();
            test_truncate();
            testInt64();
        }
    }
};

BEAST_DEFINE_TESTSUITE(Number, basics, ripple);

}  // namespace ripple

#include <xrpl/protocol/SeqProxy.h>

#include <doctest/doctest.h>

#include <limits>
#include <sstream>

using namespace xrpl;

TEST_SUITE_BEGIN("SeqProxy");

namespace {

// Exercise value(), isSeq(), and isTicket().
constexpr bool
expectValues(SeqProxy seqProx, std::uint32_t value, SeqProxy::Type type)
{
    bool const expectSeq{type == SeqProxy::seq};
    return (seqProx.value() == value) && (seqProx.isSeq() == expectSeq) &&
        (seqProx.isTicket() == !expectSeq);
}

// Exercise all SeqProxy comparison operators expecting lhs < rhs.
constexpr bool
expectLt(SeqProxy lhs, SeqProxy rhs)
{
    return (lhs < rhs) && (lhs <= rhs) && (!(lhs == rhs)) && (lhs != rhs) &&
        (!(lhs >= rhs)) && (!(lhs > rhs));
}

// Exercise all SeqProxy comparison operators expecting lhs == rhs.
constexpr bool
expectEq(SeqProxy lhs, SeqProxy rhs)
{
    return (!(lhs < rhs)) && (lhs <= rhs) && (lhs == rhs) && (!(lhs != rhs)) &&
        (lhs >= rhs) && (!(lhs > rhs));
}

// Exercise all SeqProxy comparison operators expecting lhs > rhs.
constexpr bool
expectGt(SeqProxy lhs, SeqProxy rhs)
{
    return (!(lhs < rhs)) && (!(lhs <= rhs)) && (!(lhs == rhs)) &&
        (lhs != rhs) && (lhs >= rhs) && (lhs > rhs);
}

// Verify streaming.
bool
streamTest(SeqProxy seqProx)
{
    std::string const type{seqProx.isSeq() ? "sequence" : "ticket"};
    std::string const value{std::to_string(seqProx.value())};

    std::stringstream ss;
    ss << seqProx;
    std::string str{ss.str()};

    return str.find(type) == 0 && str[type.size()] == ' ' &&
        str.find(value) == (type.size() + 1);
}

}  // namespace

TEST_CASE("SeqProxy operations")
{
    // While SeqProxy supports values of zero, they are not
    // expected in the wild.  Nevertheless they are tested here.
    // But so are values of 1, which are expected to occur in the wild.
    static constexpr std::uint32_t uintMax{
        std::numeric_limits<std::uint32_t>::max()};
    static constexpr SeqProxy::Type seq{SeqProxy::seq};
    static constexpr SeqProxy::Type ticket{SeqProxy::ticket};

    static constexpr SeqProxy seqZero{seq, 0};
    static constexpr SeqProxy seqSmall{seq, 1};
    static constexpr SeqProxy seqMid0{seq, 2};
    static constexpr SeqProxy seqMid1{seqMid0};
    static constexpr SeqProxy seqBig{seq, uintMax};

    static constexpr SeqProxy ticZero{ticket, 0};
    static constexpr SeqProxy ticSmall{ticket, 1};
    static constexpr SeqProxy ticMid0{ticket, 2};
    static constexpr SeqProxy ticMid1{ticMid0};
    static constexpr SeqProxy ticBig{ticket, uintMax};

    SUBCASE("value(), isSeq() and isTicket()")
    {
        static_assert(expectValues(seqZero, 0, seq), "");
        static_assert(expectValues(seqSmall, 1, seq), "");
        static_assert(expectValues(seqMid0, 2, seq), "");
        static_assert(expectValues(seqMid1, 2, seq), "");
        static_assert(expectValues(seqBig, uintMax, seq), "");

        static_assert(expectValues(ticZero, 0, ticket), "");
        static_assert(expectValues(ticSmall, 1, ticket), "");
        static_assert(expectValues(ticMid0, 2, ticket), "");
        static_assert(expectValues(ticMid1, 2, ticket), "");
        static_assert(expectValues(ticBig, uintMax, ticket), "");
    }

    SUBCASE("comparison operators - seqZero")
    {
        static_assert(expectEq(seqZero, seqZero), "");
        static_assert(expectLt(seqZero, seqSmall), "");
        static_assert(expectLt(seqZero, seqMid0), "");
        static_assert(expectLt(seqZero, seqMid1), "");
        static_assert(expectLt(seqZero, seqBig), "");
        static_assert(expectLt(seqZero, ticZero), "");
        static_assert(expectLt(seqZero, ticSmall), "");
        static_assert(expectLt(seqZero, ticMid0), "");
        static_assert(expectLt(seqZero, ticMid1), "");
        static_assert(expectLt(seqZero, ticBig), "");
    }

    SUBCASE("comparison operators - seqSmall")
    {
        static_assert(expectGt(seqSmall, seqZero), "");
        static_assert(expectEq(seqSmall, seqSmall), "");
        static_assert(expectLt(seqSmall, seqMid0), "");
        static_assert(expectLt(seqSmall, seqBig), "");
        static_assert(expectLt(seqSmall, ticZero), "");
    }

    SUBCASE("comparison operators - seqBig")
    {
        static_assert(expectGt(seqBig, seqZero), "");
        static_assert(expectGt(seqBig, seqSmall), "");
        static_assert(expectEq(seqBig, seqBig), "");
        static_assert(expectLt(seqBig, ticZero), "");
    }

    SUBCASE("comparison operators - ticBig")
    {
        static_assert(expectGt(ticBig, seqZero), "");
        static_assert(expectGt(ticBig, seqBig), "");
        static_assert(expectGt(ticBig, ticZero), "");
        static_assert(expectEq(ticBig, ticBig), "");
    }

    SUBCASE("streaming")
    {
        CHECK(streamTest(seqZero));
        CHECK(streamTest(seqSmall));
        CHECK(streamTest(seqMid0));
        CHECK(streamTest(seqMid1));
        CHECK(streamTest(seqBig));
        CHECK(streamTest(ticZero));
        CHECK(streamTest(ticSmall));
        CHECK(streamTest(ticMid0));
        CHECK(streamTest(ticMid1));
        CHECK(streamTest(ticBig));
    }
}

TEST_SUITE_END();


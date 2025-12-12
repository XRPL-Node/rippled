#include <xrpl/protocol/ApiVersion.h>

#include <doctest/doctest.h>

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

namespace ripple {
namespace test {

TEST_SUITE_BEGIN("protocol");

TEST_CASE("ApiVersion_test")
{
    static_assert(
        xrpl::RPC::apiMinimumSupportedVersion <=
        xrpl::RPC::apiMaximumSupportedVersion);
    static_assert(
        xrpl::RPC::apiMinimumSupportedVersion <= xrpl::RPC::apiMaximumValidVersion);
    static_assert(
        xrpl::RPC::apiMaximumSupportedVersion <= xrpl::RPC::apiMaximumValidVersion);
    static_assert(xrpl::RPC::apiBetaVersion <= xrpl::RPC::apiMaximumValidVersion);

    CHECK(true);
}

TEST_SUITE_END();

}  // namespace test
}  // namespace ripple

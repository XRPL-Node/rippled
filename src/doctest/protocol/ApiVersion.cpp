#include <xrpl/protocol/ApiVersion.h>

#include <doctest/doctest.h>

using namespace xrpl;

TEST_SUITE_BEGIN("ApiVersion");

TEST_CASE("API versions invariants")
{
    static_assert(
        RPC::apiMinimumSupportedVersion <= RPC::apiMaximumSupportedVersion);
    static_assert(
        RPC::apiMinimumSupportedVersion <= RPC::apiMaximumValidVersion);
    static_assert(
        RPC::apiMaximumSupportedVersion <= RPC::apiMaximumValidVersion);
    static_assert(RPC::apiBetaVersion <= RPC::apiMaximumValidVersion);

    CHECK(true);
}

TEST_CASE("API versions")
{
    // Update when we change versions
    static_assert(RPC::apiMinimumSupportedVersion >= 1);
    static_assert(RPC::apiMinimumSupportedVersion < 2);
    static_assert(RPC::apiMaximumSupportedVersion >= 2);
    static_assert(RPC::apiMaximumSupportedVersion < 3);
    static_assert(RPC::apiMaximumValidVersion >= 3);
    static_assert(RPC::apiMaximumValidVersion < 4);
    static_assert(RPC::apiBetaVersion >= 3);
    static_assert(RPC::apiBetaVersion < 4);

    CHECK(true);
}

TEST_SUITE_END();

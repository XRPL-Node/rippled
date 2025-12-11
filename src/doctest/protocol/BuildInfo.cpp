#include <xrpl/protocol/BuildInfo.h>

#include <doctest/doctest.h>

using namespace xrpl;

TEST_SUITE_BEGIN("BuildInfo");

TEST_CASE("EncodeSoftwareVersion")
{
    auto encodedVersion = BuildInfo::encodeSoftwareVersion("1.2.3-b7");

    SUBCASE("first two bytes identify the particular implementation, 0x183B")
    {
        CHECK(
            (encodedVersion & 0xFFFF'0000'0000'0000LLU) ==
            0x183B'0000'0000'0000LLU);
    }

    SUBCASE("next three bytes: major, minor, patch version")
    {
        CHECK(
            (encodedVersion & 0x0000'FFFF'FF00'0000LLU) ==
            0x0000'0102'0300'0000LLU);
    }

    SUBCASE("next two bits indicate release type")
    {
        // 01 if a beta
        auto betaBits = (encodedVersion & 0x0000'0000'00C0'0000LLU) >> 22;
        CHECK(betaBits == 0b01);

        // 10 if an RC
        auto rcVersion = BuildInfo::encodeSoftwareVersion("1.2.4-rc7");
        auto rcBits = (rcVersion & 0x0000'0000'00C0'0000LLU) >> 22;
        CHECK(rcBits == 0b10);

        // 11 if neither an RC nor a beta
        auto releaseVersion = BuildInfo::encodeSoftwareVersion("1.2.5");
        auto releaseBits = (releaseVersion & 0x0000'0000'00C0'0000LLU) >> 22;
        CHECK(releaseBits == 0b11);
    }

    SUBCASE("next six bits: rc/beta number (1-63)")
    {
        auto v = BuildInfo::encodeSoftwareVersion("1.2.6-b63");
        auto betaNum = (v & 0x0000'0000'003F'0000LLU) >> 16;
        CHECK(betaNum == 63);
    }

    SUBCASE("last two bytes are zeros")
    {
        CHECK((encodedVersion & 0x0000'0000'0000'FFFFLLU) == 0);
    }

    SUBCASE("wrong format version strings")
    {
        // no rc/beta number
        auto v1 = BuildInfo::encodeSoftwareVersion("1.2.3-b");
        CHECK((v1 & 0x0000'0000'00FF'0000LLU) == 0);

        // rc/beta number out of range
        auto v2 = BuildInfo::encodeSoftwareVersion("1.2.3-b64");
        CHECK((v2 & 0x0000'0000'00FF'0000LLU) == 0);
    }

    SUBCASE("rc/beta number of a release is 0")
    {
        auto v = BuildInfo::encodeSoftwareVersion("1.2.6");
        CHECK((v & 0x0000'0000'003F'0000LLU) == 0);
    }
}

TEST_CASE("IsRippledVersion")
{
    auto vFF = 0xFFFF'FFFF'FFFF'FFFFLLU;
    CHECK_FALSE(BuildInfo::isRippledVersion(vFF));

    auto vRippled = 0x183B'0000'0000'0000LLU;
    CHECK(BuildInfo::isRippledVersion(vRippled));
}

TEST_CASE("IsNewerVersion")
{
    auto vFF = 0xFFFF'FFFF'FFFF'FFFFLLU;
    CHECK_FALSE(BuildInfo::isNewerVersion(vFF));

    auto v159 = BuildInfo::encodeSoftwareVersion("1.5.9");
    CHECK_FALSE(BuildInfo::isNewerVersion(v159));

    auto vCurrent = BuildInfo::getEncodedVersion();
    CHECK_FALSE(BuildInfo::isNewerVersion(vCurrent));

    auto vMax = BuildInfo::encodeSoftwareVersion("255.255.255");
    CHECK(BuildInfo::isNewerVersion(vMax));
}

TEST_SUITE_END();

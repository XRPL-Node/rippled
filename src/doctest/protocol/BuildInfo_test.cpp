#include <xrpl/protocol/BuildInfo.h>

#include <doctest/doctest.h>

namespace BuildInfo = xrpl::BuildInfo;

namespace ripple {

TEST_SUITE_BEGIN("protocol");

TEST_CASE("BuildInfo_test - EncodeSoftwareVersion")
{

        auto encodedVersion = BuildInfo::encodeSoftwareVersion("1.2.3-b7");

        // the first two bytes identify the particular implementation, 0x183B
        CHECK(
            (encodedVersion & 0xFFFF'0000'0000'0000LLU) ==
            0x183B'0000'0000'0000LLU);

        // the next three bytes: major version, minor version, patch version,
        // 0x010203
        CHECK(
            (encodedVersion & 0x0000'FFFF'FF00'0000LLU) ==
            0x0000'0102'0300'0000LLU);

        // the next two bits:
        {
            // 01 if a beta
            CHECK(
                ((encodedVersion & 0x0000'0000'00C0'0000LLU) >> 22) == 0b01);
            // 10 if an RC
            encodedVersion = BuildInfo::encodeSoftwareVersion("1.2.4-rc7");
            CHECK(
                ((encodedVersion & 0x0000'0000'00C0'0000LLU) >> 22) == 0b10);
            // 11 if neither an RC nor a beta
            encodedVersion = BuildInfo::encodeSoftwareVersion("1.2.5");
            CHECK(
                ((encodedVersion & 0x0000'0000'00C0'0000LLU) >> 22) == 0b11);
        }

        // the next six bits: rc/beta number (1-63)
        encodedVersion = BuildInfo::encodeSoftwareVersion("1.2.6-b63");
        CHECK(((encodedVersion & 0x0000'0000'003F'0000LLU) >> 16) == 63);

        // the last two bytes are zeros
        CHECK((encodedVersion & 0x0000'0000'0000'FFFFLLU) == 0);

        // Test some version strings with wrong formats:
        // no rc/beta number
        encodedVersion = BuildInfo::encodeSoftwareVersion("1.2.3-b");
        CHECK((encodedVersion & 0x0000'0000'00FF'0000LLU) == 0);
        // rc/beta number out of range
        encodedVersion = BuildInfo::encodeSoftwareVersion("1.2.3-b64");
        CHECK((encodedVersion & 0x0000'0000'00FF'0000LLU) == 0);

        // Check that the rc/beta number of a release is 0:
        encodedVersion = BuildInfo::encodeSoftwareVersion("1.2.6");
        CHECK((encodedVersion & 0x0000'0000'003F'0000LLU) == 0);
    
}

TEST_CASE("BuildInfo_test - IsRippledVersion")
{
        auto vFF = 0xFFFF'FFFF'FFFF'FFFFLLU;
        CHECK(!BuildInfo::isRippledVersion(vFF));
        auto vRippled = 0x183B'0000'0000'0000LLU;
        CHECK(BuildInfo::isRippledVersion(vRippled));
    
}

TEST_CASE("BuildInfo_test - IsNewerVersion")
{
        auto vFF = 0xFFFF'FFFF'FFFF'FFFFLLU;
        CHECK(!BuildInfo::isNewerVersion(vFF));

        auto v159 = BuildInfo::encodeSoftwareVersion("1.5.9");
        CHECK(!BuildInfo::isNewerVersion(v159));

        auto vCurrent = BuildInfo::getEncodedVersion();
        CHECK(!BuildInfo::isNewerVersion(vCurrent));

        auto vMax = BuildInfo::encodeSoftwareVersion("255.255.255");
        CHECK(BuildInfo::isNewerVersion(vMax));
    
}

TEST_SUITE_END();

}  // namespace ripple

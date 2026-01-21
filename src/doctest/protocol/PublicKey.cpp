#include <xrpl/protocol/PublicKey.h>
#include <xrpl/protocol/SecretKey.h>

#include <doctest/doctest.h>

#include <vector>

using namespace xrpl;

TEST_SUITE_BEGIN("PublicKey");

namespace {

using blob = std::vector<std::uint8_t>;

template <class FwdIter, class Container>
void
hex_to_binary(FwdIter first, FwdIter last, Container& out)
{
    struct Table
    {
        int val[256];
        Table()
        {
            std::fill(val, val + 256, 0);
            for (int i = 0; i < 10; ++i)
                val['0' + i] = i;
            for (int i = 0; i < 6; ++i)
            {
                val['A' + i] = 10 + i;
                val['a' + i] = 10 + i;
            }
        }
        int
        operator[](int i)
        {
            return val[i];
        }
    };

    static Table lut;
    out.reserve(std::distance(first, last) / 2);
    while (first != last)
    {
        auto const hi(lut[(*first++)]);
        auto const lo(lut[(*first++)]);
        out.push_back((hi * 16) + lo);
    }
}

blob
sig(std::string const& hex)
{
    blob b;
    hex_to_binary(hex.begin(), hex.end(), b);
    return b;
}

bool
check(std::optional<ECDSACanonicality> answer, std::string const& s)
{
    return ecdsaCanonicality(makeSlice(sig(s))) == answer;
}

}  // namespace

TEST_CASE("Canonical")
{
    SUBCASE("Fully canonical")
    {
        CHECK(check(
            ECDSACanonicality::fullyCanonical,
            "3045"
            "022100FF478110D1D4294471EC76E0157540C2181F47DEBD25D7F9E7DDCCCD47EE"
            "E905"
            "0220078F07CDAE6C240855D084AD91D1479609533C147C93B0AEF19BC9724D003F"
            "28"));
        CHECK(check(
            ECDSACanonicality::fullyCanonical,
            "3045"
            "0221009218248292F1762D8A51BE80F8A7F2CD288D810CE781D5955700DA1684DF"
            "1D2D"
            "022041A1EE1746BFD72C9760CC93A7AAA8047D52C8833A03A20EAAE92EA19717B4"
            "54"));
        CHECK(check(
            ECDSACanonicality::fullyCanonical,
            "3044"
            "02206A9E43775F73B6D1EC420E4DDD222A80D4C6DF5D1BEECC431A91B63C928B75"
            "81"
            "022023E9CC2D61DDA6F73EAA6BCB12688BEB0F434769276B3127E4044ED895C9D9"
            "6B"));
    }

    SUBCASE("Canonical but not fully canonical")
    {
        CHECK(check(
            ECDSACanonicality::canonical,
            "3046"
            "022100F477B3FA6F31C7CB3A0D1AD94A231FDD24B8D78862EE334CEA7CD08F6CBC"
            "0A1B"
            "022100928E6BCF1ED2684679730C5414AEC48FD62282B090041C41453C1D064AF5"
            "97A1"));
        CHECK(check(
            ECDSACanonicality::canonical,
            "3045"
            "022063E7C7CA93CB2400E413A342C027D00665F8BAB9C22EF0A7B8AE3AAF092230"
            "B6"
            "0221008F2E8BB7D09521ABBC277717B14B93170AE6465C5A1B36561099319C4BEB"
            "254C"));
    }

    SUBCASE("Valid")
    {
        CHECK(check(
            ECDSACanonicality::fullyCanonical,
            "3006"
            "020101"
            "020102"));
        CHECK(check(
            ECDSACanonicality::fullyCanonical,
            "3044"
            "02203932c892e2e550f3af8ee4ce9c215a87f9bb831dcac87b2838e2c2eaa891df"
            "0c"
            "022030b61dd36543125d56b9f9f3a1f53189e5af33cdda8d77a5209aec03978fa0"
            "01"));
    }

    SUBCASE("Invalid")
    {
        CHECK(check(
            std::nullopt,
            "3005"
            "0201FF"
            "0200"));
        CHECK(check(
            std::nullopt,
            "3006"
            "020101"
            "020202"));
        CHECK(check(
            std::nullopt,
            "3006"
            "020701"
            "020102"));
    }
}

TEST_CASE("Base58: secp256k1")
{
    auto const pk1 = derivePublicKey(
        KeyType::secp256k1,
        generateSecretKey(
            KeyType::secp256k1, generateSeed("masterpassphrase")));

    auto const pk2 = parseBase58<PublicKey>(
        TokenType::NodePublic,
        "n94a1u4jAz288pZLtw6yFWVbi89YamiC6JBXPVUj5zmExe5fTVg9");
    CHECK(pk2);
    CHECK_EQ(pk1, *pk2);

    // Try converting short, long and malformed data
    CHECK_FALSE(parseBase58<PublicKey>(TokenType::NodePublic, ""));
    CHECK_FALSE(parseBase58<PublicKey>(TokenType::NodePublic, " "));
    CHECK_FALSE(parseBase58<PublicKey>(TokenType::NodePublic, "!ty89234gh45"));
}

TEST_CASE("Base58: ed25519")
{
    auto const pk1 = derivePublicKey(
        KeyType::ed25519,
        generateSecretKey(KeyType::ed25519, generateSeed("masterpassphrase")));

    auto const pk2 = parseBase58<PublicKey>(
        TokenType::NodePublic,
        "nHUeeJCSY2dM71oxM8Cgjouf5ekTuev2mwDpc374aLMxzDLXNmjf");
    CHECK(pk2);
    CHECK_EQ(pk1, *pk2);
}

TEST_CASE("Miscellaneous operations")
{
    auto const pk1 = derivePublicKey(
        KeyType::secp256k1,
        generateSecretKey(
            KeyType::secp256k1, generateSeed("masterpassphrase")));

    PublicKey pk2(pk1);
    CHECK_EQ(pk1, pk2);
    CHECK_EQ(pk2, pk1);

    PublicKey pk3 = derivePublicKey(
        KeyType::secp256k1,
        generateSecretKey(
            KeyType::secp256k1, generateSeed("arbitraryPassPhrase")));
    // Testing the copy assignment operation of PublicKey class
    pk3 = pk2;
    CHECK_EQ(pk3, pk2);
    CHECK_EQ(pk1, pk3);
}

TEST_SUITE_END();

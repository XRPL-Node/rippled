#include <xrpl/basics/random.h>
#include <xrpl/beast/utility/rngfill.h>
#include <xrpl/protocol/PublicKey.h>
#include <xrpl/protocol/SecretKey.h>
#include <xrpl/protocol/Seed.h>

#include <doctest/doctest.h>

#include <algorithm>

using namespace xrpl;

TEST_SUITE_BEGIN("Seed");

namespace {

bool
equal(Seed const& lhs, Seed const& rhs)
{
    return std::equal(
        lhs.data(),
        lhs.data() + lhs.size(),
        rhs.data(),
        rhs.data() + rhs.size());
}

std::string
testPassphrase(std::string passphrase)
{
    auto const seed1 = generateSeed(passphrase);
    auto const seed2 = parseBase58<Seed>(toBase58(seed1));

    CHECK(static_cast<bool>(seed2));
    CHECK(equal(seed1, *seed2));
    return toBase58(seed1);
}

}  // namespace

TEST_CASE("construction")
{
    SUBCASE("from raw bytes")
    {
        std::uint8_t src[16];

        for (std::uint8_t i = 0; i < 64; i++)
        {
            beast::rngfill(src, sizeof(src), default_prng());
            Seed const seed({src, sizeof(src)});
            CHECK_EQ(memcmp(seed.data(), src, sizeof(src)), 0);
        }
    }

    SUBCASE("from uint128")
    {
        for (int i = 0; i < 64; i++)
        {
            uint128 src;
            beast::rngfill(src.data(), src.size(), default_prng());
            Seed const seed(src);
            CHECK_EQ(memcmp(seed.data(), src.data(), src.size()), 0);
        }
    }
}

TEST_CASE("generation from passphrase")
{
    CHECK(
        testPassphrase("masterpassphrase") == "snoPBrXtMeMyMHUVTgbuqAfg1SUTb");
    CHECK(
        testPassphrase("Non-Random Passphrase") ==
        "snMKnVku798EnBwUfxeSD8953sLYA");
    CHECK(
        testPassphrase("cookies excitement hand public") ==
        "sspUXGrmjQhq6mgc24jiRuevZiwKT");
}

TEST_CASE("base58 operations")
{
    SUBCASE("success")
    {
        CHECK(parseBase58<Seed>("snoPBrXtMeMyMHUVTgbuqAfg1SUTb"));
        CHECK(parseBase58<Seed>("snMKnVku798EnBwUfxeSD8953sLYA"));
        CHECK(parseBase58<Seed>("sspUXGrmjQhq6mgc24jiRuevZiwKT"));
    }

    SUBCASE("failure")
    {
        CHECK_FALSE(parseBase58<Seed>(""));
        CHECK_FALSE(parseBase58<Seed>("sspUXGrmjQhq6mgc24jiRuevZiwK"));
        CHECK_FALSE(parseBase58<Seed>("sspUXGrmjQhq6mgc24jiRuevZiwKTT"));
        CHECK_FALSE(parseBase58<Seed>("sspOXGrmjQhq6mgc24jiRuevZiwKT"));
        CHECK_FALSE(parseBase58<Seed>("ssp/XGrmjQhq6mgc24jiRuevZiwKT"));
    }
}

TEST_CASE("random generation")
{
    for (int i = 0; i < 32; i++)
    {
        auto const seed1 = randomSeed();
        auto const seed2 = parseBase58<Seed>(toBase58(seed1));

        CHECK(static_cast<bool>(seed2));
        CHECK(equal(seed1, *seed2));
    }
}

TEST_CASE("Node keypair generation & signing (secp256k1)")
{
    std::string const message1 = "http://www.ripple.com";
    std::string const message2 = "https://www.ripple.com";

    auto const secretKey =
        generateSecretKey(KeyType::secp256k1, generateSeed("masterpassphrase"));
    auto const publicKey = derivePublicKey(KeyType::secp256k1, secretKey);

    CHECK(
        toBase58(TokenType::NodePublic, publicKey) ==
        "n94a1u4jAz288pZLtw6yFWVbi89YamiC6JBXPVUj5zmExe5fTVg9");
    CHECK(
        toBase58(TokenType::NodePrivate, secretKey) ==
        "pnen77YEeUd4fFKG7iycBWcwKpTaeFRkW2WFostaATy1DSupwXe");
    CHECK(
        to_string(calcNodeID(publicKey)) ==
        "7E59C17D50F5959C7B158FEC95C8F815BF653DC8");

    auto sig = sign(publicKey, secretKey, makeSlice(message1));
    CHECK_NE(sig.size(), 0);
    CHECK(verify(publicKey, makeSlice(message1), sig));

    // Correct public key but wrong message
    CHECK_FALSE(verify(publicKey, makeSlice(message2), sig));

    // Verify with incorrect public key
    {
        auto const otherPublicKey = derivePublicKey(
            KeyType::secp256k1,
            generateSecretKey(
                KeyType::secp256k1, generateSeed("otherpassphrase")));

        CHECK_FALSE(verify(otherPublicKey, makeSlice(message1), sig));
    }

    // Correct public key but wrong signature
    {
        // Slightly change the signature:
        if (auto ptr = sig.data())
            ptr[sig.size() / 2]++;

        CHECK_FALSE(verify(publicKey, makeSlice(message1), sig));
    }
}

TEST_CASE("Node keypair generation & signing (ed25519)")
{
    std::string const message1 = "http://www.ripple.com";
    std::string const message2 = "https://www.ripple.com";

    auto const secretKey =
        generateSecretKey(KeyType::ed25519, generateSeed("masterpassphrase"));
    auto const publicKey = derivePublicKey(KeyType::ed25519, secretKey);

    CHECK(
        toBase58(TokenType::NodePublic, publicKey) ==
        "nHUeeJCSY2dM71oxM8Cgjouf5ekTuev2mwDpc374aLMxzDLXNmjf");
    CHECK(
        toBase58(TokenType::NodePrivate, secretKey) ==
        "paKv46LztLqK3GaKz1rG2nQGN6M4JLyRtxFBYFTw4wAVHtGys36");
    CHECK(
        to_string(calcNodeID(publicKey)) ==
        "AA066C988C712815CC37AF71472B7CBBBD4E2A0A");

    auto sig = sign(publicKey, secretKey, makeSlice(message1));
    CHECK_NE(sig.size(), 0);
    CHECK_UNARY(verify(publicKey, makeSlice(message1), sig));

    // Correct public key but wrong message
    CHECK_FALSE(verify(publicKey, makeSlice(message2), sig));

    // Verify with incorrect public key
    {
        auto const otherPublicKey = derivePublicKey(
            KeyType::ed25519,
            generateSecretKey(
                KeyType::ed25519, generateSeed("otherpassphrase")));

        CHECK_FALSE(verify(otherPublicKey, makeSlice(message1), sig));
    }

    // Correct public key but wrong signature
    {
        if (auto ptr = sig.data())
            ptr[sig.size() / 2]++;

        CHECK_FALSE(verify(publicKey, makeSlice(message1), sig));
    }
}

TEST_CASE("Account keypair generation & signing (secp256k1)")
{
    std::string const message1 = "http://www.ripple.com";
    std::string const message2 = "https://www.ripple.com";

    auto const [pk, sk] =
        generateKeyPair(KeyType::secp256k1, generateSeed("masterpassphrase"));

    CHECK_EQ(toBase58(calcAccountID(pk)), "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh");
    CHECK(
        toBase58(TokenType::AccountPublic, pk) ==
        "aBQG8RQAzjs1eTKFEAQXr2gS4utcDiEC9wmi7pfUPTi27VCahwgw");
    CHECK(
        toBase58(TokenType::AccountSecret, sk) ==
        "p9JfM6HHi64m6mvB6v5k7G2b1cXzGmYiCNJf6GHPKvFTWdeRVjh");

    auto sig = sign(pk, sk, makeSlice(message1));
    CHECK_NE(sig.size(), 0);
    CHECK_UNARY(verify(pk, makeSlice(message1), sig));

    // Correct public key but wrong message
    CHECK_FALSE(verify(pk, makeSlice(message2), sig));

    // Verify with incorrect public key
    {
        auto const otherKeyPair = generateKeyPair(
            KeyType::secp256k1, generateSeed("otherpassphrase"));

        CHECK_FALSE(verify(otherKeyPair.first, makeSlice(message1), sig));
    }

    // Correct public key but wrong signature
    {
        if (auto ptr = sig.data())
            ptr[sig.size() / 2]++;

        CHECK_FALSE(verify(pk, makeSlice(message1), sig));
    }
}

TEST_CASE("Account keypair generation & signing (ed25519)")
{
    std::string const message1 = "http://www.ripple.com";
    std::string const message2 = "https://www.ripple.com";

    auto const [pk, sk] =
        generateKeyPair(KeyType::ed25519, generateSeed("masterpassphrase"));

    CHECK_EQ(
        to_string(calcAccountID(pk)), "rGWrZyQqhTp9Xu7G5Pkayo7bXjH4k4QYpf");
    CHECK(
        toBase58(TokenType::AccountPublic, pk) ==
        "aKGheSBjmCsKJVuLNKRAKpZXT6wpk2FCuEZAXJupXgdAxX5THCqR");
    CHECK(
        toBase58(TokenType::AccountSecret, sk) ==
        "pwDQjwEhbUBmPuEjFpEG75bFhv2obkCB7NxQsfFxM7xGHBMVPu9");

    auto sig = sign(pk, sk, makeSlice(message1));
    CHECK_NE(sig.size(), 0);
    CHECK_UNARY(verify(pk, makeSlice(message1), sig));

    // Correct public key but wrong message
    CHECK_FALSE(verify(pk, makeSlice(message2), sig));

    // Verify with incorrect public key
    {
        auto const otherKeyPair =
            generateKeyPair(KeyType::ed25519, generateSeed("otherpassphrase"));

        CHECK_FALSE(verify(otherKeyPair.first, makeSlice(message1), sig));
    }

    // Correct public key but wrong signature
    {
        if (auto ptr = sig.data())
            ptr[sig.size() / 2]++;

        CHECK_FALSE(verify(pk, makeSlice(message1), sig));
    }
}

TEST_CASE("Parsing")
{
    // account IDs and node and account public and private
    // keys should not be parseable as seeds.

    auto const node1 = randomKeyPair(KeyType::secp256k1);

    CHECK_FALSE(parseGenericSeed(toBase58(TokenType::NodePublic, node1.first)));
    CHECK_FALSE(
        parseGenericSeed(toBase58(TokenType::NodePrivate, node1.second)));

    auto const node2 = randomKeyPair(KeyType::ed25519);

    CHECK_FALSE(parseGenericSeed(toBase58(TokenType::NodePublic, node2.first)));
    CHECK_FALSE(
        parseGenericSeed(toBase58(TokenType::NodePrivate, node2.second)));

    auto const account1 = generateKeyPair(KeyType::secp256k1, randomSeed());

    CHECK_FALSE(parseGenericSeed(toBase58(calcAccountID(account1.first))));
    CHECK_FALSE(
        parseGenericSeed(toBase58(TokenType::AccountPublic, account1.first)));
    CHECK_FALSE(
        parseGenericSeed(toBase58(TokenType::AccountSecret, account1.second)));

    auto const account2 = generateKeyPair(KeyType::ed25519, randomSeed());

    CHECK_FALSE(parseGenericSeed(toBase58(calcAccountID(account2.first))));
    CHECK_FALSE(
        parseGenericSeed(toBase58(TokenType::AccountPublic, account2.first)));
    CHECK_FALSE(
        parseGenericSeed(toBase58(TokenType::AccountSecret, account2.second)));
}

TEST_SUITE_END();

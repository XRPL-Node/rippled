#include <xrpl/beast/utility/rngfill.h>
#include <xrpl/crypto/csprng.h>
#include <xrpl/protocol/PublicKey.h>
#include <xrpl/protocol/SecretKey.h>
#include <xrpl/protocol/Seed.h>

#include <doctest/doctest.h>

#include <algorithm>
#include <string>
#include <vector>

namespace xrpl {

struct TestKeyData
{
    std::array<std::uint8_t, 16> seed;
    std::array<std::uint8_t, 33> pubkey;
    std::array<std::uint8_t, 32> seckey;
    char const* addr;
};

using blob = std::vector<std::uint8_t>;

TEST_SUITE_BEGIN("SecretKey");

TEST_CASE("secp256k1: canonicality")
{
    std::array<std::uint8_t, 32> const digestData{
        0x34, 0xC1, 0x90, 0x28, 0xC8, 0x0D, 0x21, 0xF3, 0xF4, 0x8C, 0x93,
        0x54, 0x89, 0x5F, 0x8D, 0x5B, 0xF0, 0xD5, 0xEE, 0x7F, 0xF4, 0x57,
        0x64, 0x7C, 0xF6, 0x55, 0xF5, 0x53, 0x0A, 0x30, 0x22, 0xA7};

    std::array<std::uint8_t, 33> const pkData{
        0x02, 0x50, 0x96, 0xEB, 0x12, 0xD3, 0xE9, 0x24, 0x23, 0x4E, 0x71,
        0x62, 0x36, 0x9C, 0x11, 0xD8, 0xBF, 0x87, 0x7E, 0xDA, 0x23, 0x87,
        0x78, 0xE7, 0xA3, 0x1F, 0xF0, 0xAA, 0xC5, 0xD0, 0xDB, 0xCF, 0x37};

    std::array<std::uint8_t, 32> const skData{
        0xAA, 0x92, 0x14, 0x17, 0xE7, 0xE5, 0xC2, 0x99, 0xDA, 0x4E, 0xEC,
        0x16, 0xD1, 0xCA, 0xA9, 0x2F, 0x19, 0xB1, 0x9F, 0x2A, 0x68, 0x51,
        0x1F, 0x68, 0xEC, 0x73, 0xBB, 0xB2, 0xF5, 0x23, 0x6F, 0x3D};

    std::array<std::uint8_t, 71> const sig{
        0x30, 0x45, 0x02, 0x21, 0x00, 0xB4, 0x9D, 0x07, 0xF0, 0xE9, 0x34, 0xBA,
        0x46, 0x8C, 0x0E, 0xFC, 0x78, 0x11, 0x77, 0x91, 0x40, 0x8D, 0x1F, 0xB8,
        0xB6, 0x3A, 0x64, 0x92, 0xAD, 0x39, 0x5A, 0xC2, 0xF3, 0x60, 0xF2, 0x46,
        0x60, 0x02, 0x20, 0x50, 0x87, 0x39, 0xDB, 0x0A, 0x2E, 0xF8, 0x16, 0x76,
        0xE3, 0x9F, 0x45, 0x9C, 0x8B, 0xBB, 0x07, 0xA0, 0x9C, 0x3E, 0x9F, 0x9B,
        0xEB, 0x69, 0x62, 0x94, 0xD5, 0x24, 0xD4, 0x79, 0xD6, 0x27, 0x40};

    std::array<std::uint8_t, 72> const non{
        0x30, 0x46, 0x02, 0x21, 0x00, 0xB4, 0x9D, 0x07, 0xF0, 0xE9, 0x34, 0xBA,
        0x46, 0x8C, 0x0E, 0xFC, 0x78, 0x11, 0x77, 0x91, 0x40, 0x8D, 0x1F, 0xB8,
        0xB6, 0x3A, 0x64, 0x92, 0xAD, 0x39, 0x5A, 0xC2, 0xF3, 0x60, 0xF2, 0x46,
        0x60, 0x02, 0x21, 0x00, 0xAF, 0x78, 0xC6, 0x24, 0xF5, 0xD1, 0x07, 0xE9,
        0x89, 0x1C, 0x60, 0xBA, 0x63, 0x74, 0x44, 0xF7, 0x1A, 0x12, 0x9E, 0x47,
        0x13, 0x5D, 0x36, 0xD9, 0x2A, 0xFD, 0x39, 0xB8, 0x56, 0x60, 0x1A, 0x01};

    auto const digest = uint256::fromVoid(digestData.data());

    PublicKey const pk{makeSlice(pkData)};
    SecretKey const sk{makeSlice(skData)};

    {
        auto const canonicality = ecdsaCanonicality(makeSlice(sig));
        CHECK(canonicality);
        CHECK(*canonicality == ECDSACanonicality::fullyCanonical);
    }

    {
        auto const canonicality = ecdsaCanonicality(makeSlice(non));
        CHECK(canonicality);
        CHECK(*canonicality != ECDSACanonicality::fullyCanonical);
    }

    CHECK(verifyDigest(pk, digest, makeSlice(sig), false));
    CHECK(verifyDigest(pk, digest, makeSlice(sig), true));
    CHECK(verifyDigest(pk, digest, makeSlice(non), false));
    CHECK(!verifyDigest(pk, digest, makeSlice(non), true));
}

TEST_CASE("secp256k1: digest signing & verification")
{
    for (std::size_t i = 0; i < 32; i++)
    {
        auto const [pk, sk] = randomKeyPair(KeyType::secp256k1);

        CHECK(pk == derivePublicKey(KeyType::secp256k1, sk));
        CHECK(*publicKeyType(pk) == KeyType::secp256k1);

        for (std::size_t j = 0; j < 32; j++)
        {
            uint256 digest;
            beast::rngfill(digest.data(), digest.size(), crypto_prng());

            auto sig = signDigest(pk, sk, digest);

            CHECK(sig.size() != 0);
            CHECK(verifyDigest(pk, digest, sig, true));

            // Wrong digest:
            CHECK(!verifyDigest(pk, ~digest, sig, true));

            // Slightly change the signature:
            if (auto ptr = sig.data())
                ptr[j % sig.size()]++;

            // Wrong signature:
            CHECK(!verifyDigest(pk, digest, sig, true));

            // Wrong digest and signature:
            CHECK(!verifyDigest(pk, ~digest, sig, true));
        }
    }
}

void
testSigning(KeyType type)
{
    for (std::size_t i = 0; i < 32; i++)
    {
        auto const [pk, sk] = randomKeyPair(type);

        CHECK(pk == derivePublicKey(type, sk));
        CHECK(*publicKeyType(pk) == type);

        for (std::size_t j = 0; j < 32; j++)
        {
            std::vector<std::uint8_t> data(64 + (8 * i) + j);
            beast::rngfill(data.data(), data.size(), crypto_prng());

            auto sig = sign(pk, sk, makeSlice(data));

            CHECK(sig.size() != 0);
            CHECK(verify(pk, makeSlice(data), sig));

            // Construct wrong data:
            auto badData = data;

            // swaps the smallest and largest elements in buffer
            std::iter_swap(
                std::min_element(badData.begin(), badData.end()),
                std::max_element(badData.begin(), badData.end()));

            // Wrong data: should fail
            CHECK(!verify(pk, makeSlice(badData), sig));

            // Slightly change the signature:
            if (auto ptr = sig.data())
                ptr[j % sig.size()]++;

            // Wrong signature: should fail
            CHECK(!verify(pk, makeSlice(data), sig));

            // Wrong data and signature: should fail
            CHECK(!verify(pk, makeSlice(badData), sig));
        }
    }
}

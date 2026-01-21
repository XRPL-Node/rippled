#include <xrpl/basics/Blob.h>
#include <xrpl/basics/base_uint.h>
#include <xrpl/basics/hardened_hash.h>

#include <boost/endian/conversion.hpp>

#include <doctest/doctest.h>

#include <complex>
#include <type_traits>
#include <unordered_set>

using namespace xrpl;

// a non-hashing Hasher that just copies the bytes.
// Used to test hash_append in base_uint
template <std::size_t Bits>
struct nonhash
{
    static constexpr auto const endian = boost::endian::order::big;
    static constexpr std::size_t WIDTH = Bits / 8;

    std::array<std::uint8_t, WIDTH> data_;

    nonhash() = default;

    void
    operator()(void const* key, std::size_t len) noexcept
    {
        assert(len == WIDTH);
        memcpy(data_.data(), key, len);
    }

    explicit
    operator std::size_t() noexcept
    {
        return WIDTH;
    }
};

using test96 = base_uint<96>;
static_assert(std::is_copy_constructible<test96>::value);
static_assert(std::is_copy_assignable<test96>::value);

TEST_SUITE_BEGIN("base_uint");

TEST_CASE("comparisons 64-bit")
{
    static constexpr std::
        array<std::pair<std::string_view, std::string_view>, 6>
            test_args{
                {{"0000000000000000", "0000000000000001"},
                 {"0000000000000000", "ffffffffffffffff"},
                 {"1234567812345678", "2345678923456789"},
                 {"8000000000000000", "8000000000000001"},
                 {"aaaaaaaaaaaaaaa9", "aaaaaaaaaaaaaaaa"},
                 {"fffffffffffffffe", "ffffffffffffffff"}}};

    for (auto const& arg : test_args)
    {
        xrpl::base_uint<64> const u{arg.first}, v{arg.second};
        CHECK_LT(u, v);
        CHECK_LE(u, v);
        CHECK_NE(u, v);
        CHECK_FALSE(u == v);
        CHECK_FALSE(u > v);
        CHECK_FALSE(u >= v);
        CHECK_FALSE(v < u);
        CHECK_FALSE(v <= u);
        CHECK_NE(v, u);
        CHECK_FALSE(v == u);
        CHECK_GT(v, u);
        CHECK_GE(v, u);
        CHECK_EQ(u, u);
        CHECK_EQ(v, v);
    }
}

TEST_CASE("comparisons 96-bit")
{
    static constexpr std::
        array<std::pair<std::string_view, std::string_view>, 6>
            test_args{{
                {"000000000000000000000000", "000000000000000000000001"},
                {"000000000000000000000000", "ffffffffffffffffffffffff"},
                {"0123456789ab0123456789ab", "123456789abc123456789abc"},
                {"555555555555555555555555", "55555555555a555555555555"},
                {"aaaaaaaaaaaaaaa9aaaaaaaa", "aaaaaaaaaaaaaaaaaaaaaaaa"},
                {"fffffffffffffffffffffffe", "ffffffffffffffffffffffff"},
            }};

    for (auto const& arg : test_args)
    {
        xrpl::base_uint<96> const u{arg.first}, v{arg.second};
        CHECK_LT(u, v);
        CHECK_LE(u, v);
        CHECK_NE(u, v);
        CHECK_FALSE(u == v);
        CHECK_FALSE(u > v);
        CHECK_FALSE(u >= v);
        CHECK_FALSE(v < u);
        CHECK_FALSE(v <= u);
        CHECK_NE(v, u);
        CHECK_FALSE(v == u);
        CHECK_GT(v, u);
        CHECK_GE(v, u);
        CHECK_EQ(u, u);
        CHECK_EQ(v, v);
    }
}

TEST_CASE("general purpose tests")
{
    static_assert(!std::is_constructible<test96, std::complex<double>>::value);
    static_assert(!std::is_assignable<test96&, std::complex<double>>::value);

    // used to verify set insertion (hashing required)
    std::unordered_set<test96, hardened_hash<>> uset;

    Blob raw{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    CHECK_EQ(test96::bytes, raw.size());

    test96 u{raw};
    uset.insert(u);
    CHECK_EQ(raw.size(), u.size());
    CHECK_EQ(to_string(u), "0102030405060708090A0B0C");
    CHECK_EQ(to_short_string(u), "01020304...");
    CHECK_EQ(*u.data(), 1);
    CHECK_EQ(u.signum(), 1);
    CHECK_UNARY(!!u);
    CHECK_FALSE(u.isZero());
    CHECK_UNARY(u.isNonZero());
    unsigned char t = 0;
    for (auto& d : u)
    {
        CHECK_EQ(d, ++t);
    }

    // Test hash_append by "hashing" with a no-op hasher (h)
    // and then extracting the bytes that were written during hashing
    // back into another base_uint (w) for comparison with the original
    nonhash<96> h;
    hash_append(h, u);
    test96 w{std::vector<std::uint8_t>(h.data_.begin(), h.data_.end())};
    CHECK_EQ(w, u);

    test96 v{~u};
    uset.insert(v);
    CHECK_EQ(to_string(v), "FEFDFCFBFAF9F8F7F6F5F4F3");
    CHECK_EQ(to_short_string(v), "FEFDFCFB...");
    CHECK_EQ(*v.data(), 0xfe);
    CHECK_EQ(v.signum(), 1);
    CHECK_UNARY(!!v);
    CHECK_FALSE(v.isZero());
    CHECK_UNARY(v.isNonZero());
    t = 0xff;
    for (auto& d : v)
    {
        CHECK_EQ(d, --t);
    }

    CHECK_LT(u, v);
    CHECK_GT(v, u);

    v = u;
    CHECK_EQ(v, u);

    test96 z{beast::zero};
    uset.insert(z);
    CHECK_EQ(to_string(z), "000000000000000000000000");
    CHECK_EQ(to_short_string(z), "00000000...");
    CHECK_EQ(*z.data(), 0);
    CHECK_EQ(*z.begin(), 0);
    CHECK_EQ(*std::prev(z.end(), 1), 0);
    CHECK_EQ(z.signum(), 0);
    CHECK_UNARY(!z);  // base_uint doesn't have explicit bool conversion
    CHECK_UNARY(z.isZero());
    CHECK_UNARY(!z.isNonZero());
    for (auto& d : z)
    {
        CHECK_EQ(d, 0);
    }

    test96 n{z};
    n++;
    CHECK_EQ(n, test96(1));
    n--;
    CHECK_EQ(n, beast::zero);
    CHECK_EQ(n, z);
    n--;
    CHECK_EQ(to_string(n), "FFFFFFFFFFFFFFFFFFFFFFFF");
    CHECK_EQ(to_short_string(n), "FFFFFFFF...");
    n = beast::zero;
    CHECK_EQ(n, z);

    test96 zp1{z};
    zp1++;
    test96 zm1{z};
    zm1--;
    test96 x{zm1 ^ zp1};
    uset.insert(x);
    CHECK_EQ(to_string(x), "FFFFFFFFFFFFFFFFFFFFFFFE");
    CHECK_EQ(to_short_string(x), "FFFFFFFF...");

    CHECK_EQ(uset.size(), 4);

    test96 tmp;
    CHECK_UNARY(tmp.parseHex(to_string(u)));
    CHECK_EQ(tmp, u);
    tmp = z;

    // fails with extra char
    CHECK_FALSE(tmp.parseHex("A" + to_string(u)));
    tmp = z;

    // fails with extra char at end
    CHECK_FALSE(tmp.parseHex(to_string(u) + "A"));

    // fails with a non-hex character at some point in the string:
    tmp = z;

    for (std::size_t i = 0; i != 24; ++i)
    {
        std::string x = to_string(z);
        x[i] = ('G' + (i % 10));
        CHECK_FALSE(tmp.parseHex(x));
    }

    // Walking 1s:
    for (std::size_t i = 0; i != 24; ++i)
    {
        std::string s1 = "000000000000000000000000";
        s1[i] = '1';

        CHECK_UNARY(tmp.parseHex(s1));
        CHECK_EQ(to_string(tmp), s1);
    }

    // Walking 0s:
    for (std::size_t i = 0; i != 24; ++i)
    {
        std::string s1 = "111111111111111111111111";
        s1[i] = '0';

        CHECK_UNARY(tmp.parseHex(s1));
        CHECK_EQ(to_string(tmp), s1);
    }
}

TEST_CASE("constexpr constructors")
{
    static_assert(test96{}.signum() == 0);
    static_assert(test96("0").signum() == 0);
    static_assert(test96("000000000000000000000000").signum() == 0);
    static_assert(test96("000000000000000000000001").signum() == 1);
    static_assert(test96("800000000000000000000000").signum() == 1);

    // Using the constexpr constructor in a non-constexpr context
    // with an error in the parsing throws an exception.
    {
        // Invalid length for string.
        bool caught = false;
        try
        {
            // Try to prevent constant evaluation.
            std::vector<char> str(23, '7');
            std::string_view sView(str.data(), str.size());
            [[maybe_unused]] test96 t96(sView);
        }
        catch (std::invalid_argument const& e)
        {
            CHECK_EQ(e.what(), std::string("invalid length for hex string"));
            caught = true;
        }
        CHECK_UNARY(caught);
    }
    {
        // Invalid character in string.
        bool caught = false;
        try
        {
            // Try to prevent constant evaluation.
            std::vector<char> str(23, '7');
            str.push_back('G');
            std::string_view sView(str.data(), str.size());
            [[maybe_unused]] test96 t96(sView);
        }
        catch (std::range_error const& e)
        {
            CHECK_EQ(e.what(), std::string("invalid hex character"));
            caught = true;
        }
        CHECK_UNARY(caught);
    }

    // Verify that constexpr base_uints interpret a string the same
    // way parseHex() does.
    struct StrBaseUint
    {
        char const* const str;
        test96 tst;

        constexpr StrBaseUint(char const* s) : str(s), tst(s)
        {
        }
    };
    constexpr StrBaseUint testCases[] = {
        "000000000000000000000000",
        "000000000000000000000001",
        "fedcba9876543210ABCDEF91",
        "19FEDCBA0123456789abcdef",
        "800000000000000000000000",
        "fFfFfFfFfFfFfFfFfFfFfFfF"};

    for (StrBaseUint const& t : testCases)
    {
        test96 t96;
        CHECK_UNARY(t96.parseHex(t.str));
        CHECK_EQ(t96, t.tst);
    }
}

TEST_SUITE_END();

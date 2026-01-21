// IPEndpoint doctest - converted from src/test/beast/IPEndpoint_test.cpp

#include <xrpl/basics/random.h>
#include <xrpl/beast/net/IPEndpoint.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/predef.h>

#include <doctest/doctest.h>

#include <sstream>
#include <unordered_set>

TEST_SUITE_BEGIN("IPEndpoint");

namespace {

using namespace beast::IP;

Endpoint
randomEP(bool v4 = true)
{
    using namespace xrpl;
    auto dv4 = []() -> AddressV4::bytes_type {
        return {
            {static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX))}};
    };
    auto dv6 = []() -> AddressV6::bytes_type {
        return {
            {static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX)),
             static_cast<std::uint8_t>(rand_int<int>(1, UINT8_MAX))}};
    };
    return Endpoint{
        v4 ? Address{AddressV4{dv4()}} : Address{AddressV6{dv6()}},
        rand_int<std::uint16_t>(1, UINT16_MAX)};
}

void
shouldParseAddrV4(
    std::string const& s,
    std::uint32_t value,
    std::string const& normal = "")
{
    boost::system::error_code ec;
    Address const result{boost::asio::ip::make_address(s, ec)};
    REQUIRE_MESSAGE(!ec, ec.message());
    REQUIRE_MESSAGE(result.is_v4(), s.c_str());
    REQUIRE_MESSAGE(result.to_v4().to_uint() == value, s.c_str());
    CHECK_MESSAGE(
        result.to_string() == (normal.empty() ? s : normal), s.c_str());
}

void
failParseAddr(std::string const& s)
{
    boost::system::error_code ec;
    auto a = boost::asio::ip::make_address(s, ec);
    CHECK_MESSAGE(ec, s.c_str());
}

void
shouldParseEPV4(
    std::string const& s,
    AddressV4::bytes_type const& value,
    std::uint16_t p,
    std::string const& normal = "")
{
    auto const result = Endpoint::from_string_checked(s);
    REQUIRE(result);
    REQUIRE(result->address().is_v4());
    REQUIRE_EQ(result->address().to_v4(), AddressV4{value});
    CHECK_EQ(result->port(), p);
    CHECK_EQ(to_string(*result), (normal.empty() ? s : normal));
}

void
shouldParseEPV6(
    std::string const& s,
    AddressV6::bytes_type const& value,
    std::uint16_t p,
    std::string const& normal = "")
{
    auto result = Endpoint::from_string_checked(s);
    REQUIRE(result);
    REQUIRE(result->address().is_v6());
    REQUIRE_EQ(result->address().to_v6(), AddressV6{value});
    CHECK_EQ(result->port(), p);
    CHECK_EQ(to_string(*result), (normal.empty() ? s : normal));
}

void
failParseEP(std::string s)
{
    auto a1 = Endpoint::from_string(s);
    CHECK_MESSAGE(is_unspecified(a1), s.c_str());

    auto a2 = Endpoint::from_string(s);
    CHECK_MESSAGE(is_unspecified(a2), s.c_str());

    boost::replace_last(s, ":", " ");
    auto a3 = Endpoint::from_string(s);
    CHECK_MESSAGE(is_unspecified(a3), s.c_str());
}

template <typename T>
bool
parse(std::string const& text, T& t)
{
    std::istringstream stream{text};
    stream >> t;
    return !stream.fail();
}

template <typename T>
void
shouldPass(std::string const& text, std::string const& normal = "")
{
    T t;
    CHECK(parse(text, t));
    CHECK_MESSAGE(
        to_string(t) == (normal.empty() ? text : normal), text.c_str());
}

template <typename T>
void
shouldFail(std::string const& text)
{
    T t;
    CHECK_FALSE_MESSAGE(parse(text, t), text.c_str());
}

}  // namespace

TEST_CASE("AddressV4")
{
    CHECK_EQ(AddressV4{}.to_uint(), 0);
    CHECK_UNARY(is_unspecified(AddressV4{}));
    CHECK_EQ(AddressV4{0x01020304}.to_uint(), 0x01020304);

    {
        AddressV4::bytes_type d = {{1, 2, 3, 4}};
        CHECK_EQ(AddressV4{d}.to_uint(), 0x01020304);
        CHECK_FALSE(is_unspecified(AddressV4{d}));
    }

    AddressV4 const v1{1};
    CHECK_EQ(AddressV4{v1}.to_uint(), 1);

    {
        AddressV4 v;
        v = v1;
        CHECK_EQ(v.to_uint(), v1.to_uint());
    }

    {
        AddressV4 v;
        auto d = v.to_bytes();
        d[0] = 1;
        d[1] = 2;
        d[2] = 3;
        d[3] = 4;
        v = AddressV4{d};
        CHECK_EQ(v.to_uint(), 0x01020304);
    }

    CHECK_EQ(AddressV4(0x01020304).to_string(), "1.2.3.4");

    shouldParseAddrV4("1.2.3.4", 0x01020304);
    shouldParseAddrV4("255.255.255.255", 0xffffffff);
    shouldParseAddrV4("0.0.0.0", 0);

    failParseAddr(".");
    failParseAddr("..");
    failParseAddr("...");
    failParseAddr("....");
#if BOOST_OS_WINDOWS
    // WINDOWS bug in asio - I don't think these should parse
    // at all, and in-fact they do not on mac/linux
    shouldParseAddrV4("1", 0x00000001, "0.0.0.1");
    shouldParseAddrV4("1.2", 0x01000002, "1.0.0.2");
    shouldParseAddrV4("1.2.3", 0x01020003, "1.2.0.3");
#else
    failParseAddr("1");
    failParseAddr("1.2");
    failParseAddr("1.2.3");
#endif
    failParseAddr("1.");
    failParseAddr("1.2.");
    failParseAddr("1.2.3.");
    failParseAddr("256.0.0.0");
    failParseAddr("-1.2.3.4");
}

TEST_CASE("AddressV4::Bytes")
{
    AddressV4::bytes_type d1 = {{10, 0, 0, 1}};
    AddressV4 v4{d1};
    CHECK_EQ(v4.to_bytes()[0], 10);
    CHECK_EQ(v4.to_bytes()[1], 0);
    CHECK_EQ(v4.to_bytes()[2], 0);
    CHECK_EQ(v4.to_bytes()[3], 1);

    CHECK_EQ((~((0xff) << 16)), 0xff00ffff);

    auto d2 = v4.to_bytes();
    d2[1] = 10;
    v4 = AddressV4{d2};
    CHECK_EQ(v4.to_bytes()[0], 10);
    CHECK_EQ(v4.to_bytes()[1], 10);
    CHECK_EQ(v4.to_bytes()[2], 0);
    CHECK_EQ(v4.to_bytes()[3], 1);
}

TEST_CASE("Address")
{
    boost::system::error_code ec;
    Address result{boost::asio::ip::make_address("1.2.3.4", ec)};
    AddressV4::bytes_type d = {{1, 2, 3, 4}};
    CHECK_FALSE(ec);
    CHECK_UNARY(result.is_v4());
    CHECK_EQ(result.to_v4(), AddressV4{d});
}

TEST_CASE("Endpoint")
{
    shouldParseEPV4("1.2.3.4", {{1, 2, 3, 4}}, 0);
    shouldParseEPV4("1.2.3.4:5", {{1, 2, 3, 4}}, 5);
    shouldParseEPV4("1.2.3.4 5", {{1, 2, 3, 4}}, 5, "1.2.3.4:5");
    // leading, trailing space
    shouldParseEPV4("   1.2.3.4:5", {{1, 2, 3, 4}}, 5, "1.2.3.4:5");
    shouldParseEPV4("1.2.3.4:5    ", {{1, 2, 3, 4}}, 5, "1.2.3.4:5");
    shouldParseEPV4("1.2.3.4   ", {{1, 2, 3, 4}}, 0, "1.2.3.4");
    shouldParseEPV4("  1.2.3.4", {{1, 2, 3, 4}}, 0, "1.2.3.4");
    shouldParseEPV6(
        "2001:db8:a0b:12f0::1",
        {{32, 01, 13, 184, 10, 11, 18, 240, 0, 0, 0, 0, 0, 0, 0, 1}},
        0);
    shouldParseEPV6(
        "[2001:db8:a0b:12f0::1]:8",
        {{32, 01, 13, 184, 10, 11, 18, 240, 0, 0, 0, 0, 0, 0, 0, 1}},
        8);
    shouldParseEPV6(
        "[2001:2002:2003:2004:2005:2006:2007:2008]:65535",
        {{32, 1, 32, 2, 32, 3, 32, 4, 32, 5, 32, 6, 32, 7, 32, 8}},
        65535);
    shouldParseEPV6(
        "2001:2002:2003:2004:2005:2006:2007:2008 65535",
        {{32, 1, 32, 2, 32, 3, 32, 4, 32, 5, 32, 6, 32, 7, 32, 8}},
        65535,
        "[2001:2002:2003:2004:2005:2006:2007:2008]:65535");

    Endpoint ep;

    AddressV4::bytes_type d = {{127, 0, 0, 1}};
    ep = Endpoint(AddressV4{d}, 80);
    CHECK_FALSE(is_unspecified(ep));
    CHECK_FALSE(is_public(ep));
    CHECK_UNARY(is_private(ep));
    CHECK_FALSE(is_multicast(ep));
    CHECK_UNARY(is_loopback(ep));
    CHECK_EQ(to_string(ep), "127.0.0.1:80");
    // same address as v4 mapped in ipv6
    ep = Endpoint(
        boost::asio::ip::make_address_v6(
            boost::asio::ip::v4_mapped, AddressV4{d}),
        80);
    CHECK_FALSE(is_unspecified(ep));
    CHECK_FALSE(is_public(ep));
    CHECK_UNARY(is_private(ep));
    CHECK_FALSE(is_multicast(ep));
    CHECK_FALSE(is_loopback(ep));  // mapped loopback is not a loopback
    CHECK_EQ(to_string(ep), "[::ffff:127.0.0.1]:80");

    d = {{10, 0, 0, 1}};
    ep = Endpoint(AddressV4{d});
    CHECK_EQ(get_class(ep.to_v4()), 'A');
    CHECK_FALSE(is_unspecified(ep));
    CHECK_FALSE(is_public(ep));
    CHECK_UNARY(is_private(ep));
    CHECK_FALSE(is_multicast(ep));
    CHECK_FALSE(is_loopback(ep));
    CHECK_EQ(to_string(ep), "10.0.0.1");
    // same address as v4 mapped in ipv6
    ep = Endpoint(boost::asio::ip::make_address_v6(
        boost::asio::ip::v4_mapped, AddressV4{d}));
    CHECK_EQ(
        get_class(boost::asio::ip::make_address_v4(
            boost::asio::ip::v4_mapped, ep.to_v6())),
        'A');
    CHECK_FALSE(is_unspecified(ep));
    CHECK_FALSE(is_public(ep));
    CHECK_UNARY(is_private(ep));
    CHECK_FALSE(is_multicast(ep));
    CHECK_FALSE(is_loopback(ep));
    CHECK_EQ(to_string(ep), "::ffff:10.0.0.1");

    d = {{166, 78, 151, 147}};
    ep = Endpoint(AddressV4{d});
    CHECK_FALSE(is_unspecified(ep));
    CHECK_UNARY(is_public(ep));
    CHECK_FALSE(is_private(ep));
    CHECK_FALSE(is_multicast(ep));
    CHECK_FALSE(is_loopback(ep));
    CHECK_EQ(to_string(ep), "166.78.151.147");
    // same address as v4 mapped in ipv6
    ep = Endpoint(boost::asio::ip::make_address_v6(
        boost::asio::ip::v4_mapped, AddressV4{d}));
    CHECK_FALSE(is_unspecified(ep));
    CHECK_UNARY(is_public(ep));
    CHECK_FALSE(is_private(ep));
    CHECK_FALSE(is_multicast(ep));
    CHECK_FALSE(is_loopback(ep));
    CHECK_EQ(to_string(ep), "::ffff:166.78.151.147");

    // a private IPv6
    AddressV6::bytes_type d2 = {
        {253, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}};
    ep = Endpoint(AddressV6{d2});
    CHECK_FALSE(is_unspecified(ep));
    CHECK_FALSE(is_public(ep));
    CHECK_UNARY(is_private(ep));
    CHECK_FALSE(is_multicast(ep));
    CHECK_FALSE(is_loopback(ep));
    CHECK_EQ(to_string(ep), "fd00::1");

    {
        ep = Endpoint::from_string("192.0.2.112");
        CHECK_FALSE(is_unspecified(ep));
        CHECK_EQ(ep, Endpoint::from_string("192.0.2.112"));

        auto const ep1 = Endpoint::from_string("192.0.2.112:2016");
        CHECK_FALSE(is_unspecified(ep1));
        CHECK_EQ(ep.address(), ep1.address());
        CHECK_EQ(ep1.port(), 2016);

        auto const ep2 = Endpoint::from_string("192.0.2.112:2016");
        CHECK_FALSE(is_unspecified(ep2));
        CHECK_EQ(ep.address(), ep2.address());
        CHECK_EQ(ep2.port(), 2016);
        CHECK_EQ(ep1, ep2);

        auto const ep3 = Endpoint::from_string("192.0.2.112 2016");
        CHECK_FALSE(is_unspecified(ep3));
        CHECK_EQ(ep.address(), ep3.address());
        CHECK_EQ(ep3.port(), 2016);
        CHECK_EQ(ep2, ep3);

        auto const ep4 = Endpoint::from_string("192.0.2.112     2016");
        CHECK_FALSE(is_unspecified(ep4));
        CHECK_EQ(ep.address(), ep4.address());
        CHECK_EQ(ep4.port(), 2016);
        CHECK_EQ(ep3, ep4);

        CHECK_EQ(to_string(ep1), to_string(ep2));
        CHECK_EQ(to_string(ep1), to_string(ep3));
        CHECK_EQ(to_string(ep1), to_string(ep4));
    }

    {
        ep = Endpoint::from_string("[::]:2017");
        CHECK_UNARY(is_unspecified(ep));
        CHECK_EQ(ep.port(), 2017);
        CHECK_EQ(ep.address(), AddressV6{});
    }

    // Failures:
    failParseEP("192.0.2.112:port");
    failParseEP("ip:port");
    failParseEP("");
    failParseEP("1.2.3.256");

#if BOOST_OS_WINDOWS
    // windows asio bugs...false positives
    shouldParseEPV4("255", {{0, 0, 0, 255}}, 0, "0.0.0.255");
    shouldParseEPV4("512", {{0, 0, 2, 0}}, 0, "0.0.2.0");
    shouldParseEPV4("1.2.3:80", {{1, 2, 0, 3}}, 80, "1.2.0.3:80");
#else
    failParseEP("255");
    failParseEP("512");
    failParseEP("1.2.3:80");
#endif

    failParseEP("1.2.3.4:65536");
    failParseEP("1.2.3.4:89119");
    failParseEP("1.2.3:89119");
    failParseEP("[::1]:89119");
    failParseEP("[::az]:1");
    failParseEP("[1234:5678:90ab:cdef:1234:5678:90ab:cdef:1111]:1");
    failParseEP("[1234:5678:90ab:cdef:1234:5678:90ab:cdef:1111]:12345");
    failParseEP("abcdef:12345");
    failParseEP("[abcdef]:12345");
    failParseEP("foo.org 12345");

    // test with hashed container
    std::unordered_set<Endpoint> eps;
    constexpr auto items{100};
    float max_lf{0};
    for (auto i = 0; i < items; ++i)
    {
        eps.insert(randomEP(xrpl::rand_int(0, 1) == 1));
        max_lf = std::max(max_lf, eps.load_factor());
    }
    CHECK(eps.bucket_count() >= items);
    CHECK(max_lf > 0.90);
}

TEST_CASE("Parse Endpoint")
{
    shouldPass<Endpoint>("0.0.0.0");
    shouldPass<Endpoint>("192.168.0.1");
    shouldPass<Endpoint>("168.127.149.132");
    shouldPass<Endpoint>("168.127.149.132:80");
    shouldPass<Endpoint>("168.127.149.132:54321");
    shouldPass<Endpoint>("2001:db8:a0b:12f0::1");
    shouldPass<Endpoint>("[2001:db8:a0b:12f0::1]:8");
    shouldPass<Endpoint>("2001:db8:a0b:12f0::1 8", "[2001:db8:a0b:12f0::1]:8");
    shouldPass<Endpoint>("[::1]:8");
    shouldPass<Endpoint>("[2001:2002:2003:2004:2005:2006:2007:2008]:65535");

    shouldFail<Endpoint>("1.2.3.256");
    shouldFail<Endpoint>("");
#if BOOST_OS_WINDOWS
    // windows asio bugs...false positives
    shouldPass<Endpoint>("512", "0.0.2.0");
    shouldPass<Endpoint>("255", "0.0.0.255");
    shouldPass<Endpoint>("1.2.3:80", "1.2.0.3:80");
#else
    shouldFail<Endpoint>("512");
    shouldFail<Endpoint>("255");
    shouldFail<Endpoint>("1.2.3:80");
#endif
    shouldFail<Endpoint>("1.2.3:65536");
    shouldFail<Endpoint>("1.2.3:72131");
    shouldFail<Endpoint>("[::1]:89119");
    shouldFail<Endpoint>("[::az]:1");
    shouldFail<Endpoint>("[1234:5678:90ab:cdef:1234:5678:90ab:cdef:1111]:1");
    shouldFail<Endpoint>(
        "[1234:5678:90ab:cdef:1234:5678:90ab:cdef:1111]:12345");
}

TEST_SUITE_END();

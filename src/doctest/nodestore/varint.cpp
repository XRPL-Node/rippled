#include <xrpl/nodestore/detail/varint.h>

#include <doctest/doctest.h>

#include <array>
#include <vector>

using namespace xrpl::NodeStore;

TEST_SUITE_BEGIN("varint");

TEST_CASE("encode, decode")
{
    std::vector<std::size_t> vv = {
        0,
        1,
        2,
        126,
        127,
        128,
        253,
        254,
        255,
        16127,
        16128,
        16129,
        0xff,
        0xffff,
        0xffffffff,
        0xffffffffffffUL,
        0xffffffffffffffffUL};

    for (auto const v : vv)
    {
        std::array<std::uint8_t, varint_traits<std::size_t>::max> vi;
        auto const n0 = write_varint(vi.data(), v);
        CHECK_GT(n0, 0);
        CHECK_EQ(n0, size_varint(v));
        std::size_t v1;
        auto const n1 = read_varint(vi.data(), n0, v1);
        CHECK_EQ(n1, n0);
        CHECK_EQ(v, v1);
    }
}

TEST_SUITE_END();

#include <xrpl/basics/Buffer.h>

#include <doctest/doctest.h>

#include <cstdint>
#include <type_traits>

using namespace xrpl;

namespace {
bool
sane(Buffer const& b)
{
    if (b.size() == 0)
        return b.data() == nullptr;

    return b.data() != nullptr;
}
}  // namespace

TEST_SUITE_BEGIN("Buffer");

TEST_CASE("basic operations")
{
    std::uint8_t const data[] = {
        0xa8, 0xa1, 0x38, 0x45, 0x23, 0xec, 0xe4, 0x23, 0x71, 0x6d, 0x2a,
        0x18, 0xb4, 0x70, 0xcb, 0xf5, 0xac, 0x2d, 0x89, 0x4d, 0x19, 0x9c,
        0xf0, 0x2c, 0x15, 0xd1, 0xf9, 0x9b, 0x66, 0xd2, 0x30, 0xd3};

    Buffer b0;
    CHECK(sane(b0));
    CHECK(b0.empty());

    Buffer b1{0};
    CHECK(sane(b1));
    CHECK(b1.empty());
    std::memcpy(b1.alloc(16), data, 16);
    CHECK(sane(b1));
    CHECK(!b1.empty());
    CHECK(b1.size() == 16);

    Buffer b2{b1.size()};
    CHECK(sane(b2));
    CHECK(!b2.empty());
    CHECK(b2.size() == b1.size());
    std::memcpy(b2.data(), data + 16, 16);

    Buffer b3{data, sizeof(data)};
    CHECK(sane(b3));
    CHECK(!b3.empty());
    CHECK(b3.size() == sizeof(data));
    CHECK(std::memcmp(b3.data(), data, b3.size()) == 0);

    // Check equality and inequality comparisons
    CHECK(b0 == b0);
    CHECK(b0 != b1);
    CHECK(b1 == b1);
    CHECK(b1 != b2);
    CHECK(b2 != b3);

    SUBCASE("Copy Construction / Assignment")
    {
        Buffer x{b0};
        CHECK(x == b0);
        CHECK(sane(x));
        Buffer y{b1};
        CHECK(y == b1);
        CHECK(sane(y));
        x = b2;
        CHECK(x == b2);
        CHECK(sane(x));
        x = y;
        CHECK(x == y);
        CHECK(sane(x));
        y = b3;
        CHECK(y == b3);
        CHECK(sane(y));
        x = b0;
        CHECK(x == b0);
        CHECK(sane(x));
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif

        x = x;
        CHECK(x == b0);
        CHECK(sane(x));
        y = y;
        CHECK(y == b3);
        CHECK(sane(y));

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    }

    SUBCASE("Move Construction / Assignment")
    {
        static_assert(std::is_nothrow_move_constructible<Buffer>::value, "");
        static_assert(std::is_nothrow_move_assignable<Buffer>::value, "");

        {  // Move-construct from empty buf
            Buffer x;
            Buffer y{std::move(x)};
            CHECK(sane(x));
            CHECK(x.empty());
            CHECK(sane(y));
            CHECK(y.empty());
            CHECK(x == y);
        }

        {  // Move-construct from non-empty buf
            Buffer x{b1};
            Buffer y{std::move(x)};
            CHECK(sane(x));
            CHECK(x.empty());
            CHECK(sane(y));
            CHECK(y == b1);
        }

        {  // Move assign empty buf to empty buf
            Buffer x;
            Buffer y;

            x = std::move(y);
            CHECK(sane(x));
            CHECK(x.empty());
            CHECK(sane(y));
            CHECK(y.empty());
        }

        {  // Move assign non-empty buf to empty buf
            Buffer x;
            Buffer y{b1};

            x = std::move(y);
            CHECK(sane(x));
            CHECK(x == b1);
            CHECK(sane(y));
            CHECK(y.empty());
        }

        {  // Move assign empty buf to non-empty buf
            Buffer x{b1};
            Buffer y;

            x = std::move(y);
            CHECK(sane(x));
            CHECK(x.empty());
            CHECK(sane(y));
            CHECK(y.empty());
        }

        {  // Move assign non-empty buf to non-empty buf
            Buffer x{b1};
            Buffer y{b2};
            Buffer z{b3};

            x = std::move(y);
            CHECK(sane(x));
            CHECK(!x.empty());
            CHECK(sane(y));
            CHECK(y.empty());

            x = std::move(z);
            CHECK(sane(x));
            CHECK(!x.empty());
            CHECK(sane(z));
            CHECK(z.empty());
        }
    }

    SUBCASE("Slice Conversion / Construction / Assignment")
    {
        Buffer w{static_cast<Slice>(b0)};
        CHECK(sane(w));
        CHECK(w == b0);

        Buffer x{static_cast<Slice>(b1)};
        CHECK(sane(x));
        CHECK(x == b1);

        Buffer y{static_cast<Slice>(b2)};
        CHECK(sane(y));
        CHECK(y == b2);

        Buffer z{static_cast<Slice>(b3)};
        CHECK(sane(z));
        CHECK(z == b3);

        // Assign empty slice to empty buffer
        w = static_cast<Slice>(b0);
        CHECK(sane(w));
        CHECK(w == b0);

        // Assign non-empty slice to empty buffer
        w = static_cast<Slice>(b1);
        CHECK(sane(w));
        CHECK(w == b1);

        // Assign non-empty slice to non-empty buffer
        x = static_cast<Slice>(b2);
        CHECK(sane(x));
        CHECK(x == b2);

        // Assign non-empty slice to non-empty buffer
        y = static_cast<Slice>(z);
        CHECK(sane(y));
        CHECK(y == z);

        // Assign empty slice to non-empty buffer:
        z = static_cast<Slice>(b0);
        CHECK(sane(z));
        CHECK(z == b0);
    }

    SUBCASE("Allocation, Deallocation and Clearing")
    {
        auto test = [](Buffer const& b, std::size_t i) {
            Buffer x{b};

            // Try to allocate some number of bytes, possibly
            // zero (which means clear) and sanity check
            x(i);
            CHECK(sane(x));
            CHECK(x.size() == i);
            CHECK((x.data() == nullptr) == (i == 0));

            // Try to allocate some more data (always non-zero)
            x(i + 1);
            CHECK(sane(x));
            CHECK(x.size() == i + 1);
            CHECK(x.data() != nullptr);

            // Try to clear:
            x.clear();
            CHECK(sane(x));
            CHECK(x.size() == 0);
            CHECK(x.data() == nullptr);

            // Try to clear again:
            x.clear();
            CHECK(sane(x));
            CHECK(x.size() == 0);
            CHECK(x.data() == nullptr);
        };

        for (std::size_t i = 0; i < 16; ++i)
        {
            test(b0, i);
            test(b1, i);
        }
    }
}

TEST_SUITE_END();

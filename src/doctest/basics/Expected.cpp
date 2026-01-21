#include <xrpl/basics/Expected.h>
#include <xrpl/protocol/TER.h>

#include <doctest/doctest.h>

#if BOOST_VERSION >= 107500
#include <boost/json.hpp>  // Not part of boost before version 1.75
#endif                     // BOOST_VERSION
#include <array>
#include <cstdint>

using namespace xrpl;

TEST_SUITE_BEGIN("Expected");

TEST_CASE("non-error const construction")
{
    auto const expected = []() -> Expected<std::string, TER> {
        return "Valid value";
    }();
    CHECK_UNARY(expected);
    CHECK_UNARY(expected.has_value());
    CHECK_EQ(expected.value(), "Valid value");
    CHECK_EQ(*expected, "Valid value");
    CHECK_EQ(expected->at(0), 'V');

    bool throwOccurred = false;
    try
    {
        // There's no error, so should throw.
        [[maybe_unused]] TER const t = expected.error();
    }
    catch (std::runtime_error const& e)
    {
        CHECK_EQ(e.what(), std::string("bad expected access"));
        throwOccurred = true;
    }
    CHECK_UNARY(throwOccurred);
}

TEST_CASE("non-error non-const construction")
{
    auto expected = []() -> Expected<std::string, TER> {
        return "Valid value";
    }();
    CHECK_UNARY(expected);
    CHECK_UNARY(expected.has_value());
    CHECK_EQ(expected.value(), "Valid value");
    CHECK_EQ(*expected, "Valid value");
    CHECK_EQ(expected->at(0), 'V');
    std::string mv = std::move(*expected);
    CHECK_EQ(mv, "Valid value");

    bool throwOccurred = false;
    try
    {
        // There's no error, so should throw.
        [[maybe_unused]] TER const t = expected.error();
    }
    catch (std::runtime_error const& e)
    {
        CHECK_EQ(e.what(), std::string("bad expected access"));
        throwOccurred = true;
    }
    CHECK_UNARY(throwOccurred);
}

TEST_CASE("non-error overlapping type construction")
{
    auto expected = []() -> Expected<std::uint32_t, std::uint16_t> {
        return 1;
    }();
    CHECK_UNARY(expected);
    CHECK_UNARY(expected.has_value());
    CHECK_EQ(expected.value(), 1);
    CHECK_EQ(*expected, 1);

    bool throwOccurred = false;
    try
    {
        // There's no error, so should throw.
        [[maybe_unused]] std::uint16_t const t = expected.error();
    }
    catch (std::runtime_error const& e)
    {
        CHECK_EQ(e.what(), std::string("bad expected access"));
        throwOccurred = true;
    }
    CHECK_UNARY(throwOccurred);
}

TEST_CASE("error construction from rvalue")
{
    auto const expected = []() -> Expected<std::string, TER> {
        return Unexpected(telLOCAL_ERROR);
    }();
    CHECK_FALSE(expected);
    CHECK_FALSE(expected.has_value());
    CHECK_EQ(expected.error(), telLOCAL_ERROR);

    bool throwOccurred = false;
    try
    {
        // There's no result, so should throw.
        [[maybe_unused]] std::string const s = *expected;
    }
    catch (std::runtime_error const& e)
    {
        CHECK_EQ(e.what(), std::string("bad expected access"));
        throwOccurred = true;
    }
    CHECK_UNARY(throwOccurred);
}

TEST_CASE("error construction from lvalue")
{
    auto const err(telLOCAL_ERROR);
    auto expected = [&err]() -> Expected<std::string, TER> {
        return Unexpected(err);
    }();
    CHECK_FALSE(expected);
    CHECK_FALSE(expected.has_value());
    CHECK_EQ(expected.error(), telLOCAL_ERROR);

    bool throwOccurred = false;
    try
    {
        // There's no result, so should throw.
        [[maybe_unused]] std::size_t const s = expected->size();
    }
    catch (std::runtime_error const& e)
    {
        CHECK_EQ(e.what(), std::string("bad expected access"));
        throwOccurred = true;
    }
    CHECK_UNARY(throwOccurred);
}

TEST_CASE("error construction from const char*")
{
    auto const expected = []() -> Expected<int, char const*> {
        return Unexpected("Not what is expected!");
    }();
    CHECK_FALSE(expected);
    CHECK_FALSE(expected.has_value());
    CHECK_EQ(expected.error(), std::string("Not what is expected!"));
}

TEST_CASE("error construction of string from const char*")
{
    auto expected = []() -> Expected<int, std::string> {
        return Unexpected("Not what is expected!");
    }();
    CHECK_FALSE(expected);
    CHECK_FALSE(expected.has_value());
    CHECK_EQ(expected.error(), "Not what is expected!");
    std::string const s(std::move(expected.error()));
    CHECK_EQ(s, "Not what is expected!");
}

TEST_CASE("non-error const construction of Expected<void, T>")
{
    auto const expected = []() -> Expected<void, std::string> { return {}; }();
    CHECK_UNARY(expected);
    bool throwOccurred = false;
    try
    {
        // There's no error, so should throw.
        [[maybe_unused]] std::size_t const s = expected.error().size();
    }
    catch (std::runtime_error const& e)
    {
        CHECK_EQ(e.what(), std::string("bad expected access"));
        throwOccurred = true;
    }
    CHECK_UNARY(throwOccurred);
}

TEST_CASE("non-error non-const construction of Expected<void, T>")
{
    auto expected = []() -> Expected<void, std::string> { return {}; }();
    CHECK_UNARY(expected);
    bool throwOccurred = false;
    try
    {
        // There's no error, so should throw.
        [[maybe_unused]] std::size_t const s = expected.error().size();
    }
    catch (std::runtime_error const& e)
    {
        CHECK_EQ(e.what(), std::string("bad expected access"));
        throwOccurred = true;
    }
    CHECK_UNARY(throwOccurred);
}

TEST_CASE("error const construction of Expected<void, T>")
{
    auto const expected = []() -> Expected<void, std::string> {
        return Unexpected("Not what is expected!");
    }();
    CHECK_FALSE(expected);
    CHECK_EQ(expected.error(), "Not what is expected!");
}

TEST_CASE("error non-const construction of Expected<void, T>")
{
    auto expected = []() -> Expected<void, std::string> {
        return Unexpected("Not what is expected!");
    }();
    CHECK_FALSE(expected);
    CHECK_EQ(expected.error(), "Not what is expected!");
    std::string const s(std::move(expected.error()));
    CHECK_EQ(s, "Not what is expected!");
}

#if BOOST_VERSION >= 107500
TEST_CASE("boost::json::value construction")
{
    auto expected = []() -> Expected<boost::json::value, std::string> {
        return boost::json::object{{"oops", "me array now"}};
    }();
    CHECK_UNARY(expected);
    CHECK_FALSE(expected.value().is_array());
}
#endif  // BOOST_VERSION

TEST_SUITE_END();

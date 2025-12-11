#include <xrpl/basics/base_uint.h>
#include <xrpl/basics/join.h>

#include <doctest/doctest.h>

#include <array>
#include <sstream>
#include <string>
#include <vector>

using namespace xrpl;

TEST_SUITE_BEGIN("join");

TEST_CASE("CollectionAndDelimiter")
{
    auto test = [](auto collectionanddelimiter, std::string expected) {
        std::stringstream ss;
        // Put something else in the buffer before and after to ensure that
        // the << operator returns the stream correctly.
        ss << "(" << collectionanddelimiter << ")";
        auto const str = ss.str();
        CHECK(str.substr(1, str.length() - 2) == expected);
        CHECK(str.front() == '(');
        CHECK(str.back() == ')');
    };

    // C++ array
    test(
        CollectionAndDelimiter(std::array<int, 4>{2, -1, 5, 10}, "/"),
        "2/-1/5/10");
    // One item C++ array edge case
    test(
        CollectionAndDelimiter(std::array<std::string, 1>{"test"}, " & "),
        "test");
    // Empty C++ array edge case
    test(CollectionAndDelimiter(std::array<int, 0>{}, ","), "");
    {
        // C-style array
        char letters[4]{'w', 'a', 's', 'd'};
        test(CollectionAndDelimiter(letters, std::to_string(0)), "w0a0s0d");
    }
    {
        // Auto sized C-style array
        std::string words[]{"one", "two", "three", "four"};
        test(CollectionAndDelimiter(words, "\n"), "one\ntwo\nthree\nfour");
    }
    {
        // One item C-style array edge case
        std::string words[]{"thing"};
        test(CollectionAndDelimiter(words, "\n"), "thing");
    }
    // Initializer list
    test(
        CollectionAndDelimiter(std::initializer_list<size_t>{19, 25}, "+"),
        "19+25");
    // vector
    test(
        CollectionAndDelimiter(std::vector<int>{0, 42}, std::to_string(99)),
        "09942");
    // empty vector edge case
    test(CollectionAndDelimiter(std::vector<uint256>{}, ","), "");
    // C-style string
    test(CollectionAndDelimiter("string", " "), "s t r i n g");
    // Empty C-style string edge case
    test(CollectionAndDelimiter("", "*"), "");
    // Single char C-style string edge case
    test(CollectionAndDelimiter("x", "*"), "x");
    // std::string
    test(CollectionAndDelimiter(std::string{"string"}, "-"), "s-t-r-i-n-g");
    // Empty std::string edge case
    test(CollectionAndDelimiter(std::string{""}, "*"), "");
    // Single char std::string edge case
    test(CollectionAndDelimiter(std::string{"y"}, "*"), "y");
}

TEST_SUITE_END();


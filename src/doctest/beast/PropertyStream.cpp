#include <xrpl/beast/utility/PropertyStream.h>

#include <doctest/doctest.h>

using namespace beast;
using Source = PropertyStream::Source;

TEST_SUITE_BEGIN("PropertyStream");

namespace {

void
test_peel_name(
    std::string s,
    std::string const& expected,
    std::string const& expected_remainder)
{
    std::string const peeled_name = Source::peel_name(&s);
    CHECK(peeled_name == expected);
    CHECK(s == expected_remainder);
}

void
test_peel_leading_slash(
    std::string s,
    std::string const& expected,
    bool should_be_found)
{
    bool const found(Source::peel_leading_slash(&s));
    CHECK(found == should_be_found);
    CHECK(s == expected);
}

void
test_peel_trailing_slashstar(
    std::string s,
    std::string const& expected_remainder,
    bool should_be_found)
{
    bool const found(Source::peel_trailing_slashstar(&s));
    CHECK(found == should_be_found);
    CHECK(s == expected_remainder);
}

void
test_find_one(Source& root, Source* expected, std::string const& name)
{
    Source* source(root.find_one(name));
    CHECK(source == expected);
}

void
test_find_path(Source& root, std::string const& path, Source* expected)
{
    Source* source(root.find_path(path));
    CHECK(source == expected);
}

void
test_find_one_deep(Source& root, std::string const& name, Source* expected)
{
    Source* source(root.find_one_deep(name));
    CHECK(source == expected);
}

void
test_find(Source& root, std::string path, Source* expected, bool expected_star)
{
    auto const result(root.find(path));
    CHECK(result.first == expected);
    CHECK(result.second == expected_star);
}

}  // namespace

TEST_CASE("peel_name")
{
    test_peel_name("a", "a", "");
    test_peel_name("foo/bar", "foo", "bar");
    test_peel_name("foo/goo/bar", "foo", "goo/bar");
    test_peel_name("", "", "");
}

TEST_CASE("peel_leading_slash")
{
    test_peel_leading_slash("foo/", "foo/", false);
    test_peel_leading_slash("foo", "foo", false);
    test_peel_leading_slash("/foo/", "foo/", true);
    test_peel_leading_slash("/foo", "foo", true);
}

TEST_CASE("peel_trailing_slashstar")
{
    test_peel_trailing_slashstar("/foo/goo/*", "/foo/goo", true);
    test_peel_trailing_slashstar("foo/goo/*", "foo/goo", true);
    test_peel_trailing_slashstar("/foo/goo/", "/foo/goo", false);
    test_peel_trailing_slashstar("foo/goo", "foo/goo", false);
    test_peel_trailing_slashstar("", "", false);
    test_peel_trailing_slashstar("/", "", false);
    test_peel_trailing_slashstar("/*", "", true);
    test_peel_trailing_slashstar("//", "/", false);
    test_peel_trailing_slashstar("**", "*", true);
    test_peel_trailing_slashstar("*/", "*", false);
}

TEST_CASE("find_one")
{
    Source a("a");
    Source b("b");
    Source c("c");
    Source d("d");
    Source e("e");
    Source f("f");
    Source g("g");

    // a { b { d { f }, e }, c { g } }
    a.add(b);
    a.add(c);
    c.add(g);
    b.add(d);
    b.add(e);
    d.add(f);

    test_find_one(a, &b, "b");
    test_find_one(a, nullptr, "d");
    test_find_one(b, &e, "e");
    test_find_one(d, &f, "f");
}

TEST_CASE("find_path")
{
    Source a("a");
    Source b("b");
    Source c("c");
    Source d("d");
    Source e("e");
    Source f("f");
    Source g("g");

    a.add(b);
    a.add(c);
    c.add(g);
    b.add(d);
    b.add(e);
    d.add(f);

    test_find_path(a, "a", nullptr);
    test_find_path(a, "e", nullptr);
    test_find_path(a, "a/b", nullptr);
    test_find_path(a, "a/b/e", nullptr);
    test_find_path(a, "b/e/g", nullptr);
    test_find_path(a, "b/e/f", nullptr);
    test_find_path(a, "b", &b);
    test_find_path(a, "b/e", &e);
    test_find_path(a, "b/d/f", &f);
}

TEST_CASE("find_one_deep")
{
    Source a("a");
    Source b("b");
    Source c("c");
    Source d("d");
    Source e("e");
    Source f("f");
    Source g("g");

    a.add(b);
    a.add(c);
    c.add(g);
    b.add(d);
    b.add(e);
    d.add(f);

    test_find_one_deep(a, "z", nullptr);
    test_find_one_deep(a, "g", &g);
    test_find_one_deep(a, "b", &b);
    test_find_one_deep(a, "d", &d);
    test_find_one_deep(a, "f", &f);
}

TEST_CASE("find")
{
    Source a("a");
    Source b("b");
    Source c("c");
    Source d("d");
    Source e("e");
    Source f("f");
    Source g("g");

    a.add(b);
    a.add(c);
    c.add(g);
    b.add(d);
    b.add(e);
    d.add(f);

    test_find(a, "", &a, false);
    test_find(a, "*", &a, true);
    test_find(a, "/b", &b, false);
    test_find(a, "b", &b, false);
    test_find(a, "d", &d, false);
    test_find(a, "/b*", &b, true);
    test_find(a, "b*", &b, true);
    test_find(a, "d*", &d, true);
    test_find(a, "/b/*", &b, true);
    test_find(a, "b/*", &b, true);
    test_find(a, "d/*", &d, true);
    test_find(a, "a", nullptr, false);
    test_find(a, "/d", nullptr, false);
    test_find(a, "/d*", nullptr, true);
    test_find(a, "/d/*", nullptr, true);
}

TEST_SUITE_END();

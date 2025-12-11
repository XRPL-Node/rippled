#include <xrpl/basics/hardened_hash.h>

#include <doctest/doctest.h>

#include <array>
#include <cstdint>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>

using namespace xrpl;

namespace {

template <class T>
class test_user_type_member
{
private:
    T t;

public:
    explicit test_user_type_member(T const& t_ = T()) : t(t_)
    {
    }

    template <class Hasher>
    friend void
    hash_append(Hasher& h, test_user_type_member const& a) noexcept
    {
        using beast::hash_append;
        hash_append(h, a.t);
    }
};

template <class T>
class test_user_type_free
{
private:
    T t;

public:
    explicit test_user_type_free(T const& t_ = T()) : t(t_)
    {
    }

    template <class Hasher>
    friend void
    hash_append(Hasher& h, test_user_type_free const& a) noexcept
    {
        using beast::hash_append;
        hash_append(h, a.t);
    }
};

template <class T>
using test_hardened_unordered_set = std::unordered_set<T, hardened_hash<>>;

template <class T>
using test_hardened_unordered_map = std::unordered_map<T, int, hardened_hash<>>;

template <class T>
using test_hardened_unordered_multiset =
    std::unordered_multiset<T, hardened_hash<>>;

template <class T>
using test_hardened_unordered_multimap =
    std::unordered_multimap<T, int, hardened_hash<>>;

template <class T>
void
check()
{
    T t{};
    hardened_hash<>()(t);
}

template <template <class T> class U>
void
check_user_type()
{
    check<U<bool>>();
    check<U<char>>();
    check<U<signed char>>();
    check<U<unsigned char>>();
    check<U<wchar_t>>();
    check<U<short>>();
    check<U<unsigned short>>();
    check<U<int>>();
    check<U<unsigned int>>();
    check<U<long>>();
    check<U<long long>>();
    check<U<unsigned long>>();
    check<U<unsigned long long>>();
    check<U<float>>();
    check<U<double>>();
    check<U<long double>>();
}

template <template <class T> class C>
void
check_container()
{
    {
        C<test_user_type_member<std::string>> c;
        (void)c;
    }

    {
        C<test_user_type_free<std::string>> c;
        (void)c;
    }
}

}  // namespace

TEST_SUITE_BEGIN("hardened_hash");

TEST_CASE("user types")
{
    check_user_type<test_user_type_member>();
    check_user_type<test_user_type_free>();
}

TEST_CASE("containers")
{
    check_container<test_hardened_unordered_set>();
    check_container<test_hardened_unordered_map>();
    check_container<test_hardened_unordered_multiset>();
    check_container<test_hardened_unordered_multimap>();
}

TEST_SUITE_END();

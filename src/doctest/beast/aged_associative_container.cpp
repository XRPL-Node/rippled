// Migrated from src/test/beast/aged_associative_container_test.cpp to doctest

#include <xrpl/beast/clock/manual_clock.h>
#include <xrpl/beast/container/aged_map.h>
#include <xrpl/beast/container/aged_multimap.h>
#include <xrpl/beast/container/aged_multiset.h>
#include <xrpl/beast/container/aged_set.h>
#include <xrpl/beast/container/aged_unordered_map.h>
#include <xrpl/beast/container/aged_unordered_multimap.h>
#include <xrpl/beast/container/aged_unordered_multiset.h>
#include <xrpl/beast/container/aged_unordered_set.h>

#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <list>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef BEAST_AGED_UNORDERED_NO_ALLOC_DEFAULTCTOR
#ifdef _MSC_VER
#define BEAST_AGED_UNORDERED_NO_ALLOC_DEFAULTCTOR 0
#else
#define BEAST_AGED_UNORDERED_NO_ALLOC_DEFAULTCTOR 1
#endif
#endif

#ifndef BEAST_CONTAINER_EXTRACT_NOREF
#ifdef _MSC_VER
#define BEAST_CONTAINER_EXTRACT_NOREF 1
#else
#define BEAST_CONTAINER_EXTRACT_NOREF 1
#endif
#endif

namespace beast {
namespace {

//------------------------------------------------------------------------------
// Helper types

template <class T>
struct CompT
{
    explicit CompT(int)
    {
    }

    CompT(CompT const&)
    {
    }

    bool
    operator()(T const& lhs, T const& rhs) const
    {
        return m_less(lhs, rhs);
    }

private:
    CompT() = delete;
    std::less<T> m_less;
};

template <class T>
class HashT
{
public:
    explicit HashT(int)
    {
    }

    std::size_t
    operator()(T const& t) const
    {
        return m_hash(t);
    }

private:
    HashT() = delete;
    std::hash<T> m_hash;
};

template <class T>
struct EqualT
{
public:
    explicit EqualT(int)
    {
    }

    bool
    operator()(T const& lhs, T const& rhs) const
    {
        return m_eq(lhs, rhs);
    }

private:
    EqualT() = delete;
    std::equal_to<T> m_eq;
};

template <class T>
struct AllocT
{
    using value_type = T;

    template <class U>
    struct rebind
    {
        using other = AllocT<U>;
    };

    explicit AllocT(int)
    {
    }

    AllocT(AllocT const&) = default;

    template <class U>
    AllocT(AllocT<U> const&)
    {
    }

    template <class U>
    bool
    operator==(AllocT<U> const&) const
    {
        return true;
    }

    template <class U>
    bool
    operator!=(AllocT<U> const& o) const
    {
        return !(*this == o);
    }

    T*
    allocate(std::size_t n, T const* = 0)
    {
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void
    deallocate(T* p, std::size_t)
    {
        ::operator delete(p);
    }

#if !BEAST_AGED_UNORDERED_NO_ALLOC_DEFAULTCTOR
    AllocT()
    {
    }
#else
private:
    AllocT() = delete;
#endif
};

//------------------------------------------------------------------------------
// Trait hierarchy

// ordered
template <class Base, bool IsUnordered>
class MaybeUnordered : public Base
{
public:
    using Comp = std::less<typename Base::Key>;
    using MyComp = CompT<typename Base::Key>;

protected:
    static std::string
    name_ordered_part()
    {
        return "";
    }
};

// unordered
template <class Base>
class MaybeUnordered<Base, true> : public Base
{
public:
    using Hash = std::hash<typename Base::Key>;
    using Equal = std::equal_to<typename Base::Key>;
    using MyHash = HashT<typename Base::Key>;
    using MyEqual = EqualT<typename Base::Key>;

protected:
    static std::string
    name_ordered_part()
    {
        return "unordered_";
    }
};

// unique
template <class Base, bool IsMulti>
class MaybeMulti : public Base
{
protected:
    static std::string
    name_multi_part()
    {
        return "";
    }
};

// multi
template <class Base>
class MaybeMulti<Base, true> : public Base
{
protected:
    static std::string
    name_multi_part()
    {
        return "multi";
    }
};

// set
template <class Base, bool IsMap>
class MaybeMap : public Base
{
public:
    using T = void;
    using Value = typename Base::Key;
    using Values = std::vector<Value>;

    static typename Base::Key const&
    extract(Value const& value)
    {
        return value;
    }

    static Values
    values()
    {
        Values v{
            "apple",
            "banana",
            "cherry",
            "grape",
            "orange",
        };
        return v;
    }

protected:
    static std::string
    name_map_part()
    {
        return "set";
    }
};

// map
template <class Base>
class MaybeMap<Base, true> : public Base
{
public:
    using T = int;
    using Value = std::pair<typename Base::Key, T>;
    using Values = std::vector<Value>;

    static typename Base::Key const&
    extract(Value const& value)
    {
        return value.first;
    }

    static Values
    values()
    {
        Values v{
            std::make_pair("apple", 1),
            std::make_pair("banana", 2),
            std::make_pair("cherry", 3),
            std::make_pair("grape", 4),
            std::make_pair("orange", 5)};
        return v;
    }

protected:
    static std::string
    name_map_part()
    {
        return "map";
    }
};

//------------------------------------------------------------------------------
// Container types

// ordered
template <class Base, bool IsUnordered = Base::is_unordered::value>
struct ContType
{
    template <
        class Compare = std::less<typename Base::Key>,
        class Allocator = std::allocator<typename Base::Value>>
    using Cont = detail::aged_ordered_container<
        Base::is_multi::value,
        Base::is_map::value,
        typename Base::Key,
        typename Base::T,
        typename Base::Clock,
        Compare,
        Allocator>;
};

// unordered
template <class Base>
struct ContType<Base, true>
{
    template <
        class Hash = std::hash<typename Base::Key>,
        class KeyEqual = std::equal_to<typename Base::Key>,
        class Allocator = std::allocator<typename Base::Value>>
    using Cont = detail::aged_unordered_container<
        Base::is_multi::value,
        Base::is_map::value,
        typename Base::Key,
        typename Base::T,
        typename Base::Clock,
        Hash,
        KeyEqual,
        Allocator>;
};

//------------------------------------------------------------------------------
// Test traits

struct TestTraitsBase
{
    using Key = std::string;
    using Clock = std::chrono::steady_clock;
    using ManualClock = manual_clock<Clock>;
};

template <bool IsUnordered, bool IsMulti, bool IsMap>
struct TestTraitsHelper
    : MaybeUnordered<
          MaybeMulti<MaybeMap<TestTraitsBase, IsMap>, IsMulti>,
          IsUnordered>
{
private:
    using Base = MaybeUnordered<
        MaybeMulti<MaybeMap<TestTraitsBase, IsMap>, IsMulti>,
        IsUnordered>;

public:
    using typename Base::Key;

    using is_unordered = std::integral_constant<bool, IsUnordered>;
    using is_multi = std::integral_constant<bool, IsMulti>;
    using is_map = std::integral_constant<bool, IsMap>;

    using Alloc = std::allocator<typename Base::Value>;
    using MyAlloc = AllocT<typename Base::Value>;

    static std::string
    name()
    {
        return std::string("aged_") + Base::name_ordered_part() +
            Base::name_multi_part() + Base::name_map_part();
    }
};

template <bool IsUnordered, bool IsMulti, bool IsMap>
struct TestTraits : TestTraitsHelper<IsUnordered, IsMulti, IsMap>,
                    ContType<TestTraitsHelper<IsUnordered, IsMulti, IsMap>>
{
};

template <class Cont>
std::string
name(Cont const&)
{
    return TestTraits<Cont::is_unordered, Cont::is_multi, Cont::is_map>::name();
}

template <class Traits>
struct equal_value
{
    bool
    operator()(
        typename Traits::Value const& lhs,
        typename Traits::Value const& rhs)
    {
        return Traits::extract(lhs) == Traits::extract(rhs);
    }
};

template <class Cont>
std::vector<typename Cont::value_type>
make_list(Cont const& c)
{
    return std::vector<typename Cont::value_type>(c.begin(), c.end());
}

//------------------------------------------------------------------------------
// Check contents helpers

// Check contents via at() and operator[]
// map, unordered_map
template <class Container, class Values>
typename std::enable_if<
    Container::is_map::value && !Container::is_multi::value>::type
checkMapContents(Container& c, Values const& v)
{
    if (v.empty())
    {
        CHECK_UNARY(c.empty());
        CHECK_EQ(c.size(), 0);
        return;
    }

    try
    {
        // Make sure no exception is thrown
        for (auto const& e : v)
            c.at(e.first);
        for (auto const& e : v)
            CHECK_EQ(c.operator[](e.first), e.second);
    }
    catch (std::out_of_range const&)
    {
        CHECK_UNARY(false);  // FAIL: caught exception
    }
}

template <class Container, class Values>
typename std::enable_if<
    !(Container::is_map::value && !Container::is_multi::value)>::type
checkMapContents(Container, Values const&)
{
}

// unordered
template <class C, class Values>
typename std::enable_if<
    std::remove_reference<C>::type::is_unordered::value>::type
checkUnorderedContentsRefRef(C&& c, Values const& v)
{
    using Cont = typename std::remove_reference<C>::type;
    using Traits = TestTraits<
        Cont::is_unordered::value,
        Cont::is_multi::value,
        Cont::is_map::value>;
    using size_type = typename Cont::size_type;
    auto const hash(c.hash_function());
    auto const key_eq(c.key_eq());
    for (size_type i(0); i < c.bucket_count(); ++i)
    {
        auto const last(c.end(i));
        for (auto iter(c.begin(i)); iter != last; ++iter)
        {
            auto const match(std::find_if(
                v.begin(),
                v.end(),
                [iter](typename Values::value_type const& e) {
                    return Traits::extract(*iter) == Traits::extract(e);
                }));
            CHECK_NE(match, v.end());
            CHECK(key_eq(Traits::extract(*iter), Traits::extract(*match)));
            CHECK_EQ(
                hash(Traits::extract(*iter)), hash(Traits::extract(*match)));
        }
    }
}

template <class C, class Values>
typename std::enable_if<
    !std::remove_reference<C>::type::is_unordered::value>::type
checkUnorderedContentsRefRef(C&&, Values const&)
{
}

template <class C, class Values>
void
checkContentsRefRef(C&& c, Values const& v)
{
    using Cont = typename std::remove_reference<C>::type;
    using size_type = typename Cont::size_type;

    CHECK_EQ(c.size(), v.size());
    CHECK_EQ(size_type(std::distance(c.begin(), c.end())), v.size());
    CHECK_EQ(size_type(std::distance(c.cbegin(), c.cend())), v.size());
    CHECK_EQ(
        size_type(
            std::distance(c.chronological.begin(), c.chronological.end())),
        v.size());
    CHECK_EQ(
        size_type(
            std::distance(c.chronological.cbegin(), c.chronological.cend())),
        v.size());
    CHECK_EQ(
        size_type(
            std::distance(c.chronological.rbegin(), c.chronological.rend())),
        v.size());
    CHECK_EQ(
        size_type(
            std::distance(c.chronological.crbegin(), c.chronological.crend())),
        v.size());

    checkUnorderedContentsRefRef(c, v);
}

template <class Cont, class Values>
void
checkContents(Cont& c, Values const& v)
{
    checkContentsRefRef(c, v);
    checkContentsRefRef(const_cast<Cont const&>(c), v);
    checkMapContents(c, v);
}

template <class Cont>
void
checkContents(Cont& c)
{
    using Traits = TestTraits<
        Cont::is_unordered::value,
        Cont::is_multi::value,
        Cont::is_map::value>;
    using Values = typename Traits::Values;
    checkContents(c, Values());
}

//------------------------------------------------------------------------------
// Test Construction - Ordered containers

template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if<!IsUnordered>::type
testConstructEmpty()
{
    using Traits = TestTraits<IsUnordered, IsMulti, IsMap>;
    using Comp = typename Traits::Comp;
    using Alloc = typename Traits::Alloc;
    using MyComp = typename Traits::MyComp;
    using MyAlloc = typename Traits::MyAlloc;
    typename Traits::ManualClock clock;

    SUBCASE("empty")
    {
        {
            typename Traits::template Cont<Comp, Alloc> c(clock);
            checkContents(c);
        }

        {
            typename Traits::template Cont<MyComp, Alloc> c(clock, MyComp(1));
            checkContents(c);
        }

        {
            typename Traits::template Cont<Comp, MyAlloc> c(clock, MyAlloc(1));
            checkContents(c);
        }

        {
            typename Traits::template Cont<MyComp, MyAlloc> c(
                clock, MyComp(1), MyAlloc(1));
            checkContents(c);
        }
    }
}

// Test Construction - Unordered containers
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if<IsUnordered>::type
testConstructEmpty()
{
    using Traits = TestTraits<IsUnordered, IsMulti, IsMap>;
    using Hash = typename Traits::Hash;
    using Equal = typename Traits::Equal;
    using Alloc = typename Traits::Alloc;
    using MyHash = typename Traits::MyHash;
    using MyEqual = typename Traits::MyEqual;
    using MyAlloc = typename Traits::MyAlloc;
    typename Traits::ManualClock clock;

    SUBCASE("empty")
    {
        {
            typename Traits::template Cont<Hash, Equal, Alloc> c(clock);
            checkContents(c);
        }

        {
            typename Traits::template Cont<MyHash, Equal, Alloc> c(
                clock, MyHash(1));
            checkContents(c);
        }

        {
            typename Traits::template Cont<Hash, MyEqual, Alloc> c(
                clock, Hash(), MyEqual(1));
            checkContents(c);
        }

        {
            typename Traits::template Cont<Hash, Equal, MyAlloc> c(
                clock, MyAlloc(1));
            checkContents(c);
        }

        {
            typename Traits::template Cont<MyHash, MyEqual, Alloc> c(
                clock, MyHash(1), MyEqual(1));
            checkContents(c);
        }

        {
            typename Traits::template Cont<MyHash, Equal, MyAlloc> c(
                clock, MyHash(1), Equal(), MyAlloc(1));
            checkContents(c);
        }

        {
            typename Traits::template Cont<Hash, MyEqual, MyAlloc> c(
                clock, Hash(), MyEqual(1), MyAlloc(1));
            checkContents(c);
        }

        {
            typename Traits::template Cont<MyHash, MyEqual, MyAlloc> c(
                clock, MyHash(1), MyEqual(1), MyAlloc(1));
            checkContents(c);
        }
    }
}

//------------------------------------------------------------------------------
// Test Construction with range - Ordered containers

template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if<!IsUnordered>::type
testConstructRange()
{
    using Traits = TestTraits<IsUnordered, IsMulti, IsMap>;
    using Comp = typename Traits::Comp;
    using Alloc = typename Traits::Alloc;
    using MyComp = typename Traits::MyComp;
    using MyAlloc = typename Traits::MyAlloc;
    typename Traits::ManualClock clock;

    SUBCASE("range")
    {
        auto const v(Traits::values());
        {
            typename Traits::template Cont<Comp, Alloc> c(
                v.begin(), v.end(), clock);
            checkContents(c, v);
        }

        {
            typename Traits::template Cont<MyComp, Alloc> c(
                v.begin(), v.end(), clock, MyComp(1));
            checkContents(c, v);
        }

        {
            typename Traits::template Cont<Comp, MyAlloc> c(
                v.begin(), v.end(), clock, MyAlloc(1));
            checkContents(c, v);
        }

        {
            typename Traits::template Cont<MyComp, MyAlloc> c(
                v.begin(), v.end(), clock, MyComp(1), MyAlloc(1));
            checkContents(c, v);
        }
    }
}

// Test Construction with range - Unordered containers
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if<IsUnordered>::type
testConstructRange()
{
    using Traits = TestTraits<IsUnordered, IsMulti, IsMap>;
    using Hash = typename Traits::Hash;
    using Equal = typename Traits::Equal;
    using Alloc = typename Traits::Alloc;
    using MyHash = typename Traits::MyHash;
    using MyEqual = typename Traits::MyEqual;
    using MyAlloc = typename Traits::MyAlloc;
    typename Traits::ManualClock clock;

    SUBCASE("range")
    {
        auto const v(Traits::values());
        {
            typename Traits::template Cont<Hash, Equal, Alloc> c(
                v.begin(), v.end(), clock);
            checkContents(c, v);
        }

        {
            typename Traits::template Cont<MyHash, Equal, Alloc> c(
                v.begin(), v.end(), clock, MyHash(1));
            checkContents(c, v);
        }

        {
            typename Traits::template Cont<Hash, MyEqual, Alloc> c(
                v.begin(), v.end(), clock, MyEqual(1));
            checkContents(c, v);
        }

        {
            typename Traits::template Cont<Hash, Equal, MyAlloc> c(
                v.begin(), v.end(), clock, MyAlloc(1));
            checkContents(c, v);
        }

        {
            typename Traits::template Cont<MyHash, MyEqual, Alloc> c(
                v.begin(), v.end(), clock, MyHash(1), MyEqual(1));
            checkContents(c, v);
        }

        {
            typename Traits::template Cont<MyHash, Equal, MyAlloc> c(
                v.begin(), v.end(), clock, MyHash(1), MyAlloc(1));
            checkContents(c, v);
        }

        {
            typename Traits::template Cont<Hash, MyEqual, MyAlloc> c(
                v.begin(), v.end(), clock, MyEqual(1), MyAlloc(1));
            checkContents(c, v);
        }

        {
            typename Traits::template Cont<MyHash, MyEqual, MyAlloc> c(
                v.begin(), v.end(), clock, MyHash(1), MyEqual(1), MyAlloc(1));
            checkContents(c, v);
        }
    }
}

//------------------------------------------------------------------------------
// Test Iterator

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
testIterator()
{
    using Traits = TestTraits<IsUnordered, IsMulti, IsMap>;
    typename Traits::ManualClock clock;

    SUBCASE("iterator")
    {
        using Cont = typename Traits::template Cont<>;
        auto const v(Traits::values());
        Cont c(v.cbegin(), v.cend(), clock);
        Cont const& cc(c);
        CHECK_FALSE(c.empty());
        CHECK_EQ(c.size(), v.size());

        {
            auto i = c.begin();
            CHECK_EQ(i, c.begin());
            CHECK_NE(i, c.end());
            ++i;
            CHECK_NE(i, c.begin());
        }

        {
            auto i = cc.begin();
            CHECK_EQ(i, cc.begin());
            CHECK_NE(i, cc.end());
            ++i;
            CHECK_NE(i, cc.begin());
        }

        {
            auto i = c.cbegin();
            CHECK_EQ(i, c.cbegin());
            CHECK_NE(i, c.cend());
            ++i;
            CHECK_NE(i, c.cbegin());
        }

        {
            auto i = cc.cbegin();
            CHECK_EQ(i, cc.cbegin());
            CHECK_NE(i, cc.cend());
            ++i;
            CHECK_NE(i, cc.cbegin());
        }
    }
}

// Unordered containers don't have reverse iterators
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if<!IsUnordered>::type
testReverseIterator()
{
    using Traits = TestTraits<IsUnordered, IsMulti, IsMap>;
    typename Traits::ManualClock clock;

    SUBCASE("reverse_iterator")
    {
        using Cont = typename Traits::template Cont<>;
        auto const v(Traits::values());
        Cont c(v.cbegin(), v.cend(), clock);
        Cont const& cc(c);

        {
            auto i = c.rbegin();
            CHECK_EQ(i, c.rbegin());
            CHECK_NE(i, c.rend());
            ++i;
            CHECK_NE(i, c.rbegin());
        }

        {
            auto i = cc.rbegin();
            CHECK_EQ(i, cc.rbegin());
            CHECK_NE(i, cc.rend());
            ++i;
            CHECK_NE(i, cc.rbegin());
        }

        {
            auto i = c.crbegin();
            CHECK_EQ(i, c.crbegin());
            CHECK_NE(i, c.crend());
            ++i;
            CHECK_NE(i, c.crbegin());
        }

        {
            auto i = cc.crbegin();
            CHECK_EQ(i, cc.crbegin());
            CHECK_NE(i, cc.crend());
            ++i;
            CHECK_NE(i, cc.crbegin());
        }
    }
}

template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if<IsUnordered>::type
testReverseIterator()
{
}

//------------------------------------------------------------------------------
// Modifier test helpers

template <class Container, class Values>
void
checkInsertCopy(Container& c, Values const& v)
{
    for (auto const& e : v)
    {
        auto result = c.insert(e);
        if constexpr (Container::is_multi::value)
        {
            CHECK_NE(result, c.end());
        }
        else
        {
            CHECK(result.second);
        }
    }
}

template <class Container, class Values>
void
checkInsertMove(Container& c, Values const& v)
{
    for (auto e : v)
    {
        auto result = c.insert(std::move(e));
        if constexpr (Container::is_multi::value)
        {
            CHECK_NE(result, c.end());
        }
        else
        {
            CHECK(result.second);
        }
    }
}

template <class Container, class Values>
void
checkInsertHintCopy(Container& c, Values const& v)
{
    for (auto const& e : v)
    {
        auto result = c.insert(c.cend(), e);
        // For map types with P&& overload, result may be pair<iterator, bool>
        // For other types, result is iterator
        if constexpr (std::is_same_v<
                          decltype(result),
                          std::pair<typename Container::iterator, bool>>)
        {
            CHECK_NE(result.first, c.end());
        }
        else
        {
            CHECK_NE(result, c.end());
        }
    }
}

template <class Container, class Values>
void
checkInsertHintMove(Container& c, Values const& v)
{
    for (auto e : v)
    {
        auto result = c.insert(c.cend(), std::move(e));
        // For map types with P&& overload, result may be pair<iterator, bool>
        // For other types, result is iterator
        if constexpr (std::is_same_v<
                          decltype(result),
                          std::pair<typename Container::iterator, bool>>)
        {
            CHECK_NE(result.first, c.end());
        }
        else
        {
            CHECK_NE(result, c.end());
        }
    }
}

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
testModifiers()
{
    using Traits = TestTraits<IsUnordered, IsMulti, IsMap>;
    typename Traits::ManualClock clock;

    SUBCASE("insert copy")
    {
        auto const v(Traits::values());
        typename Traits::template Cont<> c(clock);
        checkInsertCopy(c, v);
        checkContents(c, v);
    }

    SUBCASE("insert move")
    {
        auto const v(Traits::values());
        typename Traits::template Cont<> c(clock);
        checkInsertMove(c, v);
        checkContents(c, v);
    }

    SUBCASE("insert hint copy")
    {
        auto const v(Traits::values());
        typename Traits::template Cont<> c(clock);
        checkInsertHintCopy(c, v);
        checkContents(c, v);
    }

    SUBCASE("insert hint move")
    {
        auto const v(Traits::values());
        typename Traits::template Cont<> c(clock);
        checkInsertHintMove(c, v);
        checkContents(c, v);
    }
}

//------------------------------------------------------------------------------
// Chronological tests

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
testChronological()
{
    using Traits = TestTraits<IsUnordered, IsMulti, IsMap>;
    typename Traits::ManualClock clock;

    SUBCASE("chronological")
    {
        using Cont = typename Traits::template Cont<>;
        auto const v(Traits::values());
        Cont c(v.cbegin(), v.cend(), clock);
        Cont const& cc(c);

        // Check chronological iterators
        CHECK_FALSE(c.empty());
        CHECK_NE(c.chronological.begin(), c.chronological.end());
        CHECK_NE(cc.chronological.begin(), cc.chronological.end());
        CHECK_NE(c.chronological.cbegin(), c.chronological.cend());
        CHECK_NE(c.chronological.rbegin(), c.chronological.rend());
        CHECK_NE(cc.chronological.rbegin(), cc.chronological.rend());
        CHECK_NE(c.chronological.crbegin(), c.chronological.crend());

        // Check touch updates
        auto const before = c.clock().now();
        clock.advance(std::chrono::seconds(1));
        auto iter = c.begin();
        c.touch(iter);
        CHECK_GT(iter.when(), before);
    }
}

//------------------------------------------------------------------------------
// Main test function for each container type

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
testMaybeUnorderedMultiMap()
{
    testConstructEmpty<IsUnordered, IsMulti, IsMap>();
    testConstructRange<IsUnordered, IsMulti, IsMap>();
    testIterator<IsUnordered, IsMulti, IsMap>();
    testReverseIterator<IsUnordered, IsMulti, IsMap>();
    testModifiers<IsUnordered, IsMulti, IsMap>();
    testChronological<IsUnordered, IsMulti, IsMap>();
}

}  // namespace

//------------------------------------------------------------------------------
// Static assertions (compile-time checks)

using Key = std::string;
using T = int;

static_assert(
    std::is_same<
        aged_set<Key>,
        detail::aged_ordered_container<false, false, Key, void>>::value,
    "bad alias: aged_set");

static_assert(
    std::is_same<
        aged_multiset<Key>,
        detail::aged_ordered_container<true, false, Key, void>>::value,
    "bad alias: aged_multiset");

static_assert(
    std::is_same<
        aged_map<Key, T>,
        detail::aged_ordered_container<false, true, Key, T>>::value,
    "bad alias: aged_map");

static_assert(
    std::is_same<
        aged_multimap<Key, T>,
        detail::aged_ordered_container<true, true, Key, T>>::value,
    "bad alias: aged_multimap");

static_assert(
    std::is_same<
        aged_unordered_set<Key>,
        detail::aged_unordered_container<false, false, Key, void>>::value,
    "bad alias: aged_unordered_set");

static_assert(
    std::is_same<
        aged_unordered_multiset<Key>,
        detail::aged_unordered_container<true, false, Key, void>>::value,
    "bad alias: aged_unordered_multiset");

static_assert(
    std::is_same<
        aged_unordered_map<Key, T>,
        detail::aged_unordered_container<false, true, Key, T>>::value,
    "bad alias: aged_unordered_map");

static_assert(
    std::is_same<
        aged_unordered_multimap<Key, T>,
        detail::aged_unordered_container<true, true, Key, T>>::value,
    "bad alias: aged_unordered_multimap");

}  // namespace beast

//------------------------------------------------------------------------------
// TEST_CASE definitions

TEST_SUITE_BEGIN("beast");

TEST_CASE("aged_set")
{
    beast::testMaybeUnorderedMultiMap<false, false, false>();
}

TEST_CASE("aged_map")
{
    beast::testMaybeUnorderedMultiMap<false, false, true>();
}

TEST_CASE("aged_multiset")
{
    beast::testMaybeUnorderedMultiMap<false, true, false>();
}

TEST_CASE("aged_multimap")
{
    beast::testMaybeUnorderedMultiMap<false, true, true>();
}

TEST_CASE("aged_unordered_set")
{
    beast::testMaybeUnorderedMultiMap<true, false, false>();
}

TEST_CASE("aged_unordered_map")
{
    beast::testMaybeUnorderedMultiMap<true, false, true>();
}

TEST_CASE("aged_unordered_multiset")
{
    beast::testMaybeUnorderedMultiMap<true, true, false>();
}

TEST_CASE("aged_unordered_multimap")
{
    beast::testMaybeUnorderedMultiMap<true, true, true>();
}

TEST_SUITE_END();

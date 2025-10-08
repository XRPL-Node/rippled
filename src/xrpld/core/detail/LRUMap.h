//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef RIPPLE_APP_LRU_MAP_H_INCLUDED
#define RIPPLE_APP_LRU_MAP_H_INCLUDED

#include <list>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace ripple {

namespace concurrency {

struct SingleThreaded
{
    struct mutex_type
    {
        void
        lock() noexcept
        {
        }
        void
        unlock() noexcept
        {
        }
    };
    using lock_guard = std::lock_guard<mutex_type>;
};

struct ExclusiveMutex
{
    using mutex_type = std::mutex;
    using lock_guard = std::lock_guard<mutex_type>;
};

}  // namespace concurrency

template <
    class Key,
    class Value,
    class Concurrency = concurrency::SingleThreaded>
class LRUMap
{
    using List = std::list<Key>;                     // MRU .. LRU
    using DataMap = std::unordered_map<Key, Value>;  // Key -> Value
    using PosMap =
        std::unordered_map<Key, typename List::iterator>;  // Key -> pos
                                                           // iterator in the
                                                           // list

public:
    explicit LRUMap(std::size_t capacity) : capacity_(capacity)
    {
        if (capacity_ == 0)
            throw std::invalid_argument("LRUMap capacity must be positive.");
        data_.reserve(capacity_);
        pos_.reserve(capacity_);

        // TODO:
        //  static_assert(std::is_default_constructible_v<Value>,
        //        "LRUMap requires Value to be default-constructible for
        //        operator[]");
        //  static_assert(std::is_copy_constructible_v<Key> ||
        //  std::is_move_constructible_v<Key>,
        //        "LRUMap requires Key to be copy- or move-constructible");
    }

    LRUMap(LRUMap const&) = delete;

    LRUMap&
    operator=(LRUMap const&) = delete;

    LRUMap(LRUMap&&) = delete;

    LRUMap&
    operator=(LRUMap&&) = delete;

    Value&
    operator[](Key const& key)
    {
        auto g = lock();
        return insertOrpromote(key);
    }

    Value&
    operator[](Key&& key)
    {
        auto g = lock();
        auto it = data_.find(key);
        if (it != data_.end())
        {
            promote(key);
            return it->second;
        }
        evictIfFull();
        usage_.emplace_front(std::move(key));
        pos_.emplace(usage_.front(), usage_.begin());
        auto d = data_.emplace(usage_.front(), Value{});
        return d.first->second;
    }

    Value*
    get(Key const& key)
    {
        auto g = lock();
        auto it = data_.find(key);
        if (it == data_.end())
            return nullptr;
        promote(key);
        return &it->second;
    }

    template <class... Args>
    Value&
    put(Key const& key, Args&&... args)
    {
        auto g = lock();
        if (auto it = data_.find(key); it != data_.end())
        {
            it->second = Value(std::forward<Args>(args)...);
            promote(key);
            return it->second;
        }
        evictIfFull();
        usage_.emplace_front(key);
        pos_.emplace(key, usage_.begin());
        auto it = data_.emplace(key, Value(std::forward<Args>(args)...));
        return it.first->second;
    }

    bool
    erase(Key const& key)
    {
        auto g = lock();
        auto it = data_.find(key);
        if (it == data_.end())
            return false;
        usage_.erase(pos_.at(key));
        pos_.erase(key);
        data_.erase(it);
        return true;
    }

    bool
    contains(Key const& key) const
    {
        auto g = lock();
        return data_.find(key) != data_.end();
    }

    std::size_t
    size() const noexcept
    {
        auto g = lock();
        return data_.size();
    }

    std::size_t
    capacity() const noexcept
    {
        return capacity_;
    }

    bool
    empty() const noexcept
    {
        auto g = lock();
        return data_.empty();
    }

    void
    clear()
    {
        auto g = lock();
        data_.clear();
        pos_.clear();
        usage_.clear();
    }

private:
    Value&
    insertOrpromote(Key const& key)
    {
        if (auto it = data_.find(key); it != data_.end())
        {
            promote(key);
            return it->second;
        }
        evictIfFull();
        usage_.emplace_front(key);
        pos_.emplace(key, usage_.begin());
        auto it = data_.emplace(key, Value{});
        return it.first->second;
    }

    void
    promote(Key const& key)
    {
        auto lit = pos_.at(key);
        usage_.splice(usage_.begin(), usage_, lit);  // O(1)
    }

    void
    evictIfFull()
    {
        if (data_.size() < capacity_)
            return;
        auto const& k = usage_.back();
        data_.erase(k);
        pos_.erase(k);
        usage_.pop_back();
    }

    typename Concurrency::lock_guard
    lock() const
    {
        return typename Concurrency::lock_guard{mtx_};
    }

private:
    std::size_t const capacity_;
    DataMap data_;
    PosMap pos_;
    List usage_;
    mutable typename Concurrency::mutex_type mtx_;
};

}  // namespace ripple

#endif

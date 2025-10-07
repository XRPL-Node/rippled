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

#include <cstddef>
#include <list>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace ripple {

template <
    class Key,
    class Value,
    class Hash = std::hash<Key>,
    class KeyEq = std::equal_to<Key>>
class LRUMap
{
    using List = std::list<Key>;
    using DataMap = std::unordered_map<Key, Value, Hash, KeyEq>;
    using PosMap =
        std::unordered_map<Key, typename List::iterator, Hash, KeyEq>;

public:
    explicit LRUMap(std::size_t capacity) : capacity_(capacity)
    {
        if (!capacity_)
            throw std::invalid_argument(
                "LRUMap capacity must be positive.");  // TODO XRPL_ASSERT
        data_.reserve(capacity_);
        pos_.reserve(capacity_);
    }

    Value&
    operator[](Key const& key)
    {
        if (auto it = data_.find(key); it != data_.end())
        {
            auto lit = pos_.at(key);
            // promote
            usage_.splice(usage_.begin(), usage_, lit);
            return it->second;
        }

        if (data_.size() >= capacity_)
        {
            auto const& lru_key = usage_.back();
            data_.erase(lru_key);
            pos_.erase(lru_key);
            usage_.pop_back();
        }

        usage_.emplace_front(key);
        pos_.emplace(key, usage_.begin());
        auto [it, _] = data_.emplace(key, Value{});
        return it->second;
    }

    Value*
    get(Key const& key)
    {
        auto it = data_.find(key);
        if (it == data_.end())
            return nullptr;
        auto lit = pos_.at(key);
        usage_.splice(usage_.begin(), usage_, lit);
        return &it->second;
    }

    // TODO: remove
    Value const*
    peek(Key const& key) const
    {
        auto it = data_.find(key);
        return it == data_.end() ? nullptr : &it->second;
    }

    using iterator = typename DataMap::iterator;
    using const_iterator = typename DataMap::const_iterator;

    iterator
    find(Key const& key)
    {
        return data_.find(key);
    }

    const_iterator
    find(Key const& key) const
    {
        return data_.find(key);
    }

    iterator
    begin()
    {
        return data_.begin();
    }

    const_iterator
    begin() const
    {
        return data_.begin();
    }

    iterator
    end()
    {
        return data_.end();
    }

    const_iterator
    end() const
    {
        return data_.end();
    }

    bool
    erase(Key const& key)
    {
        auto it = data_.find(key);
        if (it == data_.end())
            return false;
        usage_.erase(pos_.at(key));
        pos_.erase(key);
        data_.erase(it);
        return true;
    }

    template <class... Args>
    Value&
    put(Key const& key, Args&&... args)  // assign/construct + promote
    {
        if (auto it = data_.find(key); it != data_.end())
        {
            it->second = Value(std::forward<Args>(args)...);
            auto lit = pos_.at(key);
            usage_.splice(usage_.begin(), usage_, lit);
            return it->second;
        }
        if (data_.size() >= capacity_)
        {
            auto const& lru_key = usage_.back();
            data_.erase(lru_key);
            pos_.erase(lru_key);
            usage_.pop_back();
        }
        usage_.emplace_front(key);
        pos_.emplace(key, usage_.begin());
        auto [it, _] = data_.emplace(key, Value(std::forward<Args>(args)...));
        return it->second;
    }

    bool
    contains(Key const& key) const
    {
        return data_.find(key) != data_.end();
    }

    std::size_t
    size() const noexcept
    {
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
        return data_.empty();
    }

    void
    clear()
    {
        data_.clear();
        pos_.clear();
        usage_.clear();
    }

private:
    std::size_t const capacity_;
    DataMap data_;  // Key -> Value   (this is what callers iterate/find over)
    PosMap pos_;    // Key -> list position
    List usage_;    // recency order
};

}  // namespace ripple

#endif
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
    WHATSOEVER  RESULTING  FROM  LOSS  OF  USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef RIPPLE_APP_LEDGER_INDEX_MAP_H_INCLUDED
#define RIPPLE_APP_LEDGER_INDEX_MAP_H_INCLUDED

#include <algorithm>
#include <mutex>
#include <queue>
#include <unordered_map>

namespace ripple {

template <class Key, class Mapped>
class LedgerIndexMap
{
public:
    LedgerIndexMap() = default;
    explicit LedgerIndexMap(std::size_t reserve_capacity)
    {
        data_.reserve(reserve_capacity);
    }

    LedgerIndexMap(LedgerIndexMap const&) = delete;
    LedgerIndexMap&
    operator=(LedgerIndexMap const&) = delete;
    LedgerIndexMap(LedgerIndexMap&&) = delete;
    LedgerIndexMap&
    operator=(LedgerIndexMap&&) = delete;

    Mapped&
    operator[](Key const& k)
    {
        std::lock_guard lock(mutex_);
        auto [it, inserted] = data_.try_emplace(k);
        if (inserted)
            order_.push(k);
        return it->second;
    }

    Mapped&
    operator[](Key&& k)
    {
        std::lock_guard lock(mutex_);
        auto [it, inserted] = data_.try_emplace(std::move(k));
        if (inserted)
            order_.push(it->first);
        return it->second;
    }

    [[nodiscard]] Mapped*
    get(Key const& k)
    {
        std::lock_guard lock(mutex_);
        auto it = data_.find(k);
        return it == data_.end() ? nullptr : &it->second;
    }

    [[nodiscard]] Mapped const*
    get(Key const& k) const
    {
        std::lock_guard lock(mutex_);
        auto it = data_.find(k);
        return it == data_.end() ? nullptr : &it->second;
    }

    template <class... Args>
    Mapped&
    put(Key const& k, Args&&... args)
    {
        std::lock_guard lock(mutex_);
        auto [it, inserted] = data_.try_emplace(k, std::forward<Args>(args)...);
        if (!inserted)
            it->second = Mapped(std::forward<Args>(args)...);
        else
            order_.push(k);
        return it->second;
    }

    bool
    contains(Key const& k) const
    {
        std::lock_guard lock(mutex_);
        return data_.find(k) != data_.end();
    }

    std::size_t
    size() const noexcept
    {
        std::lock_guard lock(mutex_);
        return data_.size();
    }

    bool
    empty() const noexcept
    {
        std::lock_guard lock(mutex_);
        return data_.empty();
    }

    void
    reserve(std::size_t n)
    {
        std::lock_guard lock(mutex_);
        data_.reserve(n);
    }

    void
    rehash(std::size_t n)
    {
        std::lock_guard lock(mutex_);
        data_.rehash(n);
    }

    std::size_t
    eraseBefore(Key const& cutoff)
    {
        std::lock_guard lock(mutex_);
        auto const before = data_.size();
        std::erase_if(data_, [&](auto const& kv) { return kv.first < cutoff; });
        return before - data_.size();
    }

private:
    std::unordered_map<Key, Mapped> data_;
    std::queue<Key> order_;  // assumes non-decreasing inserts for O(k) purge
    mutable std::mutex mutex_;
};

}  // namespace ripple

#endif  // RIPPLE_APP_LEDGER_INDEX_MAP_H_INCLUDED

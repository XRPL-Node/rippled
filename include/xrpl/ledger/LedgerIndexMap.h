#pragma once

#include <algorithm>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>

namespace xrpl {

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

    [[nodiscard]] std::optional<Mapped>
    get(Key const& k) const
    {
        std::lock_guard lock(mutex_);
        auto it = data_.find(k);
        return it == data_.end() ? std::nullopt : std::optional<Mapped>{it->second};
    }

    bool
    put(Key const& k, Mapped value)
    {
        std::lock_guard lock(mutex_);
        return data_.insert_or_assign(k, std::move(value)).second;
    }

    bool
    contains(Key const& k) const
    {
        std::lock_guard lock(mutex_);
        return data_.find(k) != data_.end();
    }

    std::size_t
    size() const
    {
        std::lock_guard lock(mutex_);
        return data_.size();
    }

    bool
    empty() const
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
    mutable std::mutex mutex_;
};

}  // namespace xrpl

#ifndef XRPL_APP_LEDGER_INDEX_MAP_H_INCLUDED
#define XRPL_APP_LEDGER_INDEX_MAP_H_INCLUDED

#include <algorithm>
#include <mutex>
#include <unordered_map>

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

    Mapped&
    operator[](Key const& k)
    {
        std::lock_guard lock(mutex_);
        return data_[k];
    }

    Mapped&
    operator[](Key&& k)
    {
        std::lock_guard lock(mutex_);
        return data_[std::move(k)];
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
    mutable std::mutex mutex_;
};

}  // namespace xrpl

#endif  // XRPL_APP_LEDGER_INDEX_MAP_H_INCLUDED

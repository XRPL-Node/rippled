#pragma once

#include <memory>
#include <unordered_map>

namespace xrpl {

namespace detail {

struct LocalValues
{
    explicit LocalValues() = default;

    bool onCoro = true;

    struct BasicValue
    {
        virtual ~BasicValue() = default;
        virtual void*
        get() = 0;
    };

    template <class T>
    struct Value : BasicValue
    {
        T t_;

        Value() = default;
        explicit Value(T const& t) : t_(t)
        {
        }

        void*
        get() override
        {
            return &t_;
        }
    };

    // Keys are the address of a LocalValue.
    std::unordered_map<void const*, std::unique_ptr<BasicValue>> values;
};

// Wrapper to ensure proper cleanup when thread exits
struct LocalValuesHolder
{
    LocalValues* ptr = nullptr;

    ~LocalValuesHolder()
    {
        if (ptr && !ptr->onCoro)
            delete ptr;
    }
};

LocalValuesHolder&
getLocalValuesHolder();

inline LocalValues*&
getLocalValuesPtr()
{
    return getLocalValuesHolder().ptr;
}

inline LocalValues*
getOrCreateLocalValues()
{
    auto& ptr = getLocalValuesPtr();
    if (!ptr)
    {
        ptr = new LocalValues();
        ptr->onCoro = false;
    }
    return ptr;
}

// For coroutine support, we need explicit swap functions
inline LocalValues*
releaseLocalValues()
{
    auto& ptr = getLocalValuesPtr();
    auto* result = ptr;
    ptr = nullptr;
    return result;
}

inline void
resetLocalValues(LocalValues* lvs)
{
    auto& ptr = getLocalValuesPtr();
    // Clean up old value if it's not a coroutine's LocalValues
    if (ptr && !ptr->onCoro)
        delete ptr;
    ptr = lvs;
}

}  // namespace detail

template <class T>
class LocalValue
{
public:
    template <class... Args>
    LocalValue(Args&&... args) : t_(std::forward<Args>(args)...)
    {
    }

    /** Stores instance of T specific to the calling coroutine or thread. */
    T&
    operator*();

    /** Stores instance of T specific to the calling coroutine or thread. */
    T*
    operator->()
    {
        return &**this;
    }

private:
    T t_;
};

template <class T>
T&
LocalValue<T>::operator*()
{
    auto lvs = detail::getOrCreateLocalValues();
    auto const iter = lvs->values.find(this);
    if (iter != lvs->values.end())
        return *reinterpret_cast<T*>(iter->second->get());

    return *reinterpret_cast<T*>(
        lvs->values.emplace(this, std::make_unique<detail::LocalValues::Value<T>>(t_)).first->second->get());
}
}  // namespace xrpl

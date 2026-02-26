#pragma once

#include <coroutine>
#include <exception>
#include <utility>
#include <variant>

namespace xrpl {

template <typename T = void>
class CoroTask;

// --------------------------------------------------------------------------
// CoroTask<void> — coroutine return type for void-returning coroutines
// --------------------------------------------------------------------------

template <>
class CoroTask<void>
{
public:
    struct promise_type;
    using Handle = std::coroutine_handle<promise_type>;

    struct promise_type
    {
        std::exception_ptr exception_;
        std::coroutine_handle<> continuation_;

        CoroTask
        get_return_object()
        {
            return CoroTask{Handle::from_promise(*this)};
        }

        std::suspend_always
        initial_suspend() noexcept
        {
            return {};
        }

        struct FinalAwaiter
        {
            bool
            await_ready() noexcept
            {
                return false;
            }

            std::coroutine_handle<>
            await_suspend(Handle h) noexcept
            {
                if (auto cont = h.promise().continuation_)
                    return cont;
                return std::noop_coroutine();
            }

            void
            await_resume() noexcept
            {
            }
        };

        FinalAwaiter
        final_suspend() noexcept
        {
            return {};
        }

        void
        return_void()
        {
        }

        void
        unhandled_exception()
        {
            exception_ = std::current_exception();
        }
    };

    CoroTask() = default;

    explicit CoroTask(Handle h) : handle_(h)
    {
    }

    ~CoroTask()
    {
        if (handle_)
            handle_.destroy();
    }

    CoroTask(CoroTask&& other) noexcept : handle_(std::exchange(other.handle_, {}))
    {
    }

    CoroTask&
    operator=(CoroTask&& other) noexcept
    {
        if (this != &other)
        {
            if (handle_)
                handle_.destroy();
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }

    CoroTask(CoroTask const&) = delete;
    CoroTask&
    operator=(CoroTask const&) = delete;

    Handle
    handle() const
    {
        return handle_;
    }

    bool
    done() const
    {
        return handle_ && handle_.done();
    }

    // Awaiter interface — allows co_await on a CoroTask
    bool
    await_ready() const noexcept
    {
        return false;
    }

    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> caller) noexcept
    {
        handle_.promise().continuation_ = caller;
        return handle_;  // Symmetric transfer
    }

    void
    await_resume()
    {
        if (auto& ep = handle_.promise().exception_)
            std::rethrow_exception(ep);
    }

private:
    Handle handle_;
};

// --------------------------------------------------------------------------
// CoroTask<T> — coroutine return type for value-returning coroutines
// --------------------------------------------------------------------------

template <typename T>
class CoroTask
{
public:
    struct promise_type;
    using Handle = std::coroutine_handle<promise_type>;

    struct promise_type
    {
        std::variant<std::monostate, T, std::exception_ptr> result_;
        std::coroutine_handle<> continuation_;

        CoroTask
        get_return_object()
        {
            return CoroTask{Handle::from_promise(*this)};
        }

        std::suspend_always
        initial_suspend() noexcept
        {
            return {};
        }

        struct FinalAwaiter
        {
            bool
            await_ready() noexcept
            {
                return false;
            }

            std::coroutine_handle<>
            await_suspend(Handle h) noexcept
            {
                if (auto cont = h.promise().continuation_)
                    return cont;
                return std::noop_coroutine();
            }

            void
            await_resume() noexcept
            {
            }
        };

        FinalAwaiter
        final_suspend() noexcept
        {
            return {};
        }

        void
        return_value(T value)
        {
            result_.template emplace<1>(std::move(value));
        }

        void
        unhandled_exception()
        {
            result_.template emplace<2>(std::current_exception());
        }
    };

    CoroTask() = default;

    explicit CoroTask(Handle h) : handle_(h)
    {
    }

    ~CoroTask()
    {
        if (handle_)
            handle_.destroy();
    }

    CoroTask(CoroTask&& other) noexcept : handle_(std::exchange(other.handle_, {}))
    {
    }

    CoroTask&
    operator=(CoroTask&& other) noexcept
    {
        if (this != &other)
        {
            if (handle_)
                handle_.destroy();
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }

    CoroTask(CoroTask const&) = delete;
    CoroTask&
    operator=(CoroTask const&) = delete;

    Handle
    handle() const
    {
        return handle_;
    }

    bool
    done() const
    {
        return handle_ && handle_.done();
    }

    bool
    await_ready() const noexcept
    {
        return false;
    }

    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> caller) noexcept
    {
        handle_.promise().continuation_ = caller;
        return handle_;
    }

    T
    await_resume()
    {
        auto& result = handle_.promise().result_;
        if (auto* ep = std::get_if<2>(&result))
            std::rethrow_exception(*ep);
        return std::get<1>(std::move(result));
    }

private:
    Handle handle_;
};

}  // namespace xrpl

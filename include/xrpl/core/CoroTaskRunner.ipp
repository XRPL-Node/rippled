#pragma once

namespace xrpl {

inline JobQueue::CoroTaskRunner::CoroTaskRunner(
    create_t,
    JobQueue& jq,
    JobType type,
    std::string const& name)
    : jq_(jq), type_(type), name_(name), running_(false)
{
}

template <class F>
void
JobQueue::CoroTaskRunner::init(F&& f)
{
    // Store the callable on the heap so it outlives the coroutine frame.
    // Coroutine frames store a reference to the callable's implicit object
    // parameter (the lambda). If the callable is a temporary, that reference
    // dangles after the caller returns. Keeping the callable alive here
    // ensures the coroutine's captures remain valid.
    using Fn = std::decay_t<F>;
    auto store = std::make_unique<FuncStore<Fn>>(std::forward<F>(f));
    task_ = store->func(shared_from_this());
    storedFunc_ = std::move(store);
}

inline JobQueue::CoroTaskRunner::~CoroTaskRunner()
{
#ifndef NDEBUG
    XRPL_ASSERT(finished_, "xrpl::JobQueue::CoroTaskRunner::~CoroTaskRunner : is finished");
#endif
}

inline void
JobQueue::CoroTaskRunner::onSuspend()
{
    std::lock_guard lock(jq_.m_mutex);
    ++jq_.nSuspend_;
}

inline void
JobQueue::CoroTaskRunner::onUndoSuspend()
{
    std::lock_guard lock(jq_.m_mutex);
    --jq_.nSuspend_;
}

inline auto
JobQueue::CoroTaskRunner::suspend()
{
    struct SuspendAwaiter
    {
        CoroTaskRunner& runner_;

        bool
        await_ready() const noexcept
        {
            return false;
        }

        void
        await_suspend(std::coroutine_handle<>) const
        {
            runner_.onSuspend();
        }

        void
        await_resume() const noexcept
        {
        }
    };
    return SuspendAwaiter{*this};
}

inline bool
JobQueue::CoroTaskRunner::post()
{
    {
        std::lock_guard lk(mutex_run_);
        running_ = true;
    }

    // sp prevents 'this' from being destroyed while the job is pending
    if (jq_.addJob(type_, name_, [this, sp = shared_from_this()]() { resume(); }))
    {
        return true;
    }

    // The coroutine will not run. Clean up running_.
    std::lock_guard lk(mutex_run_);
    running_ = false;
    cv_.notify_all();
    return false;
}

inline void
JobQueue::CoroTaskRunner::resume()
{
    {
        std::lock_guard lk(mutex_run_);
        running_ = true;
    }
    {
        std::lock_guard lock(jq_.m_mutex);
        --jq_.nSuspend_;
    }
    auto saved = detail::getLocalValues().release();
    detail::getLocalValues().reset(&lvs_);
    std::lock_guard lock(mutex_);
    XRPL_ASSERT(!task_.done(), "xrpl::JobQueue::CoroTaskRunner::resume : task is not done");
    task_.handle().resume();
    detail::getLocalValues().release();
    detail::getLocalValues().reset(saved);
#ifndef NDEBUG
    if (task_.done())
        finished_ = true;
#endif
    std::lock_guard lk(mutex_run_);
    running_ = false;
    cv_.notify_all();
}

inline bool
JobQueue::CoroTaskRunner::runnable() const
{
    return !task_.done();
}

inline void
JobQueue::CoroTaskRunner::expectEarlyExit()
{
#ifndef NDEBUG
    if (!finished_)
#endif
    {
        std::lock_guard lock(jq_.m_mutex);
        --jq_.nSuspend_;
#ifndef NDEBUG
        finished_ = true;
#endif
    }
    // Destroy the coroutine frame to break a potential shared_ptr cycle.
    // The coroutine is at initial_suspend and never ran user code, so
    // destroying it is safe. Without this, the frame holds a shared_ptr
    // back to this CoroTaskRunner, creating an unreachable reference cycle.
    task_ = {};
}

inline void
JobQueue::CoroTaskRunner::join()
{
    std::unique_lock<std::mutex> lk(mutex_run_);
    cv_.wait(lk, [this]() { return running_ == false; });
}

}  // namespace xrpl

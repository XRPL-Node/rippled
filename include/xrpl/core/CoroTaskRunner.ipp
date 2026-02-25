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
    task_ = std::forward<F>(f)(shared_from_this());
}

inline JobQueue::CoroTaskRunner::~CoroTaskRunner()
{
#ifndef NDEBUG
    XRPL_ASSERT(
        finished_,
        "xrpl::JobQueue::CoroTaskRunner::~CoroTaskRunner : is finished");
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
    if (jq_.addJob(
            type_, name_, [this, sp = shared_from_this()]() { resume(); }))
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
    XRPL_ASSERT(
        !task_.done(),
        "xrpl::JobQueue::CoroTaskRunner::resume : task is not done");
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
}

inline void
JobQueue::CoroTaskRunner::join()
{
    std::unique_lock<std::mutex> lk(mutex_run_);
    cv_.wait(lk, [this]() { return running_ == false; });
}

}  // namespace xrpl

#pragma once

#include <xrpl/basics/ByteUtilities.h>

namespace xrpl {

// Coroutine stack size is set to 2MB to provide sufficient headroom for
// deep call stacks in RPC operations. With 1MB, stack exhaustion occurred
// in production scenarios, particularly in:
// - RPC handlers processing complex JSON (ServerHandler::processRequest)
// - Transaction validation with deep parsing (TransactionSign::getCurrentNetworkFee)
// - Amount parsing with boost::split operations (amountFromJson)
// The 2MB stack provides ~50% safety margin even for the deepest observed
// call chains while keeping memory overhead reasonable (~2MB per coroutine).
template <class F>
JobQueue::Coro::Coro(Coro_create_t, JobQueue& jq, JobType type, std::string const& name, F&& f)
    : jq_(jq)
    , type_(type)
    , name_(name)
    , running_(false)
    , coro_([this, fn = std::forward<F>(f)](boost::coroutines2::asymmetric_coroutine<void>::push_type& do_yield) {
        yield_ = &do_yield;
        yield();
        fn(shared_from_this());
#ifndef NDEBUG
        finished_ = true;
#endif
    })
{
}

inline JobQueue::Coro::~Coro()
{
#ifndef NDEBUG
    XRPL_ASSERT(finished_, "xrpl::JobQueue::Coro::~Coro : is finished");
#endif
}

inline void
JobQueue::Coro::yield() const
{
    {
        std::lock_guard lock(jq_.m_mutex);
        ++jq_.nSuspend_;
    }
    (*yield_)();
}

inline bool
JobQueue::Coro::post()
{
    {
        std::lock_guard lk(mutex_run_);
        running_ = true;
    }

    // sp keeps 'this' alive
    if (jq_.addJob(type_, name_, [this, sp = shared_from_this()]() { resume(); }))
    {
        return true;
    }

    // The coroutine will not run.  Clean up running_.
    std::lock_guard lk(mutex_run_);
    running_ = false;
    cv_.notify_all();
    return false;
}

inline void
JobQueue::Coro::resume()
{
    {
        std::lock_guard lk(mutex_run_);
        running_ = true;
    }
    {
        std::lock_guard lock(jq_.m_mutex);
        --jq_.nSuspend_;
    }
    auto saved = detail::releaseLocalValues();
    detail::resetLocalValues(&lvs_);
    std::lock_guard lock(mutex_);
    XRPL_ASSERT(static_cast<bool>(coro_), "xrpl::JobQueue::Coro::resume : is runnable");
    coro_();

    // Restore the thread's original LocalValues
    detail::releaseLocalValues();
    detail::resetLocalValues(saved);

    std::lock_guard lk(mutex_run_);
    running_ = false;
    cv_.notify_all();
}

inline bool
JobQueue::Coro::runnable() const
{
    return static_cast<bool>(coro_);
}

inline void
JobQueue::Coro::expectEarlyExit()
{
#ifndef NDEBUG
    if (!finished_)
#endif
    {
        // expectEarlyExit() must only ever be called from outside the
        // Coro's stack.  It you're inside the stack you can simply return
        // and be done.
        //
        // That said, since we're outside the Coro's stack, we need to
        // decrement the nSuspend that the Coro's call to yield caused.
        std::lock_guard lock(jq_.m_mutex);
        --jq_.nSuspend_;
#ifndef NDEBUG
        finished_ = true;
#endif
    }
}

inline void
JobQueue::Coro::join()
{
    std::unique_lock<std::mutex> lk(mutex_run_);
    cv_.wait(lk, [this]() { return running_ == false; });
}

}  // namespace xrpl

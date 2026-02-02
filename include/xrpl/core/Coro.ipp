#ifndef XRPL_CORE_COROINL_H_INCLUDED
#define XRPL_CORE_COROINL_H_INCLUDED

#include <xrpl/basics/ByteUtilities.h>

#include <mutex>

namespace xrpl {

template <class F>
JobQueue::Coro::Coro(Coro_create_t, JobQueue& jq, JobType type, std::string const& name, F&& f)
    : jq_(jq)
    , type_(type)
    , name_(name)
    , coro_(
          [this, fn = std::forward<F>(f)](boost::coroutines::asymmetric_coroutine<int>::push_type& do_yield) {
              state_ = CoroState::Running;
              yield_ = &do_yield;
              try
              {
                  yield();
                  // self makes Coro alive until this function returns
                  std::shared_ptr<Coro> self;
                  self = shared_from_this();
                  fn(self);
                  state_ = CoroState::Finished;
              }
              catch (...)
              {
                  state_ = CoroState::Finished;
                  throw;
              }
          },
          boost::coroutines::attributes(megabytes(1)))
{
}

inline JobQueue::Coro::~Coro()
{
    cancel();
    XRPL_ASSERT(state_ != CoroState::Running, "xrpl::JobQueue::Coro::~Coro : is not running");
}

inline void
JobQueue::Coro::yield()
{
    CoroState expected = CoroState::Running;
    if (!state_.compare_exchange_strong(expected, CoroState::Suspended))
    {
        return;
    }

    {
        std::lock_guard lock(jq_.m_mutex);

        ++jq_.nSuspend_;
        jq_.cv_.notify_all();
        jq_.m_suspendedCoros[this] = weak_from_this();
    }

    state_.notify_all();
    (*yield_)(++yieldCount_);
}

inline bool
JobQueue::Coro::post()
{
    if (state_ != CoroState::Suspended)
    {
        // The coroutine will run until it finishes if the JobQueue has stopped.
        // In the case where make_shared<Coro>() succeeds and then the JobQueue
        // stops before coro_ gets executed, post() will still be called and
        // state_ will be Finished. We should return false and avoid XRPL_ASSERT
        // as it's a valid edge case.
        return false;
    }

    // sp keeps 'this' alive
    if (jq_.addJob(type_, name_, [this, sp = shared_from_this()]() { resume(); }))
    {
        return true;
    }

    return false;
}

inline void
JobQueue::Coro::resume()
{
    auto suspended = CoroState::Suspended;
    if (!state_.compare_exchange_strong(suspended, CoroState::Running))
    {
        return;
    }

    state_.notify_all();

    // There's a small chance that the coroutine has not yielded yet and is
    // still running. We need to wait for it to yield before we can resume it.
    waitForYield();

    {
        std::lock_guard lock(jq_.m_mutex);
        jq_.m_suspendedCoros.erase(this);
        --jq_.nSuspend_;
        jq_.cv_.notify_all();
    }

    auto saved = detail::getLocalValues().release();
    detail::getLocalValues().reset(&lvs_);
    XRPL_ASSERT(static_cast<bool>(coro_), "xrpl::JobQueue::Coro::resume : is runnable");
    coro_();
    detail::getLocalValues().release();
    detail::getLocalValues().reset(saved);
}

inline bool
JobQueue::Coro::runnable() const
{
    // There's an edge case where the coroutine has updated the status
    // to Finished but the function hasn't exited and therefore, coro_ is
    // still valid. However, the coroutine is not technically runnable in this
    // case, because the coroutine is about to exit and static_cast<bool>(coro_)
    // is going to be false.
    return static_cast<bool>(coro_) && state_ != CoroState::Finished;
}

inline void
JobQueue::Coro::join()
{
    state_.wait(CoroState::Running);
}

inline void
JobQueue::Coro::cancel()
{
    auto suspended = CoroState::Suspended;
    if (!state_.compare_exchange_strong(suspended, CoroState::Running))
    {
        return;
    }

    waitForYield();

    coro_ = {};

    {
        std::lock_guard lock(jq_.m_mutex);
        jq_.m_suspendedCoros.erase(this);
        --jq_.nSuspend_;
        jq_.cv_.notify_all();
    }

    XRPL_ASSERT(state_ == CoroState::Finished, "ripple::JobQueue::Coro::cancel : should have finished");
}

inline void
JobQueue::Coro::waitForYield() const
{
    // Busy-wait for the coroutine to yield so that it's safe to destroy it.
    while (yieldCount_ != coro_.get())
        ;
}

}  // namespace xrpl

#endif

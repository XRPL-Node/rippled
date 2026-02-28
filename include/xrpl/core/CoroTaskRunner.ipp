#pragma once

/**
 * @file CoroTaskRunner.ipp
 *
 * CoroTaskRunner inline implementation.
 *
 * This file contains the business logic for managing C++20 coroutines
 * on the JobQueue. It is included at the bottom of JobQueue.h.
 *
 * Data Flow: suspend / post / resume cycle
 * =========================================
 *
 *         coroutine body                 CoroTaskRunner              JobQueue
 *         --------------                 --------------              --------
 *              |
 *    co_await runner->suspend()
 *              |
 *              +--- await_suspend ------> onSuspend()
 *              |                            ++nSuspend_ ------------> nSuspend_
 *              |                          [coroutine is now suspended]
 *              |
 *              .    (externally or by JobQueueAwaiter)
 *              .
 *              +--- (caller calls) -----> post()
 *              |                            running_ = true
 *              |                            addJob(resume) ----------> job enqueued
 *              |                                                        |
 *              |                                                      [worker picks up]
 *              |                                                        |
 *              +--- <----- resume() <-----------------------------------+
 *              |             --nSuspend_ ------> nSuspend_
 *              |             swap in LocalValues (lvs_)
 *              |             task_.handle().resume()
 *              |                |
 *              |    [coroutine body continues here]
 *              |                |
 *              |             swap out LocalValues
 *              |             running_ = false
 *              |             cv_.notify_all()
 *              v
 *
 * Thread Safety
 * =============
 * - mutex_     : guards task_.handle().resume() so that post()-before-suspend
 *                races cannot resume the coroutine while it is still running.
 *                (See the race condition discussion in JobQueue.h)
 * - mutex_run_ : guards running_ flag; used by join() to wait for completion.
 * - jq_.m_mutex: guards nSuspend_ increments/decrements.
 *
 * Common Mistakes When Modifying This File
 * =========================================
 *
 * 1. Changing lock ordering.
 *    resume() acquires locks in this order: mutex_run_ -> jq_.m_mutex -> mutex_.
 *    Acquiring them in a different order WILL deadlock. Any new code path
 *    that touches these mutexes must follow the same order.
 *
 * 2. Removing the shared_from_this() capture in post().
 *    The lambda passed to addJob captures [this, sp = shared_from_this()].
 *    If you remove sp, 'this' can be destroyed before the job runs,
 *    causing use-after-free. The sp capture is load-bearing.
 *
 * 3. Forgetting to decrement nSuspend_ on a new code path.
 *    Every ++nSuspend_ must have a matching --nSuspend_. If you add a new
 *    suspension path (e.g. a new awaiter) and forget to decrement on resume
 *    or on failure, JobQueue::stop() will hang.
 *
 * 4. Calling task_.handle().resume() without holding mutex_.
 *    This allows a race where the coroutine runs on two threads
 *    simultaneously. Always hold mutex_ around resume().
 *
 * 5. Swapping LocalValues outside of the mutex_ critical section.
 *    The swap-in and swap-out of LocalValues must bracket the resume()
 *    call. If you move the swap-out before the lock_guard(mutex_) is
 *    released, you break LocalValue isolation for any code that runs
 *    after the coroutine suspends but before the lock is dropped.
 */
//

namespace xrpl {

/**
 * Construct a CoroTaskRunner. Sets running_ to false; does not
 * create the coroutine. Call init() afterwards.
 *
 * @param jq   The JobQueue this coroutine will run on
 * @param type Job type for scheduling priority
 * @param name Human-readable name for logging
 */
inline JobQueue::CoroTaskRunner::CoroTaskRunner(
    create_t,
    JobQueue& jq,
    JobType type,
    std::string const& name)
    : jq_(jq), type_(type), name_(name), running_(false)
{
}

/**
 * Initialize with a coroutine-returning callable.
 * Stores the callable on the heap (FuncStore) so it outlives the
 * coroutine frame. Coroutine frames store a reference to the
 * callable's implicit object parameter (the lambda). If the callable
 * is a temporary, that reference dangles after the caller returns.
 * Keeping the callable alive here ensures the coroutine's captures
 * remain valid.
 *
 * @param f Callable: CoroTask<void>(shared_ptr<CoroTaskRunner>)
 */
template <class F>
void
JobQueue::CoroTaskRunner::init(F&& f)
{
    using Fn = std::decay_t<F>;
    auto store = std::make_unique<FuncStore<Fn>>(std::forward<F>(f));
    task_ = store->func(shared_from_this());
    storedFunc_ = std::move(store);
}

/**
 * Destructor. Waits for any in-flight resume() to complete, then
 * asserts (debug) that the coroutine has finished or
 * expectEarlyExit() was called.
 *
 * The join() call is necessary because with async dispatch the
 * coroutine runs on a worker thread. The gate signal (which wakes
 * the test thread) can arrive before resume() has set finished_.
 * join() synchronizes via mutex_run_, establishing a happens-before
 * edge: finished_ = true → unlock(mutex_run_) in resume() →
 * lock(mutex_run_) in join() → read finished_.
 */
inline JobQueue::CoroTaskRunner::~CoroTaskRunner()
{
#ifndef NDEBUG
    join();
    XRPL_ASSERT(finished_, "xrpl::JobQueue::CoroTaskRunner::~CoroTaskRunner : is finished");
#endif
}

/**
 * Increment the JobQueue's suspended-coroutine count (nSuspend_).
 */
inline void
JobQueue::CoroTaskRunner::onSuspend()
{
    std::lock_guard lock(jq_.m_mutex);
    ++jq_.nSuspend_;
}

/**
 * Decrement nSuspend_ without resuming.
 */
inline void
JobQueue::CoroTaskRunner::onUndoSuspend()
{
    std::lock_guard lock(jq_.m_mutex);
    --jq_.nSuspend_;
}

/**
 * Return a SuspendAwaiter whose await_suspend() increments nSuspend_
 * before the coroutine actually suspends. The caller must later call
 * post() or resume() to continue execution.
 *
 * @return Awaiter for use with `co_await runner->suspend()`
 */
inline auto
JobQueue::CoroTaskRunner::suspend()
{
    /**
     * Custom awaiter for suspend(). Always suspends (await_ready
     * returns false) and increments nSuspend_ in await_suspend().
     */
    struct SuspendAwaiter
    {
        CoroTaskRunner& runner_;  // The runner that owns this coroutine.

        /**
         * Always returns false so the coroutine suspends.
         */
        bool
        await_ready() const noexcept
        {
            return false;
        }

        /**
         * Called when the coroutine suspends. Increments nSuspend_
         * so the JobQueue knows a coroutine is waiting.
         */
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

/**
 * Schedule coroutine resumption as a job on the JobQueue.
 * A shared_ptr capture (sp) prevents this CoroTaskRunner from being
 * destroyed while the job is queued but not yet executed.
 *
 * @return false if the JobQueue rejected the job (shutting down)
 */
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

/**
 * Resume the coroutine on the current thread.
 *
 * Steps:
 *   1. Set running_ = true (under mutex_run_)
 *   2. Decrement nSuspend_ (under jq_.m_mutex)
 *   3. Swap in this coroutine's LocalValues for thread-local isolation
 *   4. Resume the coroutine handle (under mutex_)
 *   5. Swap out LocalValues, restoring the thread's previous state
 *   6. Set running_ = false and notify join() waiters
 */
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
    if (task_.done())
    {
#ifndef NDEBUG
        finished_ = true;
#endif
        // Break the shared_ptr cycle: frame -> shared_ptr<runner> -> this.
        // Use std::move (not task_ = {}) so task_.handle_ is null BEFORE the
        // frame is destroyed. operator= would destroy the frame while handle_
        // still holds the old value -- a re-entrancy hazard on GCC-12 if
        // frame destruction triggers runner cleanup.
        [[maybe_unused]] auto completed = std::move(task_);
    }
    std::lock_guard lk(mutex_run_);
    running_ = false;
    cv_.notify_all();
}

/**
 * @return true if the coroutine has not yet run to completion
 */
inline bool
JobQueue::CoroTaskRunner::runnable() const
{
    // After normal completion, task_ is reset to break the shared_ptr cycle
    // (handle_ becomes null). A null handle means the coroutine is done.
    return task_.handle() && !task_.done();
}

/**
 * Handle early termination when the coroutine never ran (e.g. JobQueue
 * is stopping). Decrements nSuspend_ and destroys the coroutine frame
 * to break the shared_ptr cycle: frame -> lambda -> runner -> frame.
 */
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
    // Break the shared_ptr cycle: frame -> shared_ptr<runner> -> this.
    // The coroutine is at initial_suspend and never ran user code, so
    // destroying it is safe. Use std::move (not task_ = {}) so
    // task_.handle_ is null before the frame is destroyed.
    {
        [[maybe_unused]] auto completed = std::move(task_);
    }
    storedFunc_.reset();
}

/**
 * Block until the coroutine finishes its current execution slice.
 * Uses cv_ + mutex_run_ to wait until running_ == false.
 */
inline void
JobQueue::CoroTaskRunner::join()
{
    std::unique_lock<std::mutex> lk(mutex_run_);
    cv_.wait(lk, [this]() { return running_ == false; });
}

}  // namespace xrpl

#pragma once

#include <xrpl/core/JobQueue.h>

#include <coroutine>
#include <memory>

namespace xrpl {

/** Awaiter that suspends and immediately reschedules on the JobQueue.
    Equivalent to calling yield() followed by post() in the old Coro API.

    Usage:
        co_await JobQueueAwaiter{runner};

    What it waits for: The coroutine is re-queued as a job and resumes
    when a worker thread picks it up.

    Which thread resumes: A JobQueue worker thread.

    What await_resume() returns: void.
*/
struct JobQueueAwaiter
{
    std::shared_ptr<JobQueue::CoroTaskRunner> runner;

    bool
    await_ready() const noexcept
    {
        return false;
    }

    bool
    await_suspend(std::coroutine_handle<>)
    {
        // Increment nSuspend (equivalent to yield())
        runner->onSuspend();
        // Schedule resume on JobQueue (equivalent to post())
        if (!runner->post())
        {
            // JobQueue is stopping. Undo the suspend count and
            // don't actually suspend — the coroutine continues
            // immediately so it can clean up and co_return.
            runner->onUndoSuspend();
            return false;
        }
        return true;
    }

    void
    await_resume() const noexcept
    {
    }
};

}  // namespace xrpl

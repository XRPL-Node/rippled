#pragma once

#include <xrpl/core/JobQueue.h>

#include <coroutine>
#include <memory>

namespace xrpl {

/**
 * Awaiter that suspends and immediately reschedules on the JobQueue.
 * Equivalent to calling yield() followed by post() in the old Coro API.
 *
 * Usage:
 *     co_await JobQueueAwaiter{runner};
 *
 * What it waits for: The coroutine is re-queued as a job and resumes
 * when a worker thread picks it up.
 *
 * Which thread resumes: A JobQueue worker thread.
 *
 * What await_resume() returns: void.
 *
 * Dependency Diagram
 * ==================
 *
 *   JobQueueAwaiter
 *   +----------------------------------------------+
 *   | + runner : shared_ptr<CoroTaskRunner>         |
 *   +----------------------------------------------+
 *   | + await_ready()   -> false (always suspend)   |
 *   | + await_suspend() -> bool (suspend or cancel) |
 *   | + await_resume()  -> void                     |
 *   +----------------------------------------------+
 *          |                         |
 *          | uses                    | uses
 *          v                         v
 *   CoroTaskRunner              JobQueue
 *   .onSuspend()                (via runner->post() -> addJob)
 *   .onUndoSuspend()
 *   .post()
 *
 * Control Flow (await_suspend)
 * ============================
 *
 *   co_await JobQueueAwaiter{runner}
 *     |
 *     +-- await_ready() -> false
 *     +-- await_suspend(handle)
 *           |
 *           +-- runner->onSuspend()     // ++nSuspend_
 *           +-- runner->post()          // addJob to JobQueue
 *           |     |
 *           |     +-- success? return true   // coroutine stays suspended
 *           |     |                          // worker thread will call resume()
 *           |     +-- failure? (JQ stopping)
 *           |           +-- runner->onUndoSuspend()  // --nSuspend_
 *           |           +-- return false   // coroutine continues immediately
 *           |                              // so it can clean up and co_return
 *
 * Usage Examples
 * ==============
 *
 * 1. Yield and auto-repost (most common -- replaces yield() + post()):
 *
 *     CoroTask<void> handler(auto runner) {
 *         doPartA();
 *         co_await JobQueueAwaiter{runner};  // yield + repost
 *         doPartB();                         // runs on a worker thread
 *         co_return;
 *     }
 *
 * 2. Multiple yield points in a loop:
 *
 *     CoroTask<void> batchProcessor(auto runner) {
 *         for (auto& item : items) {
 *             process(item);
 *             co_await JobQueueAwaiter{runner};  // let other jobs run
 *         }
 *         co_return;
 *     }
 *
 * 3. Graceful shutdown -- checking after resume:
 *
 *     CoroTask<void> longTask(auto runner, JobQueue& jq) {
 *         while (hasWork()) {
 *             co_await JobQueueAwaiter{runner};
 *             // If JQ is stopping, await_suspend returns false and
 *             // the coroutine continues immediately without re-queuing.
 *             // Always check isStopping() to decide whether to proceed:
 *             if (jq.isStopping())
 *                 co_return;
 *             doNextChunk();
 *         }
 *         co_return;
 *     }
 *
 * Caveats / Pitfalls
 * ==================
 *
 * BUG-RISK: Using a stale or null runner.
 *   The runner shared_ptr must be valid and point to the CoroTaskRunner
 *   that owns the coroutine currently executing. Passing a runner from
 *   a different coroutine, or a default-constructed shared_ptr, is UB.
 *
 * BUG-RISK: Assuming resume happens on the same thread.
 *   After co_await JobQueueAwaiter, the coroutine resumes on whatever
 *   worker thread picks up the job. Do not rely on thread-local state
 *   unless it is managed through LocalValue (which CoroTaskRunner
 *   automatically swaps in/out).
 *
 * BUG-RISK: Ignoring the shutdown path.
 *   When the JobQueue is stopping, post() fails and await_suspend()
 *   returns false (coroutine does NOT actually suspend). The coroutine
 *   body continues immediately on the same thread. If your code after
 *   co_await assumes it was re-queued and is running on a worker thread,
 *   that assumption breaks during shutdown. Always handle the "JQ is
 *   stopping" case, either by checking jq.isStopping() or by letting
 *   the coroutine fall through to co_return naturally.
 *
 * DIFFERENCE from runner->suspend() + runner->post():
 *   JobQueueAwaiter combines both in one atomic operation. With the
 *   manual suspend()/post() pattern, there is a window between the
 *   two calls where an external event could race. JobQueueAwaiter
 *   removes that window -- onSuspend() and post() happen within the
 *   same await_suspend() call while the coroutine is guaranteed to
 *   be suspended. Prefer JobQueueAwaiter unless you need an external
 *   party to decide *when* to call post().
 */
struct JobQueueAwaiter
{
    // The CoroTaskRunner that owns the currently executing coroutine.
    std::shared_ptr<JobQueue::CoroTaskRunner> runner;

    /**
     * Always returns false so the coroutine suspends.
     */
    bool
    await_ready() const noexcept
    {
        return false;
    }

    /**
     * Increment nSuspend (equivalent to yield()) and schedule resume
     * on the JobQueue (equivalent to post()). If the JobQueue is
     * stopping, undoes the suspend count and returns false so the
     * coroutine continues immediately and can clean up.
     *
     * @return true if coroutine should stay suspended (job posted);
     *         false if coroutine should continue (JQ stopping)
     */
    bool
    await_suspend(std::coroutine_handle<>)
    {
        runner->onSuspend();
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

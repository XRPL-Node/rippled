#include <xrpl/core/PerfLog.h>
#include <xrpl/core/detail/Workers.h>
#include <xrpl/json/json_value.h>

#include <doctest/doctest.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

namespace xrpl {

TEST_SUITE_BEGIN("Workers");

namespace {

/**
 * Dummy class for unit tests.
 */
class PerfLogTest : public perf::PerfLog
{
    void
    rpcStart(std::string const& method, std::uint64_t requestId) override
    {
    }

    void
    rpcFinish(std::string const& method, std::uint64_t requestId) override
    {
    }

    void
    rpcError(std::string const& method, std::uint64_t dur) override
    {
    }

    void
    jobQueue(JobType const type) override
    {
    }

    void
    jobStart(
        JobType const type,
        std::chrono::microseconds dur,
        std::chrono::time_point<std::chrono::steady_clock> startTime,
        int instance) override
    {
    }

    void
    jobFinish(JobType const type, std::chrono::microseconds dur, int instance)
        override
    {
    }

    Json::Value
    countersJson() const override
    {
        return Json::Value();
    }

    Json::Value
    currentJson() const override
    {
        return Json::Value();
    }

    void
    resizeJobs(int const resize) override
    {
    }

    void
    rotate() override
    {
    }
};

struct TestCallback : Workers::Callback
{
    void
    processTask(int instance) override
    {
        std::lock_guard lk{mut};
        if (--count == 0)
            cv.notify_all();
    }

    std::condition_variable cv;
    std::mutex mut;
    int count = 0;
};

void
testThreads(int const tc1, int const tc2, int const tc3)
{
    TestCallback cb;
    std::unique_ptr<perf::PerfLog> perfLog = std::make_unique<PerfLogTest>();

    Workers w(cb, perfLog.get(), "Test", tc1);
    CHECK(w.getNumberOfThreads() == tc1);

    auto testForThreadCount = [&cb, &w](int const threadCount) {
        // Prepare the callback.
        cb.count = threadCount;

        // Execute the test.
        w.setNumberOfThreads(threadCount);
        CHECK(w.getNumberOfThreads() == threadCount);

        for (int i = 0; i < threadCount; ++i)
            w.addTask();

        // 10 seconds should be enough to finish on any system
        using namespace std::chrono_literals;
        std::unique_lock<std::mutex> lk{cb.mut};
        bool const signaled =
            cb.cv.wait_for(lk, 10s, [&cb] { return cb.count == 0; });
        CHECK(signaled);
        CHECK(cb.count == 0);
    };
    testForThreadCount(tc1);
    testForThreadCount(tc2);
    testForThreadCount(tc3);
    w.stop();

    // We had better finished all our work!
    CHECK(cb.count == 0);
}

}  // namespace

TEST_CASE("threadCounts: 0 -> 0 -> 0")
{
    testThreads(0, 0, 0);
}

TEST_CASE("threadCounts: 1 -> 0 -> 1")
{
    testThreads(1, 0, 1);
}

TEST_CASE("threadCounts: 2 -> 1 -> 2")
{
    testThreads(2, 1, 2);
}

TEST_CASE("threadCounts: 4 -> 3 -> 5")
{
    testThreads(4, 3, 5);
}

TEST_CASE("threadCounts: 16 -> 4 -> 15")
{
    testThreads(16, 4, 15);
}

TEST_CASE("threadCounts: 64 -> 3 -> 65")
{
    testThreads(64, 3, 65);
}

TEST_SUITE_END();

}  // namespace xrpl


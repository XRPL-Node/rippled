#include <test/jtx.h>

#include <xrpl/core/JobQueue.h>
#include <xrpl/core/JobQueueAwaiter.h>

#include <chrono>
#include <mutex>

namespace xrpl {
namespace test {

class CoroTask_test : public beast::unit_test::suite
{
public:
    class gate
    {
    private:
        std::condition_variable cv_;
        std::mutex mutex_;
        bool signaled_ = false;

    public:
        template <class Rep, class Period>
        bool
        wait_for(std::chrono::duration<Rep, Period> const& rel_time)
        {
            std::unique_lock<std::mutex> lk(mutex_);
            auto b = cv_.wait_for(lk, rel_time, [this] { return signaled_; });
            signaled_ = false;
            return b;
        }

        void
        signal()
        {
            std::lock_guard lk(mutex_);
            signaled_ = true;
            cv_.notify_all();
        }
    };

    // NOTE: All coroutine lambdas passed to postCoroTask use explicit
    // pointer-by-value captures instead of [&] to work around a GCC 14
    // bug where reference captures in coroutine lambdas are corrupted
    // in the coroutine frame.

    // Test: CoroTask<void> runs to completion
    void
    testVoidCompletion()
    {
        using namespace std::chrono_literals;
        using namespace jtx;

        testcase("void completion");

        Env env(*this, envconfig([](std::unique_ptr<Config> cfg) {
            cfg->FORCE_MULTI_THREAD = true;
            return cfg;
        }));

        gate g;
        auto runner = env.app().getJobQueue().postCoroTask(
            jtCLIENT, "CoroTaskTest", [gp = &g](auto) -> CoroTask<void> {
                gp->signal();
                co_return;
            });
        BEAST_EXPECT(runner);
        BEAST_EXPECT(g.wait_for(5s));
        runner->join();
        BEAST_EXPECT(!runner->runnable());
    }

    // Test: correct_order — suspend, join, post, complete
    // Mirrors existing Coroutine_test::correct_order
    void
    testCorrectOrder()
    {
        using namespace std::chrono_literals;
        using namespace jtx;

        testcase("correct order");

        Env env(*this, envconfig([](std::unique_ptr<Config> cfg) {
            cfg->FORCE_MULTI_THREAD = true;
            return cfg;
        }));

        gate g1, g2;
        std::shared_ptr<JobQueue::CoroTaskRunner> r;
        auto runner = env.app().getJobQueue().postCoroTask(
            jtCLIENT,
            "CoroTaskTest",
            [rp = &r, g1p = &g1, g2p = &g2](auto runner) -> CoroTask<void> {
                *rp = runner;
                g1p->signal();
                co_await runner->suspend();
                g2p->signal();
                co_return;
            });
        BEAST_EXPECT(runner);
        BEAST_EXPECT(g1.wait_for(5s));
        runner->join();
        runner->post();
        BEAST_EXPECT(g2.wait_for(5s));
        runner->join();
    }

    // Test: incorrect_order — post before suspend
    // Mirrors existing Coroutine_test::incorrect_order
    void
    testIncorrectOrder()
    {
        using namespace std::chrono_literals;
        using namespace jtx;

        testcase("incorrect order");

        Env env(*this, envconfig([](std::unique_ptr<Config> cfg) {
            cfg->FORCE_MULTI_THREAD = true;
            return cfg;
        }));

        gate g;
        env.app().getJobQueue().postCoroTask(
            jtCLIENT, "CoroTaskTest", [gp = &g](auto runner) -> CoroTask<void> {
                runner->post();
                co_await runner->suspend();
                gp->signal();
                co_return;
            });
        BEAST_EXPECT(g.wait_for(5s));
    }

    // Test: JobQueueAwaiter — suspend + auto-repost
    void
    testJobQueueAwaiter()
    {
        using namespace std::chrono_literals;
        using namespace jtx;

        testcase("JobQueueAwaiter");

        Env env(*this, envconfig([](std::unique_ptr<Config> cfg) {
            cfg->FORCE_MULTI_THREAD = true;
            return cfg;
        }));

        gate g;
        int step = 0;
        env.app().getJobQueue().postCoroTask(
            jtCLIENT, "CoroTaskTest", [sp = &step, gp = &g](auto runner) -> CoroTask<void> {
                *sp = 1;
                co_await JobQueueAwaiter{runner};
                *sp = 2;
                co_await JobQueueAwaiter{runner};
                *sp = 3;
                gp->signal();
                co_return;
            });
        BEAST_EXPECT(g.wait_for(5s));
        BEAST_EXPECT(step == 3);
    }

    // Test: thread_specific_storage — per-coroutine LocalValue isolation
    // Mirrors existing Coroutine_test::thread_specific_storage
    void
    testThreadSpecificStorage()
    {
        using namespace std::chrono_literals;
        using namespace jtx;

        testcase("thread specific storage");
        Env env(*this);

        auto& jq = env.app().getJobQueue();

        static int const N = 4;
        std::array<std::shared_ptr<JobQueue::CoroTaskRunner>, N> a;

        LocalValue<int> lv(-1);
        BEAST_EXPECT(*lv == -1);

        gate g;
        jq.addJob(jtCLIENT, "LocalValTest", [&]() {
            this->BEAST_EXPECT(*lv == -1);
            *lv = -2;
            this->BEAST_EXPECT(*lv == -2);
            g.signal();
        });
        BEAST_EXPECT(g.wait_for(5s));
        BEAST_EXPECT(*lv == -1);

        for (int i = 0; i < N; ++i)
        {
            jq.postCoroTask(
                jtCLIENT,
                "CoroTaskTest",
                [this, ap = &a, gp = &g, lvp = &lv, id = i](auto runner) -> CoroTask<void> {
                    (*ap)[id] = runner;
                    gp->signal();
                    co_await runner->suspend();

                    this->BEAST_EXPECT(**lvp == -1);
                    **lvp = id;
                    this->BEAST_EXPECT(**lvp == id);
                    gp->signal();
                    co_await runner->suspend();

                    this->BEAST_EXPECT(**lvp == id);
                    co_return;
                });
            BEAST_EXPECT(g.wait_for(5s));
            a[i]->join();
        }
        for (auto const& r : a)
        {
            r->post();
            BEAST_EXPECT(g.wait_for(5s));
            r->join();
        }
        for (auto const& r : a)
        {
            r->post();
            r->join();
        }

        jq.addJob(jtCLIENT, "LocalValTest", [&]() {
            this->BEAST_EXPECT(*lv == -2);
            g.signal();
        });
        BEAST_EXPECT(g.wait_for(5s));
        BEAST_EXPECT(*lv == -1);
    }

    // Test: exception propagation
    void
    testExceptionPropagation()
    {
        using namespace std::chrono_literals;
        using namespace jtx;

        testcase("exception propagation");

        Env env(*this, envconfig([](std::unique_ptr<Config> cfg) {
            cfg->FORCE_MULTI_THREAD = true;
            return cfg;
        }));

        gate g;
        auto runner = env.app().getJobQueue().postCoroTask(
            jtCLIENT, "CoroTaskTest", [gp = &g](auto) -> CoroTask<void> {
                gp->signal();
                throw std::runtime_error("test exception");
                co_return;
            });
        BEAST_EXPECT(runner);
        BEAST_EXPECT(g.wait_for(5s));
        runner->join();
        // The exception is caught by promise_type::unhandled_exception()
        // and the coroutine is considered done
        BEAST_EXPECT(!runner->runnable());
    }

    // Test: multiple sequential co_await points
    void
    testMultipleYields()
    {
        using namespace std::chrono_literals;
        using namespace jtx;

        testcase("multiple yields");

        Env env(*this, envconfig([](std::unique_ptr<Config> cfg) {
            cfg->FORCE_MULTI_THREAD = true;
            return cfg;
        }));

        gate g;
        int counter = 0;
        std::shared_ptr<JobQueue::CoroTaskRunner> r;
        auto runner = env.app().getJobQueue().postCoroTask(
            jtCLIENT,
            "CoroTaskTest",
            [rp = &r, cp = &counter, gp = &g](auto runner) -> CoroTask<void> {
                *rp = runner;
                ++(*cp);
                gp->signal();
                co_await runner->suspend();
                ++(*cp);
                gp->signal();
                co_await runner->suspend();
                ++(*cp);
                gp->signal();
                co_return;
            });
        BEAST_EXPECT(runner);

        BEAST_EXPECT(g.wait_for(5s));
        BEAST_EXPECT(counter == 1);
        runner->join();

        runner->post();
        BEAST_EXPECT(g.wait_for(5s));
        BEAST_EXPECT(counter == 2);
        runner->join();

        runner->post();
        BEAST_EXPECT(g.wait_for(5s));
        BEAST_EXPECT(counter == 3);
        runner->join();
        BEAST_EXPECT(!runner->runnable());
    }

    // Test: CoroTask<T> returns a value via co_return
    void
    testValueReturn()
    {
        using namespace std::chrono_literals;
        using namespace jtx;

        testcase("value return");

        Env env(*this, envconfig([](std::unique_ptr<Config> cfg) {
            cfg->FORCE_MULTI_THREAD = true;
            return cfg;
        }));

        gate g;
        int result = 0;
        auto runner = env.app().getJobQueue().postCoroTask(
            jtCLIENT, "CoroTaskTest", [rp = &result, gp = &g](auto) -> CoroTask<void> {
                auto inner = []() -> CoroTask<int> { co_return 42; };
                *rp = co_await inner();
                gp->signal();
                co_return;
            });
        BEAST_EXPECT(runner);
        BEAST_EXPECT(g.wait_for(5s));
        runner->join();
        BEAST_EXPECT(result == 42);
        BEAST_EXPECT(!runner->runnable());
    }

    // Test: CoroTask<T> propagates exceptions from inner coroutines
    void
    testValueException()
    {
        using namespace std::chrono_literals;
        using namespace jtx;

        testcase("value exception");

        Env env(*this, envconfig([](std::unique_ptr<Config> cfg) {
            cfg->FORCE_MULTI_THREAD = true;
            return cfg;
        }));

        gate g;
        bool caught = false;
        auto runner = env.app().getJobQueue().postCoroTask(
            jtCLIENT, "CoroTaskTest", [cp = &caught, gp = &g](auto) -> CoroTask<void> {
                auto inner = []() -> CoroTask<int> {
                    throw std::runtime_error("inner error");
                    co_return 0;
                };
                try
                {
                    co_await inner();
                }
                catch (std::runtime_error const& e)
                {
                    *cp = true;
                }
                gp->signal();
                co_return;
            });
        BEAST_EXPECT(runner);
        BEAST_EXPECT(g.wait_for(5s));
        runner->join();
        BEAST_EXPECT(caught);
        BEAST_EXPECT(!runner->runnable());
    }

    // Test: CoroTask<T> chaining — nested value-returning coroutines
    void
    testValueChaining()
    {
        using namespace std::chrono_literals;
        using namespace jtx;

        testcase("value chaining");

        Env env(*this, envconfig([](std::unique_ptr<Config> cfg) {
            cfg->FORCE_MULTI_THREAD = true;
            return cfg;
        }));

        gate g;
        int result = 0;
        auto runner = env.app().getJobQueue().postCoroTask(
            jtCLIENT, "CoroTaskTest", [rp = &result, gp = &g](auto) -> CoroTask<void> {
                auto add = [](int a, int b) -> CoroTask<int> { co_return a + b; };
                auto mul = [add](int a, int b) -> CoroTask<int> {
                    int sum = co_await add(a, b);
                    co_return sum * 2;
                };
                *rp = co_await mul(3, 4);
                gp->signal();
                co_return;
            });
        BEAST_EXPECT(runner);
        BEAST_EXPECT(g.wait_for(5s));
        runner->join();
        BEAST_EXPECT(result == 14);  // (3 + 4) * 2
        BEAST_EXPECT(!runner->runnable());
    }

    // Test: postCoroTask returns nullptr when JobQueue is stopping
    void
    testShutdownRejection()
    {
        using namespace std::chrono_literals;
        using namespace jtx;

        testcase("shutdown rejection");

        Env env(*this, envconfig([](std::unique_ptr<Config> cfg) {
            cfg->FORCE_MULTI_THREAD = true;
            return cfg;
        }));

        // Stop the JobQueue
        env.app().getJobQueue().stop();

        auto runner = env.app().getJobQueue().postCoroTask(
            jtCLIENT, "CoroTaskTest", [](auto) -> CoroTask<void> { co_return; });
        BEAST_EXPECT(!runner);
    }

    void
    run() override
    {
        testVoidCompletion();
        testCorrectOrder();
        testIncorrectOrder();
        testJobQueueAwaiter();
        testThreadSpecificStorage();
        testExceptionPropagation();
        testMultipleYields();
        testValueReturn();
        testValueException();
        testValueChaining();
        testShutdownRejection();
    }
};

BEAST_DEFINE_TESTSUITE(CoroTask, core, xrpl);

}  // namespace test
}  // namespace xrpl

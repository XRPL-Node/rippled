#include <test/jtx.h>

#include <xrpl/core/JobQueue.h>

#include <chrono>
#include <mutex>

namespace xrpl {
namespace test {

class Coroutine_test : public beast::unit_test::suite
{
public:
    class gate
    {
    private:
        std::condition_variable cv_;
        std::mutex mutex_;
        bool signaled_ = false;

    public:
        // Thread safe, blocks until signaled or period expires.
        // Returns `true` if signaled.
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

    void
    correct_order()
    {
        using namespace std::chrono_literals;
        using namespace jtx;

        testcase("correct order");

        Env env(*this, envconfig([](std::unique_ptr<Config> cfg) {
            cfg->FORCE_MULTI_THREAD = true;
            return cfg;
        }));

        gate g1, g2;
        std::shared_ptr<JobQueue::Coro> c;
        env.app().getJobQueue().postCoro(jtCLIENT, "CoroTest", [&](auto const& cr) {
            c = cr;
            g1.signal();
            c->yield();
            g2.signal();
        });
        BEAST_EXPECT(g1.wait_for(5s));
        c->join();
        c->post();
        BEAST_EXPECT(g2.wait_for(5s));
    }

    void
    incorrect_order()
    {
        using namespace std::chrono_literals;
        using namespace jtx;

        testcase("incorrect order");

        Env env(*this, envconfig([](std::unique_ptr<Config> cfg) {
            cfg->FORCE_MULTI_THREAD = true;
            return cfg;
        }));

        gate g;
        gate gStart;
        auto coro = env.app().getJobQueue().postCoro(jtCLIENT, "CoroTest", [&](auto const& c) {
            gStart.signal();
            c->yield();
            g.signal();
        });

        // Wait for the coroutine to start.
        BEAST_EXPECT(gStart.wait_for(5s));

        BEAST_EXPECT(coro->state() == JobQueue::Coro::CoroState::Suspended);
        // Post the coroutine.
        coro->post();

        BEAST_EXPECT(g.wait_for(5s));
    }

    void
    thread_specific_storage()
    {
        using namespace std::chrono_literals;
        using namespace jtx;

        testcase("thread specific storage");
        Env env(*this);

        auto& jq = env.app().getJobQueue();

        static int const N = 4;
        std::array<std::shared_ptr<JobQueue::Coro>, N> a;

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
            jq.postCoro(jtCLIENT, "CoroTest", [&, id = i](auto const& c) {
                a[id] = c;
                g.signal();
                c->yield();

                this->BEAST_EXPECT(*lv == -1);
                *lv = id;
                this->BEAST_EXPECT(*lv == id);
                g.signal();
                c->yield();

                this->BEAST_EXPECT(*lv == id);
            });
            BEAST_EXPECT(g.wait_for(5s));
            a[i]->join();
        }
        for (auto const& c : a)
        {
            c->post();
            BEAST_EXPECT(g.wait_for(5s));
            c->join();
        }
        for (auto const& c : a)
        {
            c->post();
            c->join();
        }

        jq.addJob(jtCLIENT, "LocalValTest", [&]() {
            this->BEAST_EXPECT(*lv == -2);
            g.signal();
        });
        BEAST_EXPECT(g.wait_for(5s));
        BEAST_EXPECT(*lv == -1);
    }

    void
    stopJobQueueWhenCoroutineSuspended()
    {
        using namespace std::chrono_literals;
        using namespace jtx;

        testcase("Stop JobQueue when a coroutine is suspended");

        Env env(*this, envconfig([](std::unique_ptr<Config> cfg) {
            cfg->FORCE_MULTI_THREAD = true;
            return cfg;
        }));

        bool started = false;
        bool finished = false;
        std::condition_variable cv;
        std::mutex m;
        std::unique_lock<std::mutex> lk(m);
        auto coro = env.app().getJobQueue().postCoro(jtCLIENT, "Coroutine-Test", [&](auto const& c) {
            try
            {
                started = true;
                cv.notify_all();
                c->yield();
                finished = true;
                cv.notify_all();
            }
            catch (...)
            {
                finished = true;
                cv.notify_all();
                throw;
            }
        });

        cv.wait_for(lk, 5s, [&]() { return started; });
        env.app().getJobQueue().stop();

        cv.wait_for(lk, 5s, [&]() { return finished; });
        BEAST_EXPECT(finished);
    }

    void
    coroutineGetsDestroyedBeforeExecuting()
    {
        using namespace std::chrono_literals;
        using namespace jtx;

        testcase("Coroutine gets destroyed before executing");

        Env env(*this, envconfig([](std::unique_ptr<Config> cfg) {
            cfg->FORCE_MULTI_THREAD = true;
            return cfg;
        }));

        {
            auto coro = std::make_shared<JobQueue::Coro>(
                Coro_create_t{}, env.app().getJobQueue(), JobType::jtCLIENT, "test", [](auto coro) {

                });
        }

        pass();
    }

    void
    run() override
    {
        correct_order();
        incorrect_order();
        thread_specific_storage();
        stopJobQueueWhenCoroutineSuspended();
        coroutineGetsDestroyedBeforeExecuting();
    }
};

BEAST_DEFINE_TESTSUITE(Coroutine, core, xrpl);

}  // namespace test
}  // namespace xrpl

#include <test/csf/BasicNetwork.h>
#include <test/csf/Scheduler.h>

#include <doctest/doctest.h>

#include <set>
#include <vector>

using namespace xrpl::test::csf;

TEST_SUITE_BEGIN("BasicNetwork");

namespace {

struct Peer
{
    int id;
    std::set<int> set;

    Peer(Peer const&) = default;
    Peer(Peer&&) = default;

    explicit Peer(int id_) : id(id_)
    {
    }

    template <class Net>
    void
    start(Scheduler& scheduler, Net& net)
    {
        using namespace std::chrono_literals;
        auto t = scheduler.in(1s, [&] { set.insert(0); });
        if (id == 0)
        {
            for (auto const link : net.links(this))
                net.send(this, link.target, [&, to = link.target] {
                    to->receive(net, this, 1);
                });
        }
        else
        {
            scheduler.cancel(t);
        }
    }

    template <class Net>
    void
    receive(Net& net, Peer* from, int m)
    {
        set.insert(m);
        ++m;
        if (m < 5)
        {
            for (auto const link : net.links(this))
                net.send(this, link.target, [&, mm = m, to = link.target] {
                    to->receive(net, this, mm);
                });
        }
    }
};

}  // namespace

TEST_CASE("BasicNetwork operations")
{
    using namespace std::chrono_literals;
    std::vector<Peer> pv;
    pv.emplace_back(0);
    pv.emplace_back(1);
    pv.emplace_back(2);
    Scheduler scheduler;
    BasicNetwork<Peer*> net(scheduler);
    CHECK(!net.connect(&pv[0], &pv[0]));
    CHECK(net.connect(&pv[0], &pv[1], 1s));
    CHECK(net.connect(&pv[1], &pv[2], 1s));
    CHECK(!net.connect(&pv[0], &pv[1]));
    for (auto& peer : pv)
        peer.start(scheduler, net);
    CHECK(scheduler.step_for(0s));
    CHECK(scheduler.step_for(1s));
    CHECK(scheduler.step());
    CHECK(!scheduler.step());
    CHECK(!scheduler.step_for(1s));
    net.send(&pv[0], &pv[1], [] {});
    net.send(&pv[1], &pv[0], [] {});
    CHECK(net.disconnect(&pv[0], &pv[1]));
    CHECK(!net.disconnect(&pv[0], &pv[1]));
    for (;;)
    {
        auto const links = net.links(&pv[1]);
        if (links.empty())
            break;
        CHECK(net.disconnect(&pv[1], links[0].target));
    }
    CHECK(pv[0].set == std::set<int>({0, 2, 4}));
    CHECK(pv[1].set == std::set<int>({1, 3}));
    CHECK(pv[2].set == std::set<int>({2, 4}));
}

TEST_CASE("BasicNetwork disconnect")
{
    using namespace std::chrono_literals;
    Scheduler scheduler;
    BasicNetwork<int> net(scheduler);
    CHECK(net.connect(0, 1, 1s));
    CHECK(net.connect(0, 2, 2s));

    std::set<int> delivered;
    net.send(0, 1, [&]() { delivered.insert(1); });
    net.send(0, 2, [&]() { delivered.insert(2); });

    scheduler.in(1000ms, [&]() { CHECK(net.disconnect(0, 2)); });
    scheduler.in(1100ms, [&]() { CHECK(net.connect(0, 2)); });

    scheduler.step();

    // only the first message is delivered because the disconnect at 1 s
    // purges all pending messages from 0 to 2
    CHECK(delivered == std::set<int>({1}));
}

TEST_SUITE_END();


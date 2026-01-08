#include <xrpl/core/JobQueue.h>
#include <xrpl/core/ServiceRegistry.h>
#include <xrpl/overlay/Overlay.h>
#include <xrpl/overlay/PeerSet.h>

namespace xrpl {

class PeerSetImpl : public PeerSet
{
public:
    PeerSetImpl(ServiceRegistry& registry);

    void
    addPeers(
        std::size_t limit,
        std::function<bool(std::shared_ptr<Peer> const&)> hasItem,
        std::function<void(std::shared_ptr<Peer> const&)> onPeerAdded) override;

    /** Send a message to one or all peers. */
    void
    sendRequest(
        ::google::protobuf::Message const& message,
        protocol::MessageType type,
        std::shared_ptr<Peer> const& peer) override;

    std::set<Peer::id_t> const&
    getPeerIds() const override;

private:
    // Used in this class for access to boost::asio::io_context and
    // xrpl::Overlay.
    ServiceRegistry& registry_;
    beast::Journal journal_;

    /** The identifiers of the peers we are tracking. */
    std::set<Peer::id_t> peers_;
};

PeerSetImpl::PeerSetImpl(ServiceRegistry& registry)
    : registry_(registry), journal_(registry.journal("PeerSet"))
{
}

void
PeerSetImpl::addPeers(
    std::size_t limit,
    std::function<bool(std::shared_ptr<Peer> const&)> hasItem,
    std::function<void(std::shared_ptr<Peer> const&)> onPeerAdded)
{
    using ScoredPeer = std::pair<int, std::shared_ptr<Peer>>;

    auto const& overlay = registry_.overlay();

    std::vector<ScoredPeer> pairs;
    pairs.reserve(overlay.size());

    overlay.foreach([&](auto const& peer) {
        auto const score = peer->getScore(hasItem(peer));
        pairs.emplace_back(score, std::move(peer));
    });

    std::sort(
        pairs.begin(),
        pairs.end(),
        [](ScoredPeer const& lhs, ScoredPeer const& rhs) {
            return lhs.first > rhs.first;
        });

    std::size_t accepted = 0;
    for (auto const& pair : pairs)
    {
        auto const peer = pair.second;
        if (!peers_.insert(peer->id()).second)
            continue;
        onPeerAdded(peer);
        if (++accepted >= limit)
            break;
    }
}

void
PeerSetImpl::sendRequest(
    ::google::protobuf::Message const& message,
    protocol::MessageType type,
    std::shared_ptr<Peer> const& peer)
{
    auto packet = std::make_shared<Message>(message, type);
    if (peer)
    {
        peer->send(packet);
        return;
    }

    for (auto id : peers_)
    {
        if (auto p = registry_.overlay().findPeerByShortID(id))
            p->send(packet);
    }
}

std::set<Peer::id_t> const&
PeerSetImpl::getPeerIds() const
{
    return peers_;
}

class PeerSetBuilderImpl : public PeerSetBuilder
{
public:
    PeerSetBuilderImpl(ServiceRegistry& registry) : registry_(registry)
    {
    }

    virtual std::unique_ptr<PeerSet>
    build() override
    {
        return std::make_unique<PeerSetImpl>(registry_);
    }

private:
    ServiceRegistry& registry_;
};

std::unique_ptr<PeerSetBuilder>
make_PeerSetBuilder(ServiceRegistry& registry)
{
    return std::make_unique<PeerSetBuilderImpl>(registry);
}

class DummyPeerSet : public PeerSet
{
public:
    DummyPeerSet(ServiceRegistry& registry)
        : j_(registry.journal("DummyPeerSet"))
    {
    }

    void
    addPeers(
        std::size_t limit,
        std::function<bool(std::shared_ptr<Peer> const&)> hasItem,
        std::function<void(std::shared_ptr<Peer> const&)> onPeerAdded) override
    {
        JLOG(j_.error()) << "DummyPeerSet addPeers should not be called";
    }

    void
    sendRequest(
        ::google::protobuf::Message const& message,
        protocol::MessageType type,
        std::shared_ptr<Peer> const& peer) override
    {
        JLOG(j_.error()) << "DummyPeerSet sendRequest should not be called";
    }

    std::set<Peer::id_t> const&
    getPeerIds() const override
    {
        static std::set<Peer::id_t> emptyPeers;
        JLOG(j_.error()) << "DummyPeerSet getPeerIds should not be called";
        return emptyPeers;
    }

private:
    beast::Journal j_;
};

std::unique_ptr<PeerSet>
make_DummyPeerSet(ServiceRegistry& registry)
{
    return std::make_unique<DummyPeerSet>(registry);
}

}  // namespace xrpl

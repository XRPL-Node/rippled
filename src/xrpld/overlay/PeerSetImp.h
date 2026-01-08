#ifndef XRPL_OVERLAY_MAKE_PEERSET_H_INCLUDED
#define XRPL_OVERLAY_MAKE_PEERSET_H_INCLUDED

#include <xrpl/overlay/PeerSet.h>

#include <memory>

namespace xrpl {

class ServiceRegistry;
class PeerSet;
class PeerSetBuilder;

std::unique_ptr<PeerSetBuilder>
make_PeerSetBuilder(ServiceRegistry& registry);

/**
 * Make a dummy PeerSet that does not do anything.
 * @note For the use case of InboundLedger in ApplicationImp::loadOldLedger(),
 *       where a real PeerSet is not needed.
 */
std::unique_ptr<PeerSet>
make_DummyPeerSet(ServiceRegistry& registry);

}  // namespace xrpl

#endif

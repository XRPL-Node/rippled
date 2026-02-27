# Overlay Module Best Practices

## Description
Use when working with P2P networking in `src/xrpld/overlay/`. Covers peer connections, protocol negotiation, message relay, and clustering.

## Responsibility
Manages peer-to-peer connections and the overlay network protocol. Handles HTTP/1.1 upgrade handshake, session signatures (MITM prevention), Protocol Buffer message serialization, message compression, and cluster node management.

## Key Patterns

### HTTP/1.1 Upgrade Handshake
```cpp
// Connection starts as HTTP, upgrades to ripple protocol
// Custom headers:
// Connect-As, Public-Key, Session-Signature
// Network-ID, Network-Time
// Closed-Ledger, Previous-Ledger
// Remote-IP, Local-IP, Crawl
```

### Session Signature (MITM Prevention)
```cpp
// Binds SSL/TLS session to node identities
// Each peer signs the shared SSL session data
// Prevents man-in-the-middle attacks at the protocol level
```

### Message Serialization
```cpp
// Google Protocol Buffers for all p2p messages
// Messages are compressed before transmission
// Compression.h handles LZ4/no-compression selection
```

### Peer Management
```cpp
// Overlay manages the set of connected peers
class Overlay {
    virtual Peer::ptr findPeerByShortID(Peer::id_t) = 0;
    virtual void send(std::shared_ptr<Message> const&) = 0;  // Broadcast
    virtual void relay(Message const&, uint256 const& suppression) = 0;
};
```

### Clustering
```cpp
// Multiple rippled nodes can form a cluster
// Cluster nodes: share load, distribute crypto operations
// Cluster membership verified via node public keys
```

### Reduce Relay
```cpp
// ReduceRelayCommon.h - Minimize redundant message relay
// Uses HashRouter for message deduplication
// Peers track which messages they've already seen
```

## Common Pitfalls
- Always validate session signatures during handshake - never skip
- Never send uncompressed messages to peers that advertise compression support
- Cluster keys must be pre-configured - no dynamic cluster joining
- Message suppression via HashRouter is critical for network health - never bypass
- Protocol Buffer messages must match the expected schema version

## Key Files
- `src/xrpld/overlay/Overlay.h` - Main overlay manager interface
- `src/xrpld/overlay/Peer.h` - Peer connection representation
- `src/xrpld/overlay/PeerSet.h` - Collection of peers
- `src/xrpld/overlay/Cluster.h` - Cluster management
- `src/xrpld/overlay/Message.h` - Message structure
- `src/xrpld/overlay/Compression.h` - Message compression
- `src/xrpld/overlay/ReduceRelayCommon.h` - Relay optimization
- `src/xrpld/overlay/README.md` - Comprehensive protocol documentation

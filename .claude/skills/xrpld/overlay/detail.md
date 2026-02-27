# Overlay Detail Module Best Practices

## Description
Use when working with P2P networking implementation details in `src/xrpld/overlay/detail/`. Internal helpers for the overlay module.

## Responsibility
Internal implementation details for the overlay networking layer, including protocol message handling, peer connection state machines, and handshake implementation.

## Key Patterns

### PeerImp
```cpp
// PeerImp is the concrete implementation of the Peer interface
// Manages: SSL connection, protocol state, message queuing
// Uses enable_shared_from_this for async callback safety
```

### OverlayImpl
```cpp
// OverlayImpl is the concrete implementation of Overlay
// Manages: listening sockets, peer lifecycle, broadcast
```

## Common Pitfalls
- Peer connection state machine transitions must be atomic
- Never access PeerImp internals from outside overlay/detail/
- Message queuing must respect back-pressure limits

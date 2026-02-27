# PeerFinder Module Best Practices

## Description
Use when working with network discovery in `src/xrpld/peerfinder/`. Covers peer bootstrap, cache management, and connection slot strategy.

## Responsibility
Bootstraps into the XRP Ledger network and discovers peers. Manages bootcache (persistent addresses ranked by success), livecache (ephemeral recently-seen peers), and connection slot allocation.

## Key Patterns

### Three-Stage Connection Strategy
```cpp
// Stage 1: Connect to fixed peers (configured addresses)
// Stage 2: Try Livecache addresses (recently seen, with open slots)
// Stage 3: Fall back to Bootcache (persistent, ranked by valence)
```

### Bootcache (Persistent)
```cpp
// Persistent store of peer addresses ranked by "valence" (connection success)
// Higher valence = more reliable peer
// Persisted across restarts
// Used as fallback when livecache is empty
```

### Livecache (Ephemeral)
```cpp
// Recently seen peers with available connection slots
// Uses hop counts for distance estimation
// Random distribution for load balancing
// Not persisted - rebuilt on restart via gossip
```

### Slot Management
```cpp
// Slot states: accept, connect, connected, active, closing
// Slot properties: Inbound/Outbound, Fixed, Cluster
// Slots track connection lifecycle from handshake to disconnect
```

### Callback Pattern
```cpp
// Abstract interface for socket operations
// PeerFinder doesn't directly manage sockets
// Delegates I/O to the overlay layer via callbacks
```

## Common Pitfalls
- Fixed peers should always be tried first (Stage 1) - they are trusted
- Bootcache valence can decay over time - implement proper aging
- Livecache entries expire quickly - don't rely on stale data
- Slot limits prevent connection exhaustion - never bypass them
- Always check README.md for algorithm details before modifying

## Key Files
- `src/xrpld/peerfinder/PeerfinderManager.h` - Main manager interface
- `src/xrpld/peerfinder/Slot.h` - Connection slot representation
- `src/xrpld/peerfinder/README.md` - Extensive design documentation

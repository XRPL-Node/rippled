# Core Module Best Practices

## Description
Use when working with job processing and routing infrastructure in `src/libxrpl/core/` or `include/xrpl/core/`. Covers job queues, load monitoring, and message deduplication.

## Responsibility
Distributed job processing with priority levels, load monitoring for execution tracking, and hash-based message suppression/routing for peer deduplication.

## Key Patterns

### Job Queue Priorities
```cpp
// Job types ordered by priority (lower enum = lower priority)
enum JobType {
    jtINVALID = -1,
    jtPACK,              // Lowest priority
    jtCLIENT_WEBSOCKET,
    // ...
    jtVALIDATION_t,      // Trusted validation (high)
    jtADMIN              // Admin operations (highest)
};
// Position in enum determines scheduling priority
```

### HashRouter for Deduplication
```cpp
// Suppress duplicate messages from peers
std::optional<std::set<PeerShortID>> shouldRelay(uint256 const& key);
// Returns {} if already relayed, set of peers if should relay

// Track peer that sent a message
auto result = router.addSuppressionPeer(key, peer);
```

### RAII Locking
```cpp
// Always use lock_guard for mutex protection
std::lock_guard lock(mutex_);
auto result = emplace(key);
result.first.addPeer(peer);
return result.second;  // Return while lock is held
```

### Optional Return Types
```cpp
// Use std::optional for "maybe" results
std::optional<std::set<PeerShortID>> shouldRelay(uint256 const& key);
// Caller checks: if (auto peers = shouldRelay(key)) { ... }
```

## Common Pitfalls
- Never hold locks longer than necessary - lock_guard scope = critical section
- Never ignore job priorities when scheduling work - use appropriate JobType
- Always let HashRouter handle message deduplication rather than implementing custom logic

## Key Files
- `include/xrpl/core/Job.h` - Job wrapper with type, index, function
- `include/xrpl/core/JobTypes.h` - Job type enumerations and metadata
- `include/xrpl/core/JobQueue.h` - Priority queue for dispatching jobs
- `include/xrpl/core/HashRouter.h` - Duplicate message suppression
- `include/xrpl/core/LoadMonitor.h` - Job execution time tracking

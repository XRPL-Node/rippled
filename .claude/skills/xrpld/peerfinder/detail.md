# PeerFinder Detail Module Best Practices

## Description
Use when working with peer discovery implementation details in `src/xrpld/peerfinder/detail/`. Internal helpers for the peerfinder module.

## Responsibility
Internal implementation details for peer discovery: bootcache persistence, livecache management, slot state machine, and connection strategy implementation.

## Key Patterns

### Logic Class
```cpp
// The main implementation class for peer finding
// Implements the three-stage connection strategy
// Manages bootcache and livecache state
```

## Common Pitfalls
- Bootcache persistence must be atomic to prevent corruption
- Livecache expiration must be checked on every access
- Never include detail headers from outside peerfinder/

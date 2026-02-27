# App Paths Module Best Practices

## Description
Use when working with payment path finding in `src/xrpld/app/paths/`. Covers the path finding algorithm, trust line caching, and AMM liquidity integration.

## Responsibility
Implements the XRP Ledger payment path finding algorithm at the application level. Manages path requests, trust line caching, and integration with Automated Market Maker liquidity.

## Key Patterns

### Path Finding
```cpp
// Pathfinder: Algorithm that finds payment paths between accounts
// Considers: trust lines, order books, AMM pools
// Returns: Set of paths ranked by cost/quality
```

### Path Request Management
```cpp
// PathRequest  - Individual path find request
// PathRequests - Manages collection of active requests
// Requests can be one-shot or subscribed (streaming updates)
```

### Trust Line Caching
```cpp
// RippleLineCache caches trust line data
// Avoids repeated ledger lookups during path finding
// Cache is invalidated on ledger close
// TrustLine.h provides the trust line representation
```

### AMM Integration
```cpp
// AMMLiquidity.h - Integrates AMM pools into path finding
// AMMOffer.h - Represents AMM as a synthetic offer
// AMM pools are treated as a special offer type in the path finder
```

## Common Pitfalls
- Path finding is computationally expensive - respect search depth limits
- Trust line cache must be invalidated on ledger close
- AMM liquidity changes with pool state - don't cache AMM offers across ledgers
- Path subscriptions can be memory-intensive with many clients

## Key Files
- `src/xrpld/app/paths/Pathfinder.h` - Path finding algorithm
- `src/xrpld/app/paths/PathRequest.h` - Individual request handling
- `src/xrpld/app/paths/PathRequests.h` - Request collection management
- `src/xrpld/app/paths/RippleLineCache.h` - Trust line caching
- `src/xrpld/app/paths/TrustLine.h` - Trust line representation
- `src/xrpld/app/paths/AMMLiquidity.h` - AMM pool integration
- `src/xrpld/app/paths/AMMOffer.h` - Synthetic AMM offers

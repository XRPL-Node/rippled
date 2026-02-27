# App RDB Module Best Practices

## Description
Use when working with application-level database operations in `src/xrpld/app/rdb/`. Covers database abstractions and backend implementations.

## Responsibility
Application-level relational database operations. Provides interfaces for peer finding data storage, SQLite backend implementation, and database operation details.

## Key Patterns

### Interface-Based Design
```cpp
// PeerFinder.h - Abstract interface for peer data storage
// Backend implementations in backend/ subdirectory
// Allows swapping database backends without changing callers
```

### SQLite Backend
```cpp
// SQLiteDatabase.h - Primary backend implementation
// Uses SOCI wrapper from libxrpl/rdb
// Handles: peer data, transaction history, ledger metadata
```

### Backend Abstraction
```cpp
// backend/ directory contains concrete implementations
// backend/detail/ has implementation-specific helpers
// detail/ has shared implementation details
```

## Common Pitfalls
- Always use the abstract interface, not the SQLite backend directly
- Database operations should be off the main thread (use job queue)
- Respect the SOCI wrapper patterns from libxrpl/rdb

## Key Files
- `src/xrpld/app/rdb/PeerFinder.h` - Peer data interface
- `src/xrpld/app/rdb/backend/SQLiteDatabase.h` - SQLite implementation

# App RDB Backend Module Best Practices

## Description
Use when working with database backend implementations in `src/xrpld/app/rdb/backend/`. Covers concrete database implementations.

## Responsibility
Concrete database backend implementations for the app/rdb module. Primary implementation is SQLiteDatabase.

## Key Patterns

### SQLiteDatabase
```cpp
// Primary backend implementation using SQLite via SOCI
// Handles: peer storage, transaction history, ledger metadata
// Thread-safe via session-per-thread pattern from libxrpl/rdb
```

### Backend Detail
```cpp
// backend/detail/ contains SQLite-specific helpers
// Schema creation, migration, and optimization queries
```

## Common Pitfalls
- Always use parameterized queries via SOCI - never raw string SQL
- Database files can grow large - implement periodic VACUUM
- WAL checkpoint scheduling prevents unbounded WAL growth

## Key Files
- `src/xrpld/app/rdb/backend/SQLiteDatabase.h` - SQLite implementation
- `src/xrpld/app/rdb/backend/detail/` - SQLite-specific helpers

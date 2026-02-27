# RDB Module Best Practices

## Description
Use when working with relational database access in `src/libxrpl/rdb/` or `include/xrpl/rdb/`. Covers SOCI wrapper, database configuration, and checkpointing.

## Responsibility
Provides a C++ wrapper around SOCI (SQLite/PostgreSQL abstraction) for relational data access, configuration management, blob type conversions, and periodic WAL checkpoint management.

## Key Patterns

### DBConfig Lazy Initialization
```cpp
// Construct config without opening connection
DBConfig config(basicConfig, "my_database");

// Open session later when needed
soci::session session;
config.open(session);
// Allows config parsing separate from database I/O
```

### Blob Conversions
```cpp
// Convert between SOCI blobs and standard types
soci::blob blob(session);
std::vector<std::uint8_t> bytes;
convert(blob, bytes);   // blob -> vector

std::string str;
convert(blob, str);     // blob -> string
```

### Checkpointer with Weak Pointer Safety
```cpp
// Checkpointer uses weak_ptr to detect session destruction
auto checkpointer = makeCheckpointer(
    id,
    std::weak_ptr<soci::session>(sessionPtr),
    jobQueue,
    logs);
checkpointer->schedule();  // Periodic WAL checkpoints
// If session is destroyed, checkpointer safely no-ops
```

### Thread-Safe Sessions
```cpp
// Each thread gets its own soci::session from the pool
// Never share a soci::session across threads
```

## Common Pitfalls
- Never share a soci::session across threads - each thread needs its own
- Always use `std::weak_ptr` for session references in long-lived objects (like Checkpointer)
- Be aware of SOCI include warnings - use pragma diagnostic push/pop
- Always convert blobs to standard types before processing

## Key Files
- `include/xrpl/rdb/SociDB.h` - Main wrapper interface
- `src/libxrpl/rdb/SociDB.cpp` - Implementation
- `src/libxrpl/rdb/DatabaseCon.cpp` - Database connection setup

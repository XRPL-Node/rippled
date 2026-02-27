# NodeStore Module Best Practices

## Description
Use when working with persistent node storage in `src/libxrpl/nodestore/` or `include/xrpl/nodestore/`. Covers the storage abstraction layer, backend implementations, and caching.

## Responsibility
Abstraction layer for persistent storage of ledger nodes (SHAMap leaves and inner nodes). Supports multiple backend implementations (RocksDB, NuDB, Memory, Null) with a plugin architecture, caching, and async operations.

## Key Patterns

### Abstract Factory for Backends
```cpp
// Backend is pure virtual - concrete implementations selected at runtime
class Backend {
    virtual ~Backend() = default;
    virtual Status fetch(void const* key, std::shared_ptr<NodeObject>*) = 0;
    virtual void store(std::shared_ptr<NodeObject> const&) = 0;
    virtual void storeBatch(Batch const& batch) = 0;
};
// Backends: RocksDB, NuDB, Memory (testing), Null (disabled)
```

### Batch Operations
```cpp
// Always prefer batch operations for multiple reads/writes
virtual std::pair<std::vector<std::shared_ptr<NodeObject>>, Status>
    fetchBatch(std::vector<uint256 const*> const& hashes) = 0;
virtual void storeBatch(Batch const& batch) = 0;
// Significantly reduces I/O overhead vs individual operations
```

### Status Enum (Not Exceptions)
```cpp
enum class Status { ok, notFound, keyTooBig, /* ... */ };
// I/O operations return Status, NOT throw exceptions
auto [objects, status] = backend->fetchBatch(hashes);
if (status != Status::ok) { /* handle */ }
```

### Configuration via Section
```cpp
// Backend options come from config file sections
RocksDBBackend(int keyBytes, Section const& keyValues,
    Scheduler& scheduler, beast::Journal journal);
// Allows runtime tuning without recompilation
```

### Async Fetch with Scheduler
```cpp
// Non-blocking reads via scheduler integration
virtual void asyncFetch(uint256 const& hash,
    std::function<void(std::shared_ptr<NodeObject>)>&& callback);
// Callback invoked on scheduler thread
```

## Common Pitfalls
- Never use individual fetch/store in tight loops - use batch operations
- Never throw exceptions from backend implementations - return Status enum
- Always configure appropriate cache sizes for the workload
- Never assume a fetch will succeed - always check Status
- Use Memory backend for unit tests, never RocksDB

## Key Files
- `include/xrpl/nodestore/Database.h` - Main storage interface
- `include/xrpl/nodestore/Backend.h` - Abstract backend interface
- `include/xrpl/nodestore/Manager.h` - Backend factory
- `include/xrpl/nodestore/NodeObject.h` - Individual node wrapper
- `src/libxrpl/nodestore/BatchWriter.cpp` - Batch write optimization

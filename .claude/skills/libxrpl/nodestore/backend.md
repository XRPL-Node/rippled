# NodeStore Backend Module Best Practices

## Description
Use when working with storage backend implementations in `src/libxrpl/nodestore/backend/`. Covers RocksDB, NuDB, Memory, and Null backend factories.

## Responsibility
Concrete backend implementations for the NodeStore abstraction layer. Each backend provides persistent (or ephemeral) storage with different performance characteristics and use cases.

## Key Patterns

### Backend Factory Registration
```cpp
// Each backend is a factory that creates Backend instances
// Registered by name in the Manager
class RocksDBFactory : public Factory {
    std::string getName() const override { return "RocksDB"; }
    std::unique_ptr<Backend> createInstance(...) override;
};
```

### RocksDB Configuration
```cpp
// Extensive tuning via config Section
// Key options:
//   open_files       - Max open file descriptors
//   filter_bits      - Bloom filter bits per key (default 10)
//   cache_mb         - Block cache size in MB
//   file_size_mb     - Target SST file size
//   compression      - snappy (default), zlib, lz4, none
//   block_size       - Block size in KB (default 4)
//   num_threads      - Background compaction threads
```

### NuDB for Deterministic Storage
```cpp
// NuDB supports deterministic initialization
// Useful for reproducible test databases
// Hash-based storage with direct memory-mapped I/O
// Lower CPU overhead than RocksDB for write-heavy workloads
```

### Memory Backend for Testing
```cpp
// In-memory storage - fast but not persistent
// Use for unit tests only
// Thread-safe with internal mutex
```

### Null Backend for Disabled Storage
```cpp
// No-op backend - all operations succeed but store nothing
// Use when storage feature is disabled in config
```

## Common Pitfalls
- Use Memory backend for tests, never RocksDB (test isolation and speed)
- Always tune RocksDB bloom filter bits - default 10 is good for most workloads
- NuDB doesn't support range queries - only point lookups by hash
- Never use Null backend in production - data loss guaranteed
- Always configure appropriate open_files limits for the OS

## Key Files
- `src/libxrpl/nodestore/backend/RocksDBFactory.cpp` - RocksDB backend (~12.5KB)
- `src/libxrpl/nodestore/backend/NuDBFactory.cpp` - NuDB backend (~12KB)
- `src/libxrpl/nodestore/backend/MemoryFactory.cpp` - In-memory backend (~5KB)
- `src/libxrpl/nodestore/backend/NullFactory.cpp` - No-op backend (~2KB)

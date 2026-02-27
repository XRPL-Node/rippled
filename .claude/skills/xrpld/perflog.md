# PerfLog Module Best Practices

## Description
Use when working with performance logging in `src/xrpld/perflog/`. Covers performance instrumentation and metric collection.

## Responsibility
Performance logging and instrumentation for the rippled server. Tracks timing data, throughput metrics, and operational statistics for monitoring and debugging.

## Key Patterns

### Performance Logging Interface
```cpp
// Abstract interface for performance data collection
// Implementations can log to file, export to monitoring systems, etc.
// Enabled/disabled via configuration
```

### Detail Namespace
```cpp
// Implementation details in detail/ subdirectory
// Not part of public API
```

## Common Pitfalls
- Performance logging should have minimal overhead when disabled
- Never log sensitive data (keys, balances) in performance logs
- Ensure timestamps use consistent clock sources

## Key Files
- `src/xrpld/perflog/` - Performance logging implementation
- `src/xrpld/perflog/detail/` - Implementation details

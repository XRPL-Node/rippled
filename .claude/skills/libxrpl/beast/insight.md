# Beast Insight Module Best Practices

## Description
Use when working with metrics and monitoring in `src/libxrpl/beast/insight/` or `include/xrpl/beast/insight/`. Covers counters, gauges, and metrics collection.

## Responsibility
Provides metrics collection infrastructure for monitoring application performance: counters for cumulative values, gauges for point-in-time measurements, and hooks for metrics export.

## Key Patterns

### Metrics Types
```cpp
// Counter: Monotonically increasing value (e.g., total requests)
insight::Counter requests;
++requests;

// Gauge: Point-in-time value (e.g., current connections)
insight::Gauge connections;
connections = currentCount;

// Event: Timed operation tracking
insight::Event latency;
```

### Null Metrics
```cpp
// Use NullCollector when metrics disabled
// All operations become no-ops with zero overhead
auto collector = insight::NullCollector::New();
```

## Common Pitfalls
- Use Counter for cumulative values, Gauge for current state - don't mix them
- Always use NullCollector for tests - avoids metrics overhead
- Never store raw metric values in long-lived objects - use the insight types

## Key Files
- `include/xrpl/beast/insight/Counter.h` - Cumulative counter
- `include/xrpl/beast/insight/Gauge.h` - Point-in-time gauge
- `include/xrpl/beast/insight/Collector.h` - Metrics collector interface
- `include/xrpl/beast/insight/NullCollector.h` - No-op collector

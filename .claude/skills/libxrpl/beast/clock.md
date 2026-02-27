# Beast Clock Module Best Practices

## Description
Use when working with clock abstractions in `src/libxrpl/beast/clock/` or `include/xrpl/beast/clock/`. Covers abstract clock interfaces and manual clock implementations for testing.

## Responsibility
Provides clock abstractions that decouple code from system time, enabling deterministic testing with manual clocks and supporting different time granularities.

## Key Patterns

### Abstract Clock Interface
```cpp
// Use abstract_clock<T> for testable time-dependent code
// Production: uses system clock
// Testing: uses manual_clock for deterministic control
```

### Manual Clock for Testing
```cpp
// Advance time manually in tests
manual_clock<std::chrono::steady_clock> clock;
clock.advance(std::chrono::seconds(30));
// Deterministic - no flaky tests from timing issues
```

## Common Pitfalls
- Never use `std::chrono::system_clock::now()` directly - inject clock dependency
- Always use manual_clock in unit tests for deterministic behavior
- Be aware of Ripple Epoch vs Unix Epoch when working with ledger timestamps

## Key Files
- `include/xrpl/beast/clock/abstract_clock.h` - Abstract clock interface
- `include/xrpl/beast/clock/manual_clock.h` - Testable clock implementation

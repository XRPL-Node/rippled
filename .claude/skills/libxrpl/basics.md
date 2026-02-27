# Basics Module Best Practices

## Description
Use when working with foundational utilities in `src/libxrpl/basics/` or `include/xrpl/basics/`. Covers data structures, memory management, logging, numeric operations, error handling, and string utilities.

## Responsibility
Provides fundamental building blocks for the entire codebase: memory types (Buffer, Slice, Blob), intrusive smart pointers, arbitrary-precision integers (base_uint), multi-precision decimal arithmetic (Number), logging infrastructure, exception handling contracts, and platform abstractions.

## Key Patterns

### Memory Types
- **Slice** - Non-owning view into contiguous bytes (like std::span)
- **Buffer** - Owning byte container (like std::vector<uint8_t>)
- **Blob** - Alias for std::vector<unsigned char>
- Prefer `Slice` for read-only parameters, `Buffer` for owned data

### Intrusive Smart Pointers
```cpp
// Prefer intrusive pointers over std::shared_ptr for minimal overhead
// Object embeds its own reference count
class MyObject : public IntrusiveRefCounts {
    // ...
};
// Use SharedIntrusive<MyObject> when weak pointers needed
```

### Error Handling
```cpp
// Contract-based exceptions (logs call stack then throws)
Throw<std::runtime_error>("Invalid source file");
LogThrow("exception message");

// Logic errors for invariant violations
LogicError("This should never happen");

// Assertions (fuzzing-aware)
XRPL_ASSERT(condition, "function_name : description");
XRPL_VERIFY(condition, "message");  // Always enabled, not just debug
```

### Logging
```cpp
// Use JLOG macro - short-circuits if level disabled
JLOG(debugLog().warn()) << "Message: " << value;
JLOG(j_.trace()) << "Verbose detail";
// Never: debugLog().warn() << "msg";  // Missing JLOG wastes formatting
```

### Number Precision
```cpp
// Use Number for multi-precision decimal arithmetic
// Always use RAII guard for rounding mode
NumberRoundModeGuard mg(Number::towards_zero);
auto result = amount * rate;
// Guard restores previous mode on scope exit
```

### base_uint Template
```cpp
// Typed big-endian integers: base_uint<Bits, Tag>
// Tag prevents mixing unrelated types of same width
using uint256 = base_uint<256, void>;
using AccountID = base_uint<160, detail::AccountIDTag>;
```

## Common Pitfalls
- Never use `std::shared_ptr` when IntrusiveRefCounts is available - it adds a separate allocation
- Never format log messages without JLOG wrapper - wastes CPU when level is disabled
- Never use raw `assert()` - use `XRPL_ASSERT` for fuzzing instrumentation support
- Never mix `Buffer` and `Slice` ownership semantics - Slice does NOT own its data

## Key Files
- `include/xrpl/basics/IntrusiveRefCounts.h` - Atomic reference counting
- `include/xrpl/basics/IntrusivePointer.h` - Smart pointer using intrusive refs
- `include/xrpl/basics/base_uint.h` - Arbitrary-length big-endian integers
- `include/xrpl/basics/Buffer.h`, `Blob.h`, `Slice.h` - Memory types
- `include/xrpl/basics/Number.h` - Multi-precision decimal arithmetic
- `include/xrpl/basics/Log.h` - Logging infrastructure
- `include/xrpl/basics/contract.h` - Throw, LogicError, XRPL_ASSERT

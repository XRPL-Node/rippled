# Beast Module Best Practices

## Description
Use when working with the Beast support layer in `src/libxrpl/beast/` or `include/xrpl/beast/`. Covers networking types, unit testing, Journal logging, and instrumentation.

## Responsibility
Library support layer providing network types (IP addresses, endpoints), unit testing framework, Journal/logging abstractions, instrumentation/assertion macros, and type introspection utilities.

## Key Patterns

### Journal Logging
```cpp
// Journal wraps a Sink pointer - copy by value is cheap
beast::Journal const j_;

// Severity levels: kTrace, kDebug, kInfo, kWarning, kError, kFatal
JLOG(j_.warn()) << "User-facing issue";
JLOG(j_.debug()) << "Implementation detail";
JLOG(j_.trace()) << "Very verbose diagnostic";
JLOG(j_.error()) << "Unexpected failure";
```

### Unit Testing
```cpp
class MyTest : public beast::unit_test::suite {
    void run() override {
        testcase("feature description");
        BEAST_EXPECT(value == expected);    // Non-fatal assertion
        BEAST_REQUIRE(value == expected);   // Fatal - aborts test on failure
    }
};
BEAST_DEFINE_TESTSUITE(MyTest, module, ripple);
```

### Instrumentation
```cpp
// Use XRPL_ASSERT for debug-only assertions (fuzzing-aware)
XRPL_ASSERT(ptr != nullptr, "MyClass::method : null pointer");

// Format: "ClassName::methodName : description"
// The string format is important for fuzzing instrumentation
```

### IP Endpoint Types
```cpp
// Use beast::IP::Endpoint for network addresses
beast::IP::Endpoint endpoint;
// Supports both IPv4 and IPv6
// Includes port information
```

## Common Pitfalls
- Never use `BEAST_EXPECT` when the test cannot continue on failure - use `BEAST_REQUIRE` instead
- Never create a Journal with a dangling Sink pointer - ensure Sink outlives Journal
- Always include "ClassName::methodName" prefix in XRPL_ASSERT messages for traceability

## Key Files
- `include/xrpl/beast/utility/Journal.h` - Log sink abstraction
- `include/xrpl/beast/unit_test/suite.h` - Test framework
- `include/xrpl/beast/utility/instrumentation.h` - XRPL_ASSERT macros
- `include/xrpl/beast/net/IPEndpoint.h` - Network endpoint types
- `include/xrpl/beast/core/SemanticVersion.h` - Version parsing

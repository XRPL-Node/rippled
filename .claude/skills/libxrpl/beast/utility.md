# Beast Utility Module Best Practices

## Description
Use when working with beast utility classes in `src/libxrpl/beast/utility/` or `include/xrpl/beast/utility/`. Covers Journal logging, instrumentation, and type introspection.

## Responsibility
Provides utility classes including the Journal logging abstraction (Sink + Stream), XRPL_ASSERT/XRPL_VERIFY instrumentation macros, and type introspection helpers.

## Key Patterns

### Journal and Sink Architecture
```cpp
// Sink: abstract base for log message routing
class Sink {
    virtual void write(beast::severities::Severity level, std::string const& text) = 0;
    void console(bool output);  // Enable console output
};

// Journal: lightweight wrapper around Sink* (copy by value)
beast::Journal j(sinkPtr);
JLOG(j.warn()) << "Message";

// WrappedSink: adds prefix to all messages
beast::WrappedSink wrappedSink(parentSink, "MyModule");
```

### Instrumentation Macros
```cpp
// XRPL_ASSERT: Debug-only assertion (disabled in release, fuzzing-aware)
XRPL_ASSERT(condition, "ClassName::method : description");

// XRPL_VERIFY: Always-enabled assertion
XRPL_VERIFY(condition, "ClassName::method : description");

// Format convention: "ClassName::methodName : human-readable description"
```

## Common Pitfalls
- Always use "ClassName::methodName" prefix in assertion messages
- Journal is cheap to copy (just a pointer) - pass by value is fine
- WrappedSink must outlive any Journal created from it
- XRPL_ASSERT is no-op in release builds - don't put side effects in the condition

## Key Files
- `include/xrpl/beast/utility/Journal.h` - Log sink abstraction and Journal
- `include/xrpl/beast/utility/instrumentation.h` - XRPL_ASSERT, XRPL_VERIFY
- `include/xrpl/beast/utility/WrappedSink.h` - Prefixed log sink

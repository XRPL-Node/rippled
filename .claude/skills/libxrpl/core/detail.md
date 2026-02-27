# Core Detail Module Best Practices

## Description
Use when working with internal core implementation details in `src/libxrpl/core/detail/`. These are private implementation helpers not intended for direct external use.

## Responsibility
Internal implementation details for the core module, including helper functions and private data structures used by JobQueue, HashRouter, and LoadMonitor.

## Key Patterns

### Detail Namespace Convention
```cpp
namespace xrpl {
namespace detail {
// Internal helpers - not part of public API
// May change without notice between versions
}
}
```

### Encapsulation
- Code in `detail/` should only be included by its parent module
- Never include detail headers from outside the core module
- If you need functionality from detail, use the public core API instead

## Common Pitfalls
- Never depend on detail namespace types or functions from outside the core module
- Detail implementations may change without preserving API compatibility
- Always use the public `core/` headers for stable interfaces

## Key Files
- Files in this directory are implementation details of `include/xrpl/core/` headers

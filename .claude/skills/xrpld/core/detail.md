# Core Detail Module Best Practices

## Description
Use when working with core configuration implementation details in `src/xrpld/core/detail/`. Internal helpers for the Config system.

## Responsibility
Internal implementation details for the core configuration module, including config parsing helpers and platform-specific abstractions.

## Common Pitfalls
- Never include detail headers from outside the core module
- Config parsing must handle all edge cases gracefully (missing sections, invalid values)

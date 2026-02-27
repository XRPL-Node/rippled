# App Paths Detail Module Best Practices

## Description
Use when working with path finding implementation details in `src/xrpld/app/paths/detail/`. Internal helpers for the path finding module.

## Responsibility
Internal implementation details for the path finding algorithm, including search heuristics, cost calculation helpers, and internal data structures.

## Key Patterns

### Detail Namespace Convention
- Code in `detail/` is internal to the `app/paths` module
- Contains algorithm-specific helpers not needed by callers
- Never include detail headers from outside `app/paths`

## Common Pitfalls
- Path finding internals are performance-sensitive - profile before modifying
- Detail implementations may change without API compatibility

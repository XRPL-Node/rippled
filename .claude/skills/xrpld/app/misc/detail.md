# App Misc Detail Module Best Practices

## Description
Use when working with internal service implementation details in `src/xrpld/app/misc/detail/`. Internal helpers for the misc services module.

## Responsibility
Internal implementation details for app/misc services (fee voting internals, amendment table implementation, SHAMapStore internals, etc.).

## Key Patterns

### Detail Namespace Convention
- Code in `detail/` is internal to the `app/misc` module
- Implementation classes (e.g., AmendmentTableImpl) live here
- Never include detail headers from outside `app/misc`

## Common Pitfalls
- Detail implementations may change without API compatibility
- Always use the public `app/misc/` interfaces

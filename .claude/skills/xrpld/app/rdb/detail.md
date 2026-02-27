# App RDB Detail Module Best Practices

## Description
Use when working with database implementation details in `src/xrpld/app/rdb/detail/`. Internal helpers for the relational database module.

## Responsibility
Internal implementation details for app-level database operations, including SQL query helpers and schema management.

## Key Patterns

### Detail Namespace Convention
- Code in `detail/` is internal to the `app/rdb` module
- Contains SQL query construction and result parsing helpers
- Never include detail headers from outside `app/rdb`

## Common Pitfalls
- SQL queries must use parameterized bindings (SOCI style) - never string concatenation
- Detail implementations may change without API compatibility

# App RDB Backend Detail Module Best Practices

## Description
Use when working with database backend implementation details in `src/xrpld/app/rdb/backend/detail/`. SQLite-specific internal helpers.

## Responsibility
SQLite-specific implementation details: schema definitions, migration scripts, query optimization, and SOCI binding helpers.

## Common Pitfalls
- Schema migrations must be backwards-compatible
- Never include these headers from outside the rdb/backend module

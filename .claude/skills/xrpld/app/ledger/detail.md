# App Ledger Detail Module Best Practices

## Description
Use when working with ledger implementation details in `src/xrpld/app/ledger/detail/`. Internal helpers for the ledger module.

## Responsibility
Internal implementation details for the app/ledger module. Contains private helpers for ledger state management, iteration, and storage operations.

## Key Patterns

### Detail Namespace Convention
- Code in `detail/` is internal to the `app/ledger` module
- Never include detail headers from outside `app/ledger`
- If you need functionality, use the public `app/ledger/` headers instead

## Common Pitfalls
- Detail implementations may change without API compatibility
- Never expose detail types in public interfaces

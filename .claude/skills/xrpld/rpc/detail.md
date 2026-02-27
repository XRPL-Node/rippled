# RPC Detail Module Best Practices

## Description
Use when working with RPC implementation details in `src/xrpld/rpc/detail/`. Internal helpers for the RPC module.

## Responsibility
Internal implementation details for RPC handling, including request parsing, response formatting, coroutine machinery, and middleware.

## Common Pitfalls
- Coroutine state must be carefully managed to prevent leaks
- Never include detail headers from outside rpc/

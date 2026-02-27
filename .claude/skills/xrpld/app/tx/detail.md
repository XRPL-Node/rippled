# App Tx Detail Module Best Practices

## Description
Use when working with application transaction handling details in `src/xrpld/app/tx/detail/`. Internal helpers for transaction submission and processing.

## Responsibility
Internal implementation details for application-level transaction handling, including submission validation, queue integration, and metadata generation.

## Common Pitfalls
- Don't duplicate libxrpl/tx validation logic here
- Detail implementations may change without API compatibility

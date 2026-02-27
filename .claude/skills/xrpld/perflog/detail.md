# PerfLog Detail Module Best Practices

## Description
Use when working with performance logging implementation details in `src/xrpld/perflog/detail/`. Internal helpers for the perflog module.

## Responsibility
Internal implementation details for performance logging, including metric collection, formatting, and output management.

## Common Pitfalls
- Performance logging overhead must be minimal when disabled
- Never include detail headers from outside perflog/

# SHAMap (xrpld) Module Best Practices

## Description
Use when working with application-level SHAMap operations in `src/xrpld/shamap/`. Covers NodeFamily and application integration with the Merkle tree.

## Responsibility
Application-level integration of the SHAMap (Merkle radix tree) with the rippled server. Provides NodeFamily for managing tree node relationships and connecting the generic SHAMap to the node store.

## Key Patterns

### NodeFamily
```cpp
// NodeFamily connects SHAMap to the persistent NodeStore
// Provides: database access, tree node creation, hash verification
// Each SHAMap references a Family for storage operations
```

### Relationship to libxrpl/shamap
```cpp
// libxrpl/shamap: Generic Merkle tree implementation
// xrpld/shamap: Application-specific integration (NodeFamily, storage)
// The generic tree doesn't know about databases - NodeFamily bridges the gap
```

## Common Pitfalls
- Always use NodeFamily to create SHAMap instances connected to storage
- The libxrpl SHAMap is the implementation - this module is the glue layer
- Don't duplicate SHAMap logic here - extend via NodeFamily only

## Key Files
- `src/xrpld/shamap/NodeFamily.h` - Family relationship management
- `src/xrpld/shamap/NodeFamily.cpp` - Implementation

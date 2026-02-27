# PermissionedDomain Transactors Best Practices

## Description
Use when working with permissioned domain transaction types in `src/libxrpl/tx/transactors/PermissionedDomain/`. Covers domain-based access control for the DEX.

## Responsibility
Implements domain-based access control for permissioned DEX operations. Allows creating and deleting permissioned domains that gate who can participate in certain trading activities.

## Key Patterns

### PermissionedDomain Transaction Types
```cpp
// PermissionedDomainSet    - Create or update a permissioned domain
// PermissionedDomainDelete - Remove a permissioned domain
```

### Integration with Payments
```cpp
// Payments can reference a domain via sfDomainID field
// The domain gates which accounts can participate
// Used for regulated/permissioned trading environments
```

### Feature Gate
```cpp
ctx.rules.enabled(featurePermissionedDEX)
```

## Common Pitfalls
- Domains are referenced by other transactions (Payments) - deletion must check for references
- Always validate the feature gate before domain operations
- Domain ID matching is exact - no partial matches

## Key Files
- `src/libxrpl/tx/transactors/PermissionedDomain/PermissionedDomainSet.cpp` - Create/update domain
- `src/libxrpl/tx/transactors/PermissionedDomain/PermissionedDomainDelete.cpp` - Remove domain

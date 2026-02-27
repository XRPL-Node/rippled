# Delegate Transactors Best Practices

## Description
Use when working with delegation transaction types in `src/libxrpl/tx/transactors/Delegate/`. Covers delegate set and utilities.

## Responsibility
Manages transaction signing delegation, allowing accounts to authorize other accounts to sign transactions on their behalf without exposing the master key.

## Key Patterns

### Delegate Operations
```cpp
// DelegateSet - Create or modify a delegation
// Validates: delegate account exists, permissions valid
// Creates/Updates: Delegate ledger entry

// DelegateUtils - Shared utilities
// deleteDelegate() - Public cleanup interface, used by DeleteAccount
```

### Feature Gates
```cpp
// fixDelegateV1_1 - Added data existence checks
if (ctx.rules.enabled(fixDelegateV1_1)) {
    // Perform additional validation
}
```

### Integration with DeleteAccount
```cpp
// DelegateUtils::deleteDelegate() is called by DeleteAccount
// for cleanup when an account is being deleted
// Must handle all delegation-related ledger entries
```

## Common Pitfalls
- Always check feature gate `fixDelegateV1_1` for data existence validation
- Delegate cleanup must be thorough - used by DeleteAccount
- Never allow self-delegation

## Key Files
- `src/libxrpl/tx/transactors/Delegate/DelegateSet.cpp` - Delegation management
- `include/xrpl/tx/transactors/Delegate/DelegateUtils.h` - Shared utilities

# Vault Transactors Best Practices

## Description
Use when working with vault transaction types in `src/libxrpl/tx/transactors/Vault/`. Covers vault creation, configuration, deposits, withdrawals, deletion, and clawback.

## Responsibility
Implements the vault mechanism for asset holding and loan collateral management. Allows creating vaults, depositing/withdrawing assets, configuring vault parameters, and managing vault lifecycle.

## Key Patterns

### Vault Transaction Types
```cpp
// VaultCreate   - Create a new vault
// VaultSet      - Configure vault parameters
// VaultDelete   - Remove an empty vault
// VaultDeposit  - Deposit assets into a vault
// VaultWithdraw - Withdraw assets from a vault
// VaultClawback - Issuer clawback from vault
```

### Feature Gate
```cpp
ctx.rules.enabled(featureSingleAssetVault)
```

### Batch Transaction Restriction
```cpp
// Vault transactions are DISABLED in batch transactions
// Listed in Batch::disabledTxTypes alongside Lending
// Cannot be included in atomic batch operations
```

### Depth Limiting for Recursive Operations
```cpp
// Vault operations may involve recursive state checks
// Use depth parameter to prevent infinite recursion
TER checkVaultState(ReadView const& view, AccountID const& id, int depth = 0);
if (depth > maxDepth) return tecINTERNAL;
```

### Integration with Lending
```cpp
// Vaults serve as collateral for the lending protocol
// VaultDeposit/Withdraw affect loan collateralization
// Vault state changes may trigger loan state recalculation
```

## Common Pitfalls
- Vault operations are disabled in batch transactions
- Vaults can only be deleted when empty
- Clawback has different authorization requirements than regular withdrawal
- Always respect depth limits in recursive vault state checks
- Vault operations may interact with lending protocol state

## Key Files
- `src/libxrpl/tx/transactors/Vault/VaultCreate.cpp` - Vault creation
- `src/libxrpl/tx/transactors/Vault/VaultSet.cpp` - Configuration
- `src/libxrpl/tx/transactors/Vault/VaultDelete.cpp` - Deletion
- `src/libxrpl/tx/transactors/Vault/VaultDeposit.cpp` - Asset deposit
- `src/libxrpl/tx/transactors/Vault/VaultWithdraw.cpp` - Asset withdrawal
- `src/libxrpl/tx/transactors/Vault/VaultClawback.cpp` - Issuer clawback

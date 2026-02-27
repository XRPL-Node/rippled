# App Consensus Module Best Practices

## Description
Use when working with RCL consensus adapters in `src/xrpld/app/consensus/`. Bridges the generic consensus algorithm to rippled-specific types.

## Responsibility
Adapts the generic consensus algorithm (in `xrpld/consensus/`) to work with rippled application types. Provides concrete type adapters for transactions, transaction sets, and ledgers.

## Key Patterns

### RCL Adapters
```cpp
// RCLCxTx     - Adapts SHAMapItem as a consensus transaction
// RCLCxTxSet  - Adapts SHAMap as a consensus transaction set
// RCLCxLedger - Adapts Ledger as a consensus ledger
// RCLConsensus - Glues generic consensus to rippled Application
```

### Adapter Pattern
```cpp
// Generic consensus works with abstract types via CRTP
// RCL adapters provide concrete implementations
class RCLConsensus : public Consensus<RCLConsensus> {
    // Implements required methods using rippled types
    // e.g., acquireLedger(), proposeTx(), relay()
};
```

### Validation Handling
```cpp
// RCLValidations tracks validation messages from the network
// Maps to the generic Validations<> template
// Handles trusted vs untrusted validators
```

## Common Pitfalls
- Adapter methods must match the CRTP interface exactly - compiler errors are cryptic
- Never import generic consensus types into adapter code - always go through the adapter
- RCLConsensus holds references to Application services - ensure Application outlives it

## Key Files
- `src/xrpld/app/consensus/RCLConsensus.h` - Main consensus orchestrator
- `src/xrpld/app/consensus/RCLConsensus.cpp` - Implementation
- `src/xrpld/app/consensus/RCLValidations.h` - Validation handling
- `src/xrpld/app/consensus/RCLCxTx.h` - Transaction adapter
- `src/xrpld/app/consensus/RCLCxTxSet.h` - Transaction set adapter
- `src/xrpld/app/consensus/RCLCxLedger.h` - Ledger adapter

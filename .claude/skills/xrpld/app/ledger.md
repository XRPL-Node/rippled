# App Ledger Module Best Practices

## Description
Use when working with ledger management in `src/xrpld/app/ledger/`. Covers ledger storage, retrieval, history, and the Ledger class itself.

## Responsibility
Manages the lifecycle of ledger objects: creation, storage, retrieval, history tracking, inbound acquisition from network, and order book caching. The Ledger class is the central data structure.

## Key Patterns

### Ledger Class
```cpp
// Ledger is composed of two SHAMaps:
// 1. State Map  - Account roots, order books, ledger entries (STLedgerEntry)
// 2. Transaction Map - Transactions and metadata for that ledger

class Ledger final : public std::enable_shared_from_this<Ledger>,
                     public CountedObject<Ledger> {
    // Can be mutable (sole ownership) or immutable (shared, read-only)
    // Genesis ledger has special constructor
};
```

### Mutable vs Immutable
```cpp
// Mutable: Only one owner, can be modified
// Immutable: Shared via shared_ptr, read-only
// Transition: mutable -> immutable when accepted
// Never modify an immutable ledger
```

### LedgerMaster
```cpp
// Manages the chain of validated ledgers
// Tracks: current, validated, closed ledgers
// Handles ledger transitions during consensus
```

### Inbound Ledger Acquisition
```cpp
// InboundLedgers manages acquiring missing ledgers from peers
// InboundTransactions handles transaction set acquisition
// Both use async fetch with timeout and retry
```

### Iterator Adapters
```cpp
// sles_iter_impl - Iterate over SLE (state entries)
// txs_iter_impl  - Iterate over transactions
// Efficient iteration without copying
```

## Common Pitfalls
- Never modify an immutable ledger - check mutability first
- Ledger must be shared via shared_ptr (enable_shared_from_this)
- Genesis ledger construction is special - don't use for normal ledgers
- LedgerMaster state transitions must be atomic with consensus
- CountedObject tracking helps detect leaks - don't disable it

## Key Files
- `src/xrpld/app/ledger/Ledger.h` - Core ledger class
- `src/xrpld/app/ledger/LedgerMaster.h` - Ledger chain management
- `src/xrpld/app/ledger/LedgerHistory.h` - Historical tracking
- `src/xrpld/app/ledger/LedgerCleaner.h` - Online deletion
- `src/xrpld/app/ledger/InboundLedgers.h` - Network acquisition
- `src/xrpld/app/ledger/OrderBookDB.h` - Order book cache
- `src/xrpld/app/ledger/TransactionMaster.h` - Transaction lookup

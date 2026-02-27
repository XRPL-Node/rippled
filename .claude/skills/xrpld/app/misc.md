# App Misc Module Best Practices

## Description
Use when working with application services in `src/xrpld/app/misc/`. Covers fee voting, amendments, SHAMapStore, transaction queue, validator management, and NetworkOPs.

## Responsibility
Collection of application-level services: fee voting system, amendment mechanism, online deletion (SHAMapStore), transaction queue (TxQ), validator key/list management, and NetworkOPs (the central operations coordinator).

## Key Patterns

### Fee Voting
```cpp
// Validators vote on fee schedules every 256 ledgers ("voting ledgers")
// Pseudo-transactions injected to set fees
// Majority vote determines new fee schedule
// FeeVote.h - voting logic
// FeeEscalation.md - escalation documentation
```

### Amendment Mechanism
```cpp
// Network-wide ledger rule changes
// Requires 80% validator approval sustained for 2 weeks
// Tracked as pseudo-transactions in each ledger
// AmendmentTableImpl.h manages tracking and state
```

### SHAMapStore (Online Delete)
```cpp
// Prunes old ledger history automatically
// Uses rotation between two databases
// Configurable deletion intervals
// Must not delete data needed for active operations
```

### Transaction Queue (TxQ)
```cpp
// Queues transactions when ledger is full
// Priority based on fee level
// Handles fee escalation under load
// TxQ.h - queue implementation
```

### Validator Management
```cpp
// ValidatorKeys.h - Node's own validator keys
// ValidatorList.h - Trusted validator list (UNL)
// ValidatorSite.h - Sites serving validator lists
// Keys loaded from config, lists fetched from URLs
```

### NetworkOPs
```cpp
// Central operations coordinator - the "god object"
// Coordinates: consensus, ledger acceptance, transaction submission
// NetworkOPs.cpp is one of the largest files in the codebase
// Access via app.getOPs()
```

## Common Pitfalls
- Fee voting only happens on voting ledgers (every 256) - don't trigger manually
- Amendments require 2-week sustained 80% approval - no fast path
- SHAMapStore deletion must respect the advisory_delete safety window
- TxQ priority depends on current fee escalation level
- NetworkOPs is massive - changes here affect many systems

## Key Files
- `src/xrpld/app/misc/FeeVote.h` - Fee voting system
- `src/xrpld/app/misc/AmendmentTableImpl.h` - Amendment tracking
- `src/xrpld/app/misc/SHAMapStore.h` - Online deletion
- `src/xrpld/app/misc/TxQ.h` - Transaction queue
- `src/xrpld/app/misc/ValidatorKeys.h` - Validator key management
- `src/xrpld/app/misc/ValidatorList.h` - Trusted validator list
- `src/xrpld/app/misc/NetworkOPs.cpp` - Central operations
- `src/xrpld/app/misc/README.md` - Architecture documentation

# Consensus Module Best Practices

## Description
Use when working with the generic consensus algorithm in `src/xrpld/consensus/`. This is the application-independent consensus implementation.

## Responsibility
Implements the generic consensus algorithm separate from application-specific types. Uses CRTP (Curiously Recurring Template Pattern) to allow the application layer to plug in concrete types without virtual dispatch overhead.

## Key Patterns

### CRTP Design
```cpp
// Generic consensus uses CRTP - derived class provides concrete types
template <class Derived>
class Consensus {
    // Calls static_cast<Derived*>(this)->method() for app-specific behavior
    // No virtual dispatch overhead
};

// Application adapter:
class RCLConsensus : public Consensus<RCLConsensus> {
    // Provides concrete types: RCLCxTx, RCLCxTxSet, RCLCxLedger
};
```

### Consensus Phases
```cpp
// Open -> Establish -> Accept
// Open: Collecting transactions
// Establish: Proposing and voting on transaction set
// Accept: Applying agreed-upon transaction set to ledger
```

### Consensus Parameters (ConsensusParms.h)
```cpp
// Timing parameters for consensus rounds
// These are tuned for network behavior - don't change without careful analysis
```

### Disputed Transactions
```cpp
// DisputedTx tracks transactions where validators disagree
// Votes are collected and transaction is included/excluded based on threshold
```

## Common Pitfalls
- Never modify consensus parameters without understanding network-wide impact
- CRTP means compilation errors from type mismatches appear in template instantiation - read errors carefully
- The generic consensus module should NEVER depend on application-specific types (Ledger, STTx, etc.)
- Always test consensus changes with multi-node simulation

## Key Files
- `src/xrpld/consensus/Consensus.h` - Main consensus class (extensively documented)
- `src/xrpld/consensus/ConsensusParms.h` - Timing parameters
- `src/xrpld/consensus/ConsensusProposal.h` - Proposal structures
- `src/xrpld/consensus/DisputedTx.h` - Transaction dispute tracking
- `src/xrpld/consensus/LedgerTiming.h` - Ledger timing calculations
- `src/xrpld/consensus/LedgerTrie.h` - Transaction set representation
- `src/xrpld/consensus/Validations.h` - Generic validation tracking

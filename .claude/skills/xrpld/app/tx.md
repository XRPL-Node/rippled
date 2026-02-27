# App Tx Module Best Practices

## Description
Use when working with application-level transaction handling in `src/xrpld/app/tx/`. Covers transaction submission, validation, and application-layer processing.

## Responsibility
Application-level transaction handling that bridges the protocol-layer transaction processing (libxrpl/tx) with the running server. Handles transaction submission, validation, and integration with the consensus process.

## Key Patterns

### Relationship to libxrpl/tx
```cpp
// libxrpl/tx: Protocol-level transaction processing (Transactor, preflight, preclaim, doApply)
// xrpld/app/tx: Application-level orchestration (submission, queuing, consensus integration)
// This module calls into libxrpl/tx for actual transaction execution
```

### Transaction Submission Flow
```cpp
// 1. Client submits transaction via RPC
// 2. app/tx validates and queues transaction
// 3. Transaction enters TxQ (app/misc/TxQ)
// 4. Consensus selects transactions for next ledger
// 5. libxrpl/tx executes selected transactions
```

## Common Pitfalls
- Don't duplicate transaction validation logic - defer to libxrpl/tx
- Application-level checks (queue depth, fee escalation) happen here, not in libxrpl
- Transaction metadata is generated at this level after execution

## Key Files
- `src/xrpld/app/tx/` - Application transaction handling
- `src/xrpld/app/tx/detail/` - Implementation details

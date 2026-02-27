# Lending Transactors Best Practices

## Description
Use when working with lending protocol transaction types in `src/libxrpl/tx/transactors/Lending/`. Covers loan management, broker operations, and payment calculation.

## Responsibility
Implements a complex loan protocol with amortization schedules, interest calculation, broker management, and collateral handling. The largest transactor feature area by code volume.

## Key Patterns

### Loan Lifecycle
```cpp
// LoanSet     - Create or configure a loan
// LoanPay     - Make loan payments (principal + interest)
// LoanManage  - Manage loan state transitions
// LoanDelete  - Remove a completed/cancelled loan
```

### Broker Operations
```cpp
// LoanBrokerSet          - Create/configure a broker
// LoanBrokerDelete       - Remove a broker
// LoanBrokerCoverDeposit - Deposit collateral
// LoanBrokerCoverWithdraw - Withdraw collateral
// LoanBrokerCoverClawback - Clawback broker assets
```

### Payment Calculation (LendingHelpers.h)
```cpp
struct LoanPaymentParts {
    Number principalPaid;    // Reduces loan balance
    Number interestPaid;     // Goes to Vault
    Number valueChange;      // Loan value adjustment
    Number feePaid;          // Goes to Broker
};

// Key helper functions:
checkLendingProtocolDependencies()  // Validate feature gates
loanPeriodicRate()                  // Interest with secondsInYear
roundPeriodicPayment()              // Consistent rounding
```

### Feature Gate
```cpp
// All lending operations gated on single feature
ctx.rules.enabled(featureLendingProtocol)
```

### Batch Transaction Restriction
```cpp
// Lending transactions are DISABLED in batch transactions
// Listed in Batch::disabledTxTypes
// Cannot be included in atomic batch operations
```

## Common Pitfalls
- Lending is disabled in Batch transactions - do not try to batch lending ops
- Payment calculations require precise rounding - use LendingHelpers functions
- Always validate lending protocol dependencies before operations
- Interest calculation uses `secondsInYear` constant - be aware of leap year handling
- LendingHelpers.h is ~1800 lines - the most complex helper file in the codebase

## Key Files
- `src/libxrpl/tx/transactors/Lending/LoanSet.cpp` - Loan configuration (~585 lines)
- `src/libxrpl/tx/transactors/Lending/LoanPay.cpp` - Payment processing (~548 lines)
- `src/libxrpl/tx/transactors/Lending/LoanManage.cpp` - State management
- `src/libxrpl/tx/transactors/Lending/LoanDelete.cpp` - Loan removal
- `src/libxrpl/tx/transactors/Lending/LoanBrokerSet.cpp` - Broker setup
- `include/xrpl/tx/transactors/Lending/LendingHelpers.h` - Shared calculations (~1799 lines)

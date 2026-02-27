# Check Transactors Best Practices

## Description
Use when working with Check transaction types in `src/libxrpl/tx/transactors/Check/`. Covers check creation, cashing, and cancellation.

## Responsibility
Implements a check-like payment mechanism (delayed payment authorization). A sender creates a check, the recipient cashes it, and either party can cancel it (with expiration support).

## Key Patterns

### Check Lifecycle
```cpp
// 1. CreateCheck - Sender authorizes payment
//    Validates: destination exists, amount valid, expiration future
//    Creates: Check ledger entry with sender, destination, amount

// 2. CashCheck - Recipient claims payment
//    Validates: caller is destination, check not expired
//    Executes: transfers funds from sender to destination

// 3. CancelCheck - Either party cancels
//    Validates: caller is sender or destination, or check expired
//    Removes: Check ledger entry
```

### Error Codes
```cpp
tecNO_DST         // Destination account doesn't exist
temREDUNDANT      // Check to self
temBAD_AMOUNT     // Invalid amount
temBAD_EXPIRATION // Expiration in the past
tecEXPIRED        // Check has expired (for CashCheck)
```

## Common Pitfalls
- Checks can be cancelled by either sender or destination
- Expired checks can be cancelled by anyone
- CashCheck must handle both exact amount and "deliver minimum" modes
- Always validate expiration against the parent ledger close time

## Key Files
- `src/libxrpl/tx/transactors/Check/CreateCheck.cpp` - Check creation
- `src/libxrpl/tx/transactors/Check/CashCheck.cpp` - Check fulfillment (~440 lines)
- `src/libxrpl/tx/transactors/Check/CancelCheck.cpp` - Check cancellation

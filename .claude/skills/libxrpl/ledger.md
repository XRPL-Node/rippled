# Ledger Module Best Practices

## Description
Use when working with ledger state management in `src/libxrpl/ledger/` or `include/xrpl/ledger/`. Covers views, state tables, payments, and credit operations.

## Responsibility
Manages the state of the distributed ledger: abstractions for viewing, reading, and applying transactions to ledger state. Handles account ownership tracking, trust lines, payment processing, directory management, and credential verification.

## Key Patterns

### ReadView vs ApplyView
```cpp
// ReadView: Read-only access to ledger state
bool exists = view.exists(keylet::account(accountID));
auto sle = view.read(keylet::account(accountID));  // Returns shared_ptr<SLE const>

// ApplyView: Mutable access for transaction application
auto sle = view.peek(keylet::account(accountID));   // Returns shared_ptr<SLE>
sle->setFieldAmount(sfBalance, newBalance);
view.update(sle);   // Commit change
view.insert(newSle); // Create new entry
view.erase(sle);     // Remove entry
```

### Enum-Based Control (No Boolean Blindness)
```cpp
// NEVER: bool ignoreFreeze = true;
// ALWAYS: Use named enums
enum FreezeHandling { fhIGNORE_FREEZE, fhZERO_IF_FROZEN };
enum AuthHandling { ahIGNORE_AUTH, ahZERO_IF_UNAUTHORIZED };

STAmount accountHolds(ReadView const& view, AccountID const& account,
    Currency const& currency, AccountID const& issuer,
    FreezeHandling zeroIfFrozen, beast::Journal j);
```

### Asset Variant Handling
```cpp
// Use std::visit for Asset (Issue or MPTIssue)
return std::visit(
    [&](auto const& issue) {
        return isIndividualFrozen(view, account, issue);
    },
    asset.value());
```

### PaymentSandbox for Isolation
```cpp
// Test payment effects before committing
PaymentSandbox sandbox(&view);
// ... apply changes to sandbox ...
sandbox.apply(view);  // Commit all changes atomically
// Or let sandbox destruct to discard
```

### TER Return Codes
```cpp
// Transaction results are returned as TER, not exceptions
TER result = doApply();
if (result != tesSUCCESS) return result;
// TER categories: tes (success), tec (claimed fee), tef (failure), tel (local), tem (malformed)
```

### Depth Limiting
```cpp
// Recursive operations use depth parameter to prevent infinite recursion
TER checkVaultState(ReadView const& view, AccountID const& id, int depth = 0);
if (depth > maxDepth) return tecINTERNAL;
```

## Common Pitfalls
- Never use `read()` when you need to modify - use `peek()` instead
- Never forget to call `view.update(sle)` after modifying a peeked SLE
- Never use bare booleans for control flags - use the enum types
- Always check for frozen/unauthorized states before operating on trust lines
- Never ignore TER return values - propagate them up the call chain

## Key Files
- `include/xrpl/ledger/View.h` - Core read/write operations (largest file)
- `include/xrpl/ledger/ReadView.h` - Read-only interface
- `include/xrpl/ledger/ApplyView.h` - Mutable interface
- `src/libxrpl/ledger/PaymentSandbox.cpp` - Isolated payment testing
- `src/libxrpl/ledger/ApplyStateTable.cpp` - State change management
- `src/libxrpl/ledger/Credit.cpp` - Trust line and IOU operations

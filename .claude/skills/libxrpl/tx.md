# Transaction (tx) Module Best Practices

## Description
Use when working with transaction processing in `src/libxrpl/tx/` or `include/xrpl/tx/`. Covers the Transactor base class, transaction pipeline, invariant checks, and apply context.

## Responsibility
Implements the transaction processing pipeline: static validation (preflight), fee-claim checks (preclaim), transaction execution (doApply), and post-apply invariant verification. All transaction types derive from the Transactor base class.

## Key Patterns

### Transaction Pipeline (3 Phases)
```cpp
// Phase 1: Preflight - Static validation, NO ledger access
static NotTEC preflight(PreflightContext const& ctx) {
    if (ctx.tx[sfAmount] <= beast::zero)
        return temBAD_AMOUNT;
    return tesSUCCESS;
}

// Phase 2: Preclaim - Ledger read-only checks
static TER preclaim(PreclaimContext const& ctx) {
    auto sle = ctx.view.read(keylet::account(ctx.tx[sfAccount]));
    if (!sle) return tecNO_DST;
    return tesSUCCESS;
}

// Phase 3: doApply - Actual ledger mutations
TER doApply() override {
    auto sle = view().peek(keylet::account(account_));
    sle->setFieldAmount(sfBalance, newBalance);
    view().update(sle);
    return tesSUCCESS;
}
```

### Transactor Base Class
```cpp
class MyTransaction : public Transactor {
public:
    static constexpr ConsequencesFactoryType ConsequencesFactory{Normal};

    explicit MyTransaction(ApplyContext& ctx) : Transactor(ctx) {}

    // Optional: Gate on amendment
    static bool checkExtraFeatures(PreflightContext const& ctx);

    // Optional: Custom flag mask
    static std::uint32_t getFlagsMask(PreflightContext const& ctx);

    // Required
    static NotTEC preflight(PreflightContext const& ctx);
    static TER preclaim(PreclaimContext const& ctx);
    TER doApply() override;

    // Optional: Custom fee calculation
    static XRPAmount calculateBaseFee(ReadView const& view, STTx const& tx);
};
```

### Feature Gating
```cpp
static bool checkExtraFeatures(PreflightContext const& ctx) {
    // Return false to disable when amendment not enabled
    return ctx.rules.enabled(featureMyFeature);
}
```

### Flag Validation
```cpp
static std::uint32_t getFlagsMask(PreflightContext const& ctx) {
    return tfMyTransactionMask;  // Allowed flags
}
// Base class validates: (tx.getFlags() & ~getFlagsMask()) == 0
```

### ConsequencesFactory Types
```cpp
// Normal: Standard transaction
static constexpr ConsequencesFactoryType ConsequencesFactory{Normal};
// Blocker: Affects subsequent transactions' ability to claim fees
static constexpr ConsequencesFactoryType ConsequencesFactory{Blocker};
// Custom: Override makeTxConsequences() static method
static constexpr ConsequencesFactoryType ConsequencesFactory{Custom};
```

### TER Error Code Progression
```
tem* (malformed):  temINVALID, temBAD_AMOUNT, temDISABLED - preflight errors
tef* (failure):    tefFAILURE, tefNESTED_FAILURE - infrastructure errors
tel* (local):      telNETWORK_ID_MAKES_TX_NON_CANONICAL - local-only errors
tec* (claimed):    tecNO_DST, tecFROZEN, tecINSUFFICIENT_RESERVE - fee claimed, no effect
tes  (success):    tesSUCCESS - transaction applied
```

### Logging Convention
```cpp
JLOG(ctx.j.warn()) << "User error message";    // User-facing issues
JLOG(ctx.j.debug()) << "Detailed diagnostic";  // Implementation details
JLOG(ctx.j.trace()) << "Very verbose info";     // Deep debugging
JLOG(ctx.j.error()) << "Unexpected failure";    // Should not happen
```

### View Operations in doApply
```cpp
// Read-only (returns const SLE)
auto sle = view().read(keylet::account(accountID));

// Mutable (returns non-const SLE)
auto sle = view().peek(keylet::account(accountID));
sle->setFieldAmount(sfBalance, newBalance);
view().update(sle);  // MUST call after modification

// Create new entry
auto sle = std::make_shared<SLE>(keylet::myEntry(id));
sle->setFieldText(sfData, data);
view().insert(sle);

// Remove entry
view().erase(sle);
```

## Common Pitfalls
- Never access ledger state in preflight - it only has PreflightContext (no view)
- Never mutate ledger state in preclaim - it has ReadView only
- Always call `view().update(sle)` after modifying a peeked SLE
- Never return `tesSUCCESS` from preclaim/preflight if validation failed
- Always gate new transaction types on feature amendments
- Use `NotTEC` return type for preflight (not TER) - prevents returning tec codes from static checks
- Never forget to register new transaction types in `transactions.macro` and `applySteps.cpp`

## Key Files
- `include/xrpl/tx/Transactor.h` - Base class with static method signatures
- `src/libxrpl/tx/Transactor.cpp` - Base implementation (41KB)
- `src/libxrpl/tx/apply.cpp` - Transaction application entry point
- `src/libxrpl/tx/ApplyContext.cpp` - Context management
- `src/libxrpl/tx/InvariantCheck.cpp` - Post-apply invariant validation (117KB)
- `src/libxrpl/tx/applySteps.cpp` - Step-by-step dispatch

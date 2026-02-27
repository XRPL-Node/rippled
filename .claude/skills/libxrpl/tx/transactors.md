# Transactors Module Best Practices

## Description
Use when adding or modifying transaction types in `src/libxrpl/tx/transactors/`. This is the guide for implementing new transactors following the established patterns.

## Responsibility
Contains all concrete transaction type implementations. Each transactor follows the Transactor base class pattern with preflight/preclaim/doApply phases. Transaction types are organized by feature area in subdirectories.

## Template for New Transactors

### Header File (`include/xrpl/tx/transactors/MyTx.h`)
```cpp
#pragma once
#include <xrpl/tx/Transactor.h>

namespace xrpl {

class MyTransaction : public Transactor {
public:
    static constexpr ConsequencesFactoryType ConsequencesFactory{Normal};

    explicit MyTransaction(ApplyContext& ctx) : Transactor(ctx) {}

    static bool checkExtraFeatures(PreflightContext const& ctx);
    static std::uint32_t getFlagsMask(PreflightContext const& ctx);
    static NotTEC preflight(PreflightContext const& ctx);
    static TER preclaim(PreclaimContext const& ctx);
    TER doApply() override;
};

}
```

### Implementation File (`src/libxrpl/tx/transactors/MyFeature/MyTx.cpp`)
```cpp
#include <xrpl/tx/transactors/MyTx.h>
#include <xrpl/ledger/View.h>
#include <xrpl/protocol/Feature.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/TxFlags.h>

namespace xrpl {

bool
MyTransaction::checkExtraFeatures(PreflightContext const& ctx)
{
    return ctx.rules.enabled(featureMyFeature);
}

std::uint32_t
MyTransaction::getFlagsMask(PreflightContext const& ctx)
{
    return tfUniversalMask;  // Or custom mask
}

NotTEC
MyTransaction::preflight(PreflightContext const& ctx)
{
    // Static validation - NO ledger access
    auto const& tx = ctx.tx;

    if (tx[sfAmount] <= beast::zero)
    {
        JLOG(ctx.j.warn()) << "Bad amount.";
        return temBAD_AMOUNT;
    }

    return tesSUCCESS;
}

TER
MyTransaction::preclaim(PreclaimContext const& ctx)
{
    // Ledger read-only checks
    auto const sle = ctx.view.read(
        keylet::account(ctx.tx[sfAccount]));
    if (!sle)
        return terNO_ACCOUNT;

    return tesSUCCESS;
}

TER
MyTransaction::doApply()
{
    // Mutate ledger state
    auto sle = view().peek(keylet::account(account_));
    if (!sle)
        return tefINTERNAL;

    sle->setFieldAmount(sfBalance, newBalance);
    view().update(sle);

    return tesSUCCESS;
}

}
```

### Registration Checklist
1. Add transaction type to `include/xrpl/protocol/detail/transactions.macro`
2. Add dispatch case to `src/libxrpl/tx/applySteps.cpp`
3. Add feature flag to `include/xrpl/protocol/Feature.h` and `src/libxrpl/protocol/Feature.cpp`
4. Add invariant checks if needed in `src/libxrpl/tx/InvariantCheck.cpp`
5. Add to `disabledTxTypes` in Batch.cpp if not batch-compatible

## Key Patterns Across All Transactors

### Feature Gating
```cpp
static bool checkExtraFeatures(PreflightContext const& ctx) {
    return ctx.rules.enabled(featureMyFeature);
}
```

### Variant-Aware Validation
```cpp
// When handling Asset (Issue or MPTIssue):
if (auto const ret = std::visit(
    [&]<typename T>(T const&) { return preflightHelper<T>(ctx); },
    ctx.tx[sfAmount].asset().value()); !isTesSuccess(ret))
    return ret;
```

### Ledger Entry CRUD in doApply
```cpp
// Create
auto sle = std::make_shared<SLE>(keylet::myEntry(id));
view().insert(sle);

// Read (mutable)
auto sle = view().peek(keylet::myEntry(id));

// Update
sle->setFieldU32(sfFlags, newFlags);
view().update(sle);

// Delete
view().erase(sle);
```

### Helper/Utils Files
```cpp
// Complex features use a separate Helpers/Utils file:
// AMM -> AMMHelpers.h, AMMUtils.h
// NFT -> NFTokenUtils.h
// Lending -> LendingHelpers.h
// Keep reusable logic in helpers, transaction-specific logic in transactors
```

## Common Pitfalls
- Never forget to register in `transactions.macro` and `applySteps.cpp`
- Never access ledger state in preflight (no view available)
- Never mutate state in preclaim (read-only view)
- Always call `view().update(sle)` after modifying a peeked entry
- Always gate on feature amendment for consensus safety
- Use `NotTEC` return type from preflight, `TER` from preclaim/doApply
- Put shared logic in a *Helpers.h file, not duplicated across transactors

## Key Files
- `include/xrpl/tx/Transactor.h` - Base class definition
- `src/libxrpl/tx/applySteps.cpp` - Transaction type dispatch
- `include/xrpl/protocol/detail/transactions.macro` - Transaction type registry
- `src/libxrpl/tx/InvariantCheck.cpp` - Post-apply validation

# AMM (Automated Market Maker) Transactors Best Practices

## Description
Use when working with AMM transaction types in `src/libxrpl/tx/transactors/AMM/`. Covers pool creation, deposits, withdrawals, voting, bidding, deletion, and clawback.

## Responsibility
Implements decentralized trading pools with liquidity provider (LP) tokens. Allows users to create AMM pools, deposit/withdraw liquidity, vote on trading fees, bid on auction slots, and manage pool lifecycle.

## Key Patterns

### AMM Transaction Types
- **AMMCreate** - Create a new AMM pool with two assets
- **AMMDeposit** - Add liquidity to a pool, receive LP tokens
- **AMMWithdraw** - Remove liquidity, burn LP tokens
- **AMMVote** - Vote on trading fee percentage
- **AMMBid** - Bid on the auction slot for discounted trading
- **AMMDelete** - Remove an empty AMM pool
- **AMMClawback** - Issuer clawback of assets from AMM

### Precision Arithmetic
```cpp
// AMM calculations require controlled rounding
// ALWAYS use NumberRoundModeGuard for rounding control
NumberRoundModeGuard mg(Number::towards_zero);
auto lpTokens = ammLPTokens(amount1, amount2, lptIssue);

// Or explicit mode changes with RAII save
{
    saveNumberRoundMode _{Number::getround()};
    Number::setround(Number::upward);
    auto value = calculation;
}
```

### Helper Functions (AMMHelpers.h)
```cpp
ammLPTokens()             // Calculate LP tokens from pool reserves
lpTokensOut()             // LP tokens from deposit
ammAssetIn() / ammAssetOut() // Calculate swap amounts
withinRelativeDistance()   // Quality tolerance checks
changeSpotPriceQuality()  // Quote generation algorithm
// Quadratic equation solving for pricing
```

### Feature Gates
```cpp
// Multiple amendment versions
ammEnabled(ctx.rules)     // Base AMM feature
fixAMMv1_1                // First fix amendment
fixAMMv1_3                // Third fix amendment
// Always check which amendments are active
```

## Common Pitfalls
- Never perform AMM math without setting the rounding mode first
- Always use `NumberRoundModeGuard` (RAII) - never manually set/restore rounding
- Be aware that AMM calculations involve quadratic equations - precision matters
- AMMClawback has different authorization requirements than regular clawback
- Check `withinRelativeDistance` for quality tolerance rather than exact equality

## Key Files
- `src/libxrpl/tx/transactors/AMM/AMMCreate.cpp` - Pool creation
- `src/libxrpl/tx/transactors/AMM/AMMDeposit.cpp` - Liquidity deposit (~887 lines)
- `src/libxrpl/tx/transactors/AMM/AMMWithdraw.cpp` - Liquidity withdrawal (~911 lines)
- `src/libxrpl/tx/transactors/AMM/AMMVote.cpp` - Fee voting
- `src/libxrpl/tx/transactors/AMM/AMMBid.cpp` - Auction slot bidding
- `include/xrpl/tx/transactors/AMM/AMMHelpers.h` - Shared math utilities
- `include/xrpl/tx/transactors/AMM/AMMUtils.h` - AMM state utilities

# Transaction Paths Module Best Practices

## Description
Use when working with payment path finding and execution in `src/libxrpl/tx/paths/` or `include/xrpl/tx/paths/`. Covers the flow engine, ripple calculation, and offer stream processing.

## Responsibility
Implements payment path finding and execution for cross-currency payments. Handles the flow engine for executing payments across multiple paths (strands), offer book management, and amount calculation.

## Key Patterns

### Flow Engine Entry Point
```cpp
path::RippleCalc::Output flow(
    PaymentSandbox& view,
    STAmount const& deliver,
    AccountID const& src,
    AccountID const& dst,
    STPathSet const& paths,
    bool defaultPaths,
    bool partialPayment,
    bool ownerPaysTransferFee,
    OfferCrossing offerCrossing,
    std::optional<Quality> const& limitQuality,
    std::optional<STAmount> const& sendMax,
    std::optional<uint256> const& domainID,
    beast::Journal j,
    path::detail::FlowDebugInfo* flowDebugInfo = nullptr);
```

### Strand-Based Execution
```cpp
// A "strand" is a single payment path through the DEX
// Multiple strands can be tried to find the best rate
// StrandFlow executes a single strand
// Flow tries all strands and picks the best result
```

### OfferStream for Order Book Processing
```cpp
// Iterates through offers in an order book
// Handles expired offers, unfunded offers, and self-crossing
// Automatically cleans up stale offers during traversal
```

### Quality-Based Routing
```cpp
// Quality = exchange rate between input and output
// Lower quality number = better rate
// limitQuality prevents accepting rates worse than threshold
```

### PaymentSandbox Isolation
```cpp
// All path calculations happen in a PaymentSandbox
// Changes are only committed if the payment succeeds
// Allows trying multiple paths without side effects
```

## Common Pitfalls
- Never modify the ledger directly during path calculation - use PaymentSandbox
- Always respect limitQuality to prevent unfavorable exchange rates
- Be aware that offer traversal may clean up stale offers as a side effect
- FlowDebugInfo is optional and only for debugging - never rely on it in production logic
- Path finding is computationally expensive - respect depth and complexity limits

## Key Files
- `include/xrpl/tx/paths/Flow.h` - Main flow engine entry point
- `include/xrpl/tx/paths/RippleCalc.h` - Payment amount calculation
- `src/libxrpl/tx/paths/OfferStream.cpp` - Order book traversal (~12KB)
- `src/libxrpl/tx/paths/BookTip.cpp` - Order book tip management
- `src/libxrpl/tx/paths/detail/Steps.h` - Step-by-step calculation
- `src/libxrpl/tx/paths/detail/StrandFlow.h` - Single path execution

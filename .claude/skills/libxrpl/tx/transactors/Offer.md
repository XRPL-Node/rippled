# Offer Transactors Best Practices

## Description
Use when working with DEX offer transaction types in `src/libxrpl/tx/transactors/Offer/`. Covers order creation and cancellation on the native decentralized exchange.

## Responsibility
Implements decentralized exchange order management for the native XRPL order book. Handles offer creation (with matching/crossing), cancellation, and order book management.

## Key Patterns

### Offer Transaction Types
```cpp
// CreateOffer (~854 lines) - Create a new DEX order
//   - Validates amounts, currency pairs, account state
//   - Attempts to cross with existing offers (matching engine)
//   - Remaining amount becomes a standing order if not fully filled
//   - Handles self-crossing, expired offers, unfunded offers

// CancelOffer - Remove an existing order
//   - Validates caller owns the offer
//   - Removes offer from order book directory
```

### Offer Crossing (Matching Engine)
```cpp
// CreateOffer performs offer crossing inline:
// 1. Look up existing offers on the opposite side
// 2. Match at the offer's quality (exchange rate)
// 3. Consume matched offers (partial or full)
// 4. Place remaining amount as standing order
// This is the core DEX matching engine
```

### Quality and Order Books
```cpp
// Quality = exchange rate between TakerPays and TakerGets
// Order books indexed by currency pair and sorted by quality
// Lower quality number = better rate for taker
```

### Ticket Support
```cpp
// Offers can use tickets for sequence management
// Feature gate: featureTickets
```

## Common Pitfalls
- CreateOffer is one of the most complex transactors (~854 lines) - be thorough with changes
- Offer crossing modifies multiple ledger entries atomically
- Self-crossing (same account on both sides) has special handling
- Expired and unfunded offers are cleaned up during crossing
- Always validate that TakerPays and TakerGets are different assets

## Key Files
- `src/libxrpl/tx/transactors/Offer/CreateOffer.cpp` - Order creation and matching (~854 lines)
- `src/libxrpl/tx/transactors/Offer/CancelOffer.cpp` - Order cancellation

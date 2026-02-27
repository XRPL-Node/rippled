# MPT (Multi-Purpose Token) Transactors Best Practices

## Description
Use when working with MPT transaction types in `src/libxrpl/tx/transactors/MPT/`. Covers token issuance creation, configuration, destruction, and authorization.

## Responsibility
Implements Multi-Purpose Token (MPT) operations - a fungible token system simpler than full IOU trust lines. Handles token issuance lifecycle and holder authorization.

## Key Patterns

### MPT Transaction Types
```cpp
// MPTokenIssuanceCreate  - Create a new token issuance
// MPTokenIssuanceSet     - Modify issuance settings
// MPTokenIssuanceDestroy - Destroy an issuance (if no holders)
// MPTokenAuthorize       - Authorize/deauthorize a holder
```

### MPT vs IOU Trust Lines
```cpp
// MPT is simpler than IOU trust lines:
// - No rippling mechanics
// - No transfer fees between holders (only issuer)
// - Simpler authorization model
// - Uses MPTIssue instead of Issue
// - Uses MPTAmount instead of IOUAmount
```

### Feature Gate
```cpp
ctx.rules.enabled(featureMPTokensV1)
```

### Asset Variant Handling
```cpp
// MPTIssue is part of the Asset variant (Issue | MPTIssue)
// Use std::visit when handling Asset generically:
std::visit([&](auto const& issue) {
    // Works for both Issue and MPTIssue
}, asset.value());
```

## Common Pitfalls
- MPT issuances cannot be destroyed if any holders exist
- Authorization is per-holder, not per-token
- Don't confuse MPTIssue with Issue - they have different keylet types
- Always check `featureMPTokensV1` before MPT operations

## Key Files
- `src/libxrpl/tx/transactors/MPT/MPTokenIssuanceCreate.cpp` - Create issuance
- `src/libxrpl/tx/transactors/MPT/MPTokenIssuanceSet.cpp` - Modify settings
- `src/libxrpl/tx/transactors/MPT/MPTokenIssuanceDestroy.cpp` - Destroy issuance
- `src/libxrpl/tx/transactors/MPT/MPTokenAuthorize.cpp` - Holder authorization

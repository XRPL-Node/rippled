# NFT (Non-Fungible Token) Transactors Best Practices

## Description
Use when working with NFT transaction types in `src/libxrpl/tx/transactors/NFT/`. Covers minting, burning, offer management, and token modification.

## Responsibility
Implements NFT creation, trading, and lifecycle management. Handles token minting, burning, buy/sell offer creation and acceptance, offer cancellation, and dynamic NFT modification.

## Key Patterns

### NFT Transaction Types
```cpp
// NFTokenMint        - Create a new NFT
// NFTokenBurn        - Destroy an NFT
// NFTokenCreateOffer - Create a buy or sell offer
// NFTokenCancelOffer - Cancel an existing offer
// NFTokenAcceptOffer - Accept a buy or sell offer (execute trade)
// NFTokenModify      - Modify NFT metadata (dynamic NFTs)
```

### NFTokenUtils - Shared Utilities (~1025 lines)
```cpp
// Key utility functions:
findToken()              // Find an NFT in owner's token pages
insertToken()            // Add NFT to owner's directory
removeToken()            // Remove NFT from owner's directory
deleteTokenOffer()       // Clean up a single offer
removeTokenOffersWithLimit() // Batch cleanup with limit
notTooManyOffers()       // Check offer count limits
changeTokenURI()         // Update NFT URI (dynamic NFTs)

struct TokenAndPage {
    STObject token;
    std::shared_ptr<SLE> page;
};
```

### Token Storage in Pages
```cpp
// NFTs are stored in paginated directories
// Each page holds multiple tokens
// Pages are linked as a doubly-linked list
// insertToken/removeToken manage page splits and merges
```

### Feature Gates
```cpp
featureNFTokenMintOffer         // Combined mint + offer
fixRemoveNFTokenAutoTrustLine   // Fix for auto trust line removal
featureDynamicNFT               // NFTokenModify support
```

### Offer Directory Structure
```cpp
// Buy offers and sell offers stored in separate directories
// Indexed by token ID
// Offers can have expiration times
// Offers can specify a destination (private offers)
```

## Common Pitfalls
- Token page management is complex - always use NFTokenUtils functions
- Offer cleanup has limits (`removeTokenOffersWithLimit`) - don't assume all offers removed
- `notTooManyOffers()` must be checked before creating new offers
- Dynamic NFT modification requires `featureDynamicNFT` amendment
- Transfer fees only apply when the NFT issuer is not a party to the trade
- Burning an NFT with outstanding offers requires offer cleanup

## Key Files
- `src/libxrpl/tx/transactors/NFT/NFTokenMint.cpp` - Token minting
- `src/libxrpl/tx/transactors/NFT/NFTokenBurn.cpp` - Token destruction
- `src/libxrpl/tx/transactors/NFT/NFTokenCreateOffer.cpp` - Offer creation
- `src/libxrpl/tx/transactors/NFT/NFTokenAcceptOffer.cpp` - Offer acceptance (trade)
- `src/libxrpl/tx/transactors/NFT/NFTokenCancelOffer.cpp` - Offer cancellation
- `src/libxrpl/tx/transactors/NFT/NFTokenModify.cpp` - Dynamic NFT changes
- `include/xrpl/tx/transactors/NFT/NFTokenUtils.h` - Shared utilities (~1025 lines)

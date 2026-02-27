# Protocol Module Best Practices

## Description
Use when working with serialization and data types in `src/libxrpl/protocol/` or `include/xrpl/protocol/`. Covers STObject hierarchy, SField registry, serialization, TER codes, features, and keylets.

## Responsibility
Defines serialization formats, data types, and protocol constants. Handles conversion between C++ objects and wire format, JSON representation, and database storage. The largest module in libxrpl with 60+ files.

## Key Patterns

### STObject Hierarchy
```cpp
// STBase is the abstract root for all serializable types
// Concrete types: STInteger<N>, STBlob, STAmount, STArray, STObject, STPathSet, STVector256

// Access fields type-safely via SField references
auto amount = obj[sfAmount];          // Returns STAmount
auto flags = obj[sfFlags];            // Returns uint32
auto dest = obj[sfDestination];       // Returns AccountID
auto has = obj.isFieldPresent(sfMemos); // Check presence
```

### SField Registry
```cpp
// Fields are statically registered - never create SField at runtime
// SField carries type information and field ID
extern SField const sfAmount;       // STAmount type
extern SField const sfFlags;        // STInteger<uint32> type
extern SField const sfDestination;  // STAccount type

// Field IDs are protocol-defined and must not change
```

### Serializer for Binary Format
```cpp
// Deterministic binary serialization
Serializer s;
obj.add(s);              // Append binary representation
auto blob = s.peekData(); // Get bytes

// Deserialization
SerialIter sit(blob.data(), blob.size());
auto obj = std::make_shared<STObject>(sit, sfTransaction);
```

### Bidirectional JSON/Binary Conversion
```cpp
// Object to JSON
Json::Value json = obj.getJson(JsonOptions::none);

// JSON to Object
STParsedJSONObject parsed("tx_json", json);
if (parsed.object) { /* use parsed.object */ }
```

### TER (Transaction Error Result) Codes
```cpp
// Categories:
// tes - success (tesSUCCESS)
// tec - claimed fee, no effect (tecNO_DST, tecFROZEN, tecINSUFFICIENT_RESERVE)
// tef - failure (tefFAILURE)
// tel - local error (telNETWORK_ID_MAKES_TX_NON_CANONICAL)
// tem - malformed (temINVALID, temBAD_AMOUNT, temDISABLED)

// Check categories:
isTesSuccess(ter)   // ter == tesSUCCESS
isTecClaim(ter)     // tec range
isTemMalformed(ter) // tem range
```

### Feature/Amendment Flags
```cpp
// Gate new behavior on amendments
if (ctx.rules.enabled(featureMyFeature)) {
    // New behavior
} else {
    // Legacy behavior
}
// Register features in Feature.cpp
```

### Keylet for Ledger Entry Keys
```cpp
// Type-safe ledger key generation
auto key = keylet::account(accountID);     // Account root
auto key = keylet::line(a, b, currency);   // Trust line
auto key = keylet::offer(account, seq);    // Offer
auto key = keylet::nftoken(tokenID);       // NFT
// Keylet carries both the key (uint256) and the expected ledger entry type
```

### SOTemplate for Schema Validation
```cpp
// Define expected fields for a ledger entry type
SOTemplate const ltACCOUNT_ROOT = {
    {sfAccount, soeREQUIRED},
    {sfBalance, soeREQUIRED},
    {sfFlags, soeREQUIRED},
    {sfSequence, soeREQUIRED},
    {sfOwnerCount, soeDEFAULT},
    // ...
};
```

## Common Pitfalls
- Never create SField instances at runtime - they are static protocol definitions
- Never modify field IDs - they are part of the binary protocol
- Always use `isFieldPresent()` before accessing optional fields
- Never ignore TER return values - they carry critical error information
- Always gate new fields/behavior on feature amendments for consensus safety
- Respect JSON parsing depth limit of 10 to prevent stack overflow
- Use `[[nodiscard]]` on functions returning TER

## Key Files
- `include/xrpl/protocol/STObject.h` - Generic structured object container
- `include/xrpl/protocol/STAmount.h` - XRP and IOU amounts
- `include/xrpl/protocol/SField.h` - Field definitions
- `include/xrpl/protocol/TER.h` - Transaction result codes
- `include/xrpl/protocol/Feature.h` - Amendment flags
- `include/xrpl/protocol/Indexes.h` - Ledger entry key generation
- `include/xrpl/protocol/Keylet.h` - Type-safe ledger keys
- `include/xrpl/protocol/SOTemplate.h` - Object schema definitions
- `src/libxrpl/protocol/STParsedJSON.cpp` - JSON to object conversion

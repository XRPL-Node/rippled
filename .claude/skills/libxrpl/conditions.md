# Conditions Module Best Practices

## Description
Use when working with crypto-conditions in `src/libxrpl/conditions/` or `include/xrpl/conditions/`. Implements the RFC crypto-conditions specification.

## Responsibility
Implements crypto-conditions for conditional payments: condition fingerprints, fulfillment validation, multiple condition types (preimage, prefix, threshold, RSA, Ed25519), and binary DER serialization/deserialization.

## Key Patterns

### Fulfillment Interface
```cpp
// Abstract base for all fulfillment types
struct Fulfillment {
    virtual ~Fulfillment() = default;
    virtual Buffer fingerprint() const = 0;
    virtual Type type() const = 0;
    virtual bool validate(Slice data) const = 0;
    virtual std::uint32_t cost() const = 0;
    virtual Condition condition() const = 0;
};
```

### Factory Pattern for Deserialization
```cpp
// Returns unique_ptr + sets error_code (non-throwing)
std::unique_ptr<Fulfillment> deserialize(Slice s, std::error_code& ec);

// Usage:
std::error_code ec;
auto fulfillment = Fulfillment::deserialize(data, ec);
if (ec) return {};  // Early return on error
```

### Type-Safe Enums
```cpp
enum class Type : std::uint8_t {
    preimageSha256 = 0,
    prefixSha256 = 1,
    thresholdSha256 = 2,
    rsaSha256 = 3,
    ed25519Sha256 = 4
};
```

### Size Limits
```cpp
static constexpr std::size_t maxSerializedCondition = 128;
static constexpr std::size_t maxSerializedFulfillment = 256;
// Always respect these limits when creating conditions
```

## Common Pitfalls
- Never throw exceptions from binary parsing - use `std::error_code` parameter pattern
- Always check `ec` after deserialization before using the result
- Respect `maxSerializedFulfillment` size limits

## Key Files
- `include/xrpl/conditions/Condition.h` - Base condition with type/fingerprint/cost
- `include/xrpl/conditions/Fulfillment.h` - Abstract fulfillment interface
- `include/xrpl/conditions/detail/PreimageSha256.h` - Preimage implementation
- `include/xrpl/conditions/detail/utils.h` - DER encoding/decoding
- `include/xrpl/conditions/detail/error.h` - Error codes

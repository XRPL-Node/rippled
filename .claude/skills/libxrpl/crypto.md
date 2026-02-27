# Crypto Module Best Practices

## Description
Use when working with cryptographic operations in `src/libxrpl/crypto/` or `include/xrpl/crypto/`. Covers CSPRNG, secure memory erasure, and RFC1751 encoding.

## Responsibility
Low-level cryptographic operations: cryptographically secure random number generation (CSPRNG), RFC1751 word-based encoding for keys, and secure memory erasure to prevent sensitive data leakage.

## Key Patterns

### CSPRNG Usage
```cpp
// Thread-safe PRNG - use the global singleton
auto& prng = crypto_prng();

// Generate random value
auto randomValue = prng();

// Fill buffer with random bytes
prng(buffer.data(), buffer.size());

// NEVER create your own csprng_engine - use crypto_prng()
```

### Secure Memory Erasure
```cpp
// Use volatile memset to prevent compiler optimization
secure_erase(secretKey.data(), secretKey.size());
// MUST be called before destroying any buffer containing secrets
```

### Engine Concept Conformance
```cpp
// csprng_engine meets UniformRandomNumberEngine requirements
using result_type = std::uint64_t;
static constexpr result_type min();
static constexpr result_type max();
// Can be used with standard distributions
```

## Common Pitfalls
- Never create a local `csprng_engine` instance - always use `crypto_prng()` singleton
- Never use `std::rand()` or `std::mt19937` for security-sensitive operations
- Always call `secure_erase()` on secret key material before destruction
- Never copy or move a `csprng_engine` - it's non-copyable and non-movable by design
- Be aware of OpenSSL version differences - locking behavior varies

## Key Files
- `include/xrpl/crypto/csprng.h` - Thread-safe PRNG engine
- `include/xrpl/crypto/RFC1751.h` - Human-readable key encoding
- `include/xrpl/crypto/secure_erase.h` - Volatile memset for secrets

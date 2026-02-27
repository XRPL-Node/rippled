# xrpld Codebase Skills Index

## Description
This is the top-level guide for all best-practices skills in this repository. Use this to understand the codebase organization and find the right skill for any task.

## When to Use Skills
Reference a skill whenever you are:
- **Writing new code** in a module - check the skill first for established patterns
- **Modifying existing code** - verify your changes follow module conventions
- **Adding a new transaction type** - see `libxrpl/tx/transactors.md` for the full template
- **Debugging** - skills list key files and common pitfalls per module
- **Reviewing code** - skills document what "correct" looks like for each module

## Codebase Architecture

The codebase is split into two main areas:

### `src/libxrpl/` — The Library (skills in `.claude/skills/libxrpl/`)
Reusable library code: data types, serialization, cryptography, ledger state, transaction processing, and storage. This is the **protocol layer**.

| Module | Responsibility |
|--------|---------------|
| `basics` | Foundational types: Buffer, Slice, base_uint, Number, logging, error contracts |
| `beast` | Support layer: Journal logging, test framework, instrumentation, IP types |
| `conditions` | Crypto-conditions (RFC): fulfillment validation, DER encoding |
| `core` | Job queue, load monitoring, hash-based message dedup |
| `crypto` | CSPRNG, secure erasure, RFC1751 encoding |
| `json` | Json::Value, parsing, serialization, StaticString optimization |
| `ledger` | ReadView/ApplyView, state tables, payment sandbox, credit ops |
| `net` | HTTP/HTTPS client, SSL certs, async I/O |
| `nodestore` | Persistent node storage: RocksDB, NuDB, Memory backends |
| `protocol` | STObject hierarchy, SField, Serializer, TER codes, Features, Keylets |
| `proto` | Protocol Buffer generated headers (gRPC API definitions) |
| `rdb` | SOCI database wrapper, checkpointing |
| `resource` | Rate limiting, endpoint tracking, abuse prevention |
| `server` | Port config, SSL/TLS, WebSocket, admin networks |
| `shamap` | SHA-256 Merkle radix tree (16-way branching, COW) |
| `tx` | Transaction pipeline: Transactor base, preflight/preclaim/doApply |

### `src/xrpld/` — The Server Application (skills in `.claude/skills/xrpld/`)
The running rippled server: application lifecycle, consensus, networking, RPC, and peer management. This is the **application layer**.

| Module | Responsibility |
|--------|---------------|
| `app` | Application singleton, ledger management, consensus adapters, services |
| `app/main` | Application initialization and lifecycle |
| `app/ledger` | Ledger storage, retrieval, immutable state management |
| `app/consensus` | RCL consensus adapters (bridges generic algorithm to rippled) |
| `app/misc` | Fee voting, amendments, SHAMapStore, TxQ, validators, NetworkOPs |
| `app/paths` | Payment path finding algorithm, trust line caching |
| `app/rdb` | Application-level database operations |
| `app/tx` | Application-level transaction handling |
| `consensus` | Generic consensus algorithm (CRTP-based, app-independent) |
| `core` | Configuration (Config.h), time keeping, network ID |
| `overlay` | P2P networking: peer connections, protocol buffers, clustering |
| `peerfinder` | Network discovery: bootcache, livecache, slot management |
| `perflog` | Performance logging and instrumentation |
| `rpc` | RPC handler dispatch, coroutine suspension, 40+ command handlers |
| `shamap` | Application-level SHAMap operations (NodeFamily) |

### `include/xrpl/` — Header Files
Headers live in `include/xrpl/` and mirror the `src/libxrpl/` structure. Each skill already references its corresponding headers in the "Key Files" section.

## Cross-Cutting Conventions

### Error Handling
- **Transaction errors**: Return `TER` enum (tesSUCCESS, tecFROZEN, temBAD_AMOUNT, etc.)
- **Logic errors**: `Throw<std::runtime_error>()`, `LogicError()`, `XRPL_ASSERT()`
- **I/O errors**: `Status` enum or `boost::system::error_code`
- **RPC errors**: Inject via `context.params`

### Assertions
```cpp
XRPL_ASSERT(condition, "ClassName::method : description");  // Debug only
XRPL_VERIFY(condition, "ClassName::method : description");  // Always enabled
```

### Logging
```cpp
JLOG(j_.warn()) << "Message";  // Always wrap in JLOG macro
```

### Memory Management
- `IntrusiveRefCounts` + `SharedIntrusive` for shared ownership in libxrpl
- `std::shared_ptr` for shared ownership in xrpld
- `std::unique_ptr` for exclusive ownership
- `CountedObject<T>` mixin for instance tracking

### Feature Gating
```cpp
if (ctx.rules.enabled(featureMyFeature)) { /* new behavior */ }
```

### Code Organization
- Headers in `include/xrpl/`, implementations in `src/libxrpl/` or `src/xrpld/`
- `#pragma once` (never `#ifndef` guards)
- `namespace xrpl { }` for all code
- `detail/` namespace for internal helpers
- Factory functions: `make_*()` returning `unique_ptr` or `shared_ptr`

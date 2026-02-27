# App Main Module Best Practices

## Description
Use when working with application initialization and lifecycle in `src/xrpld/app/main/`. Covers Application startup, shutdown, and core services.

## Responsibility
Application initialization, lifecycle management, and core service wiring. Contains the Application interface and its implementation, gRPC server, load management, node identity, and the node store scheduler.

## Key Patterns

### Application Lifecycle
```cpp
// 1. Construct Application (parse config, create services)
// 2. setup() - Initialize all services, open databases
// 3. run() - Main event loop
// 4. signalStop() - Graceful shutdown
// 5. Destructor - Clean up resources
```

### Node Identity
```cpp
// NodeIdentity manages the node's public/private key pair
// Used for peer handshake, session signatures, and cluster membership
// Keys persisted in wallet_db
```

### Load Manager
```cpp
// Tracks server load across all subsystems
// Triggers load-based fee escalation
// Monitors job queue depth and execution time
```

### gRPC Server
```cpp
// Optional gRPC server alongside JSON-RPC
// Uses proto definitions from include/xrpl/proto/
// Separate handler set from JSON-RPC
```

## Common Pitfalls
- setup() must complete before any service is used
- signalStop() should be called for graceful shutdown - avoid hard kills
- Node identity keys must never be logged or exposed
- Load manager affects fee escalation - test under load

## Key Files
- `src/xrpld/app/main/Application.h` - Core interface
- `src/xrpld/app/main/Application.cpp` - Implementation
- `src/xrpld/app/main/BasicApp.h` - Base services
- `src/xrpld/app/main/GRPCServer.h` - gRPC integration
- `src/xrpld/app/main/LoadManager.h` - Load tracking
- `src/xrpld/app/main/NodeIdentity.h` - Node key management
- `src/xrpld/app/main/NodeStoreScheduler.h` - Async DB scheduler

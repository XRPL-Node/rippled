# App Module Best Practices

## Description
Use when working with the main application layer in `src/xrpld/app/`. Covers the Application singleton, ledger management, consensus adapters, services, and runtime coordination.

## Responsibility
Contains the main application logic and runtime coordination for the rippled server. The Application singleton provides access to all services. Sub-modules handle ledger management, consensus, fee voting, amendments, path finding, and transaction queuing.

## Key Patterns

### Application Singleton
```cpp
// Application is the master interface - access all services through it
class Application {
    virtual Config& config() = 0;
    virtual Logs& logs() = 0;
    virtual JobQueue& getJobQueue() = 0;
    virtual LedgerMaster& getLedgerMaster() = 0;
    virtual NetworkOPs& getOPs() = 0;
    virtual NodeStore::Database& getNodeStore() = 0;
    // ... many more service accessors
};
// Implemented by ApplicationImp (hidden in .cpp)
```

### ServiceRegistry Pattern
```cpp
// Services are injected via ServiceRegistry from libxrpl
// Application provides centralized access to all registered services
```

### Master Mutex
```cpp
// Recursive mutex protects shared state:
using MutexType = std::recursive_mutex;
// Protects: open ledger, server state, consensus engine
// Use std::lock_guard<Application::MutexType> for access
```

### Module Interface Pattern
```cpp
// Public interface: Module.h (abstract)
// Implementation: ModuleImp.h/cpp (hidden)
// Factory: make_Module() returns unique_ptr
```

## Common Pitfalls
- Never access Application services before setup() completes
- Always acquire master mutex when modifying open ledger or consensus state
- Use `std::recursive_mutex` (not plain mutex) - multiple services may lock re-entrantly
- Never create a second Application instance

## Key Files
- `src/xrpld/app/main/Application.h` - Core application interface
- `src/xrpld/app/main/Application.cpp` - Implementation (150+ includes)
- `src/xrpld/app/main/BasicApp.h` - Base application services
- `src/xrpld/app/main/GRPCServer.h` - gRPC server integration

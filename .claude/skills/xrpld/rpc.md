# RPC Module Best Practices

## Description
Use when working with RPC request handling in `src/xrpld/rpc/`. Covers command dispatch, handler implementation, coroutine suspension, and the gRPC interface.

## Responsibility
HTTP/JSON and gRPC RPC interface to rippled. Dispatches requests to 40+ command handlers, manages request context, supports coroutine-based suspension for async operations, and handles user roles/permissions.

## Key Patterns

### RPC Handler Pattern
```cpp
// Each handler is a function taking context, returning Json::Value
Json::Value doMyCommand(RPC::JsonContext& context) {
    // Validate parameters
    if (!context.params.isMember("field"))
        return RPC::missing_field_error("field");

    // Access application services
    auto& app = context.app;

    // Build response
    Json::Value result(Json::objectValue);
    result["status"] = "success";
    return result;
}
```

### Coroutine Suspension
```cpp
// RPC handlers can suspend for async operations
// Key types:
// Callback     - 0-argument function
// Continuation - Takes callback, promises to call it later
// Suspend      - Function from coroutine that takes continuation
// Coroutine    - Function given suspend to enable suspension

// This allows handlers to yield without blocking threads
```

### Request Context
```cpp
struct JsonContext {
    Application& app;
    Resource::Charge& loadType;
    Json::Value params;
    beast::Journal j;
    Role role;  // User, Admin, Identified, etc.
};
```

### Role-Based Access
```cpp
enum class Role { GUEST, USER, ADMIN, IDENTIFIED, PROXY, FORBID };
// Handlers specify minimum required role
// Admin handlers only accessible from admin IP ranges
```

### Error Handling
```cpp
// Standard error helpers:
RPC::missing_field_error("field_name");
RPC::invalid_field_error("field_name");
RPC::make_error(rpcINVALID_PARAMS, "description");
// Errors injected into JSON response
```

### gRPC Handlers
```cpp
// Separate handler set for gRPC interface
// Defined in GRPCHandlers.h
// Use Protocol Buffer request/response types
// Convert to/from native types at boundary
```

## Common Pitfalls
- Always validate parameters before accessing them
- Check user Role before executing privileged operations
- Use coroutine suspension for any operation that might block
- Never return internal data structures directly - always convert to JSON
- Handler functions are in `handlers/` directory - one file per command or group
- gRPC handlers must convert proto types to native types at the boundary

## Key Files
- `src/xrpld/rpc/RPCHandler.h` - Command dispatcher
- `src/xrpld/rpc/Context.h` - RPC context/state
- `src/xrpld/rpc/Status.h` - RPC status codes
- `src/xrpld/rpc/Role.h` - User role/permissions
- `src/xrpld/rpc/GRPCHandlers.h` - gRPC integration
- `src/xrpld/rpc/handlers/` - Individual command handlers (40+)
- `src/xrpld/rpc/README.md` - Coroutine documentation

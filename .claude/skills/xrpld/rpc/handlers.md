# RPC Handlers Module Best Practices

## Description
Use when adding or modifying RPC command handlers in `src/xrpld/rpc/handlers/`. Contains 40+ individual RPC command implementations.

## Responsibility
Individual RPC command handler implementations. Each file implements one or more related RPC commands following the standard handler pattern.

## Key Patterns

### Handler Function Signature
```cpp
// Standard handler pattern
Json::Value doMyCommand(RPC::JsonContext& context) {
    // 1. Validate parameters
    if (!context.params.isMember("required_field"))
        return RPC::missing_field_error("required_field");

    // 2. Check permissions
    // (Role already verified by dispatcher based on handler metadata)

    // 3. Access application services
    auto& app = context.app;
    auto& ledger = context.ledgerMaster.getValidatedLedger();

    // 4. Execute logic
    // ...

    // 5. Build and return response
    Json::Value result(Json::objectValue);
    result["status"] = "success";
    return result;
}
```

### Parameter Validation
```cpp
// Use standard error helpers
RPC::missing_field_error("field_name");    // Required field missing
RPC::invalid_field_error("field_name");    // Field has wrong type/value
RPC::make_error(rpcINVALID_PARAMS, "description"); // Generic error
```

### Adding a New Handler
1. Create a new .cpp file in `handlers/`
2. Implement the handler function following the pattern above
3. Register the command in the handler dispatch table
4. Document the command parameters and response format

### Common Handler Files
```
AccountInfo.cpp     - account_info command
AccountLines.cpp    - account_lines (trust lines)
AccountOffers.cpp   - account_offers
Submit.cpp          - submit (transaction submission)
Subscribe.cpp       - subscribe (event streaming)
LedgerData.cpp      - ledger_data
ServerInfo.cpp      - server_info
Fee1.cpp            - fee command
Tx.cpp              - tx (transaction lookup)
BookOffers.cpp      - book_offers (order book)
```

## Common Pitfalls
- Always validate ALL parameters before using them
- Never return internal data structures - convert to JSON at the boundary
- Respect Role-based access control - admin handlers must check role
- Handlers should be stateless - all state comes from context
- Use coroutine suspension for any blocking operation (DB queries, network)
- Error responses must follow the standard error format

## Key Files
- `src/xrpld/rpc/handlers/` - All handler implementations
- `src/xrpld/rpc/RPCHandler.h` - Dispatch table and registration

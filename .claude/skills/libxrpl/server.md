# Server Module Best Practices

## Description
Use when working with server configuration in `src/libxrpl/server/` or `include/xrpl/server/`. Covers port configuration, SSL/TLS, WebSocket options, and authentication.

## Responsibility
Manages server listening ports, WebSocket configuration, SSL/TLS setup, authentication credentials, and admin network restrictions. Handles parsing configuration into validated port structures.

## Key Patterns

### Dual ParsedPort/Port Pattern
```cpp
// ParsedPort: intermediate parsing result (may be incomplete)
struct ParsedPort { /* partially validated fields */ };

// Port: final validated configuration (ready to use)
struct Port {
    std::string name;
    boost::asio::ip::address ip;
    std::uint16_t port = 0;
    std::set<std::string, iless> protocol;
    // ...
};
```

### Configuration Section Parsing
```cpp
// Parse from config file section, log errors to stream
void parse_Port(ParsedPort& port, Section const& section, std::ostream& log);
// Does NOT throw - logs issues and sets defaults
```

### CIDR Admin Networks
```cpp
// IP-based admin access control with CIDR support
std::vector<boost::asio::ip::network_v4> admin_nets_v4;
std::vector<boost::asio::ip::network_v6> admin_nets_v6;
// Supports both IPv4 and IPv6 ranges
```

### WebSocket Options
```cpp
// Fine-grained compression control
boost::beast::websocket::permessage_deflate pmd_options;
// Queue limits prevent memory exhaustion
std::optional<std::uint16_t> ws_queue_limit;
// limit=0 means unlimited (not -1)
```

### SSL Context Lazy Creation
```cpp
// SSL context created only when needed
std::shared_ptr<boost::asio::ssl::context> context;
// Initialized when first SSL connection established
```

## Common Pitfalls
- Never use port-only access control - always use CIDR network restrictions for admin
- Use `iless` comparator for protocol names (case-insensitive)
- Remember that `limit=0` means unlimited, not disabled
- Always log parsing errors rather than throwing
- Separate user/admin credentials - don't reuse

## Key Files
- `include/xrpl/server/Port.h` - Port configuration structure
- `src/libxrpl/server/Port.cpp` - Parsing and validation
- `src/libxrpl/server/JSONRPCUtil.cpp` - RPC utility functions
- `src/libxrpl/server/LoadFeeTrack.cpp` - Load and fee tracking

# Beast Net Module Best Practices

## Description
Use when working with network types in `src/libxrpl/beast/net/` or `include/xrpl/beast/net/`. Covers IP address and endpoint representations.

## Responsibility
Provides network type abstractions for IP addresses (v4 and v6) and endpoints (address + port), with parsing utilities and comparison operators.

## Key Patterns

### IP Endpoint Usage
```cpp
// beast::IP::Endpoint combines address + port
beast::IP::Endpoint endpoint(
    beast::IP::Address::from_string("127.0.0.1"), 51235);

// Supports both IPv4 and IPv6
beast::IP::Endpoint v6endpoint(
    beast::IP::Address::from_string("::1"), 51235);
```

### Address Parsing
```cpp
// Parse from string with error handling
boost::system::error_code ec;
auto address = beast::IP::Address::from_string(input, ec);
if (ec) { /* handle invalid address */ }
```

### Comparison and Ordering
```cpp
// Endpoints support full comparison for use in containers
std::set<beast::IP::Endpoint> endpoints;
std::map<beast::IP::Endpoint, Consumer> consumers;
```

## Common Pitfalls
- Always handle both IPv4 and IPv6 when working with endpoints
- Use error_code overload of from_string to handle invalid input gracefully
- Never assume address format - always parse and validate

## Key Files
- `include/xrpl/beast/net/IPEndpoint.h` - Combined address + port
- `include/xrpl/beast/net/IPAddress.h` - IP address types
- `include/xrpl/beast/net/IPAddressV4.h` - IPv4 specific
- `include/xrpl/beast/net/IPAddressV6.h` - IPv6 specific

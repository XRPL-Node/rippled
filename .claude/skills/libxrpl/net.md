# Net Module Best Practices

## Description
Use when working with network communication in `src/libxrpl/net/` or `include/xrpl/net/`. Covers HTTP/HTTPS client functionality and SSL certificate management.

## Responsibility
Provides HTTP/HTTPS client functionality for network requests, including SSL certificate registration, asynchronous request handling with boost::asio, and callback-based completion.

## Key Patterns

### Async HTTP with Callbacks
```cpp
// Callback returns bool indicating success/continuation
HTTPClient::get(
    bSSL, io_context,
    std::deque<std::string>{"site1.com", "site2.com"},  // Fallback sites
    443, "/path",
    maxResponseBytes,
    timeout,
    [](boost::system::error_code const& ec, int status, std::string const& body) -> bool {
        if (ec) return false;  // Stop
        // Process response
        return true;  // Success
    },
    headers, journal);
```

### Lifetime Management with shared_from_this
```cpp
// Async callbacks use intrusive_ptr for safe lifetime
class HTTPClientImp : public HTTPClient,
    public std::enable_shared_from_this<HTTPClientImp> {
    void startAsync() {
        auto self = shared_from_this();  // Prevent premature destruction
        // ... schedule async operations ...
    }
};
```

### Deadline Timers
```cpp
// Always set timeouts for network operations
boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer_;
timer_.expires_after(std::chrono::seconds(timeout));
// Cancel pending operation on timeout
```

## Common Pitfalls
- Never start async operations without capturing shared_from_this - object may be destroyed before callback
- Always set deadline timers on network operations to prevent hanging connections
- Always set maximum response size limits to prevent resource exhaustion
- Never assume DNS resolution will succeed - handle resolver errors

## Key Files
- `include/xrpl/net/HTTPClient.h` - Main HTTP client interface
- `src/libxrpl/net/HTTPClient.cpp` - Implementation with async I/O
- `src/libxrpl/net/RegisterSSLCerts.cpp` - SSL certificate registration
- `include/xrpl/net/HTTPClientSSLContext.h` - SSL context management

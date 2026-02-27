# Resource Module Best Practices

## Description
Use when working with rate limiting and resource management in `src/libxrpl/resource/` or `include/xrpl/resource/`. Covers endpoint tracking, charge/fee management, and abuse prevention.

## Responsibility
Tracks resource consumption by network endpoints (inbound/outbound connections) to prevent abuse and ensure fair resource allocation. Manages consumer lifecycle, charge accounting, and gossip-based reputation sharing.

## Key Patterns

### Manager/Impl Pattern
```cpp
// Abstract Manager interface with factory function
class Manager { virtual ~Manager() = 0; };
std::unique_ptr<Manager> make_Manager(beast::Journal journal);

// Implementation hidden in cpp
class ManagerImp : public Manager { /* ... */ };
```

### Consumer Factory
```cpp
// Different consumer types for different connection sources
Consumer newInboundEndpoint(beast::IP::Endpoint const& address);
Consumer newOutboundEndpoint(beast::IP::Endpoint const& address);
Consumer newUnlimitedEndpoint(beast::IP::Endpoint const& address);
```

### Background Thread Shutdown
```cpp
// Clean shutdown pattern with condition variable
~ManagerImp() {
    {
        std::lock_guard lock(mutex_);
        stop_ = true;
        cond_.notify_one();
    }
    thread_.join();  // Wait for thread to finish
}
```

### Proxy-Aware Endpoint Detection
```cpp
// Handle forwarded-for headers safely
boost::system::error_code ec;
auto proxiedIp = boost::asio::ip::make_address(forwardedFor, ec);
if (ec) {
    journal_.warn() << "Invalid IP: " << ec.message();
    return newInboundEndpoint(address);  // Fallback to socket IP
}
```

## Common Pitfalls
- Never trust forwarded-for headers without proxy flag validation
- Always use the factory methods - never construct Consumer directly
- Always join background threads in destructor
- Use separate consumer types for different connection trust levels

## Key Files
- `include/xrpl/resource/ResourceManager.h` - Manager interface
- `src/libxrpl/resource/ResourceManager.cpp` - Implementation
- `include/xrpl/resource/Consumer.h` - Per-endpoint tracking
- `src/libxrpl/resource/Charge.cpp` - Cost definitions
- `src/libxrpl/resource/Fees.cpp` - Fee calculations

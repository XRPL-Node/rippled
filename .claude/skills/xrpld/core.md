# Core Module Best Practices

## Description
Use when working with core configuration and system services in `src/xrpld/core/`. Covers Config, time keeping, and network identity.

## Responsibility
System-level configuration and core services. The Config class extends BasicConfig from libxrpl with rippled-specific settings. Provides time keeping and network ID management.

## Key Patterns

### Config Class
```cpp
// Config extends BasicConfig from libxrpl
class Config : public BasicConfig {
    // SizedItem enum for memory-dependent configuration
    enum SizedItem { siSweepInterval, siNodeCacheSize, ... };

    // FeeSetup for fee parameters
    struct FeeSetup {
        XRPAmount reference_fee;
        XRPAmount account_reserve;
        XRPAmount owner_reserve;
    };

    // Access sections by name
    auto const& section = config["server"];
};
```

### Configuration Sections
```cpp
// Section names defined in ConfigSections.h
static constexpr char const* SECTION_NODE_DB = "node_db";
static constexpr char const* SECTION_VALIDATORS = "validators";
// Always use constants, never string literals
```

### Gradual Refactoring
```cpp
// Legacy config uses mixed patterns - refactoring is ongoing
// New code should follow the Section-based approach
// Deprecated patterns are documented with TODO comments
```

## Common Pitfalls
- Always use `ConfigSections.h` constants for section names
- Config extends BasicConfig - check both for available methods
- SizedItem values scale with node size configuration - don't hardcode sizes
- Never modify Config after Application startup

## Key Files
- `src/xrpld/core/Config.h` - Configuration object
- `src/xrpld/core/ConfigSections.h` - Section name constants
- `src/xrpld/core/TimeKeeper.h` - System time tracking
- `src/xrpld/core/NetworkIDServiceImpl.h` - Network ID implementation

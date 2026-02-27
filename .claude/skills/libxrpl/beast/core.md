# Beast Core Module Best Practices

## Description
Use when working with core beast utilities in `src/libxrpl/beast/core/` or `include/xrpl/beast/core/`. Covers semantic versioning and core type utilities.

## Responsibility
Provides core utilities including semantic version parsing/comparison, system abstractions, and foundational type support.

## Key Patterns

### Semantic Version Parsing
```cpp
// Parse version strings following semver spec
SemanticVersion version;
if (version.parse("1.2.3-beta")) {
    auto major = version.majorVersion;
    auto minor = version.minorVersion;
    auto patch = version.patchVersion;
}
// Supports comparison operators for version ordering
```

## Common Pitfalls
- Always validate version strings before using - parse() returns false on invalid input
- Remember that pre-release versions have lower precedence than release versions

## Key Files
- `include/xrpl/beast/core/SemanticVersion.h` - Version parsing and comparison
- `src/libxrpl/beast/core/SemanticVersion.cpp` - Implementation

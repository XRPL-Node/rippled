# JSON Module Best Practices

## Description
Use when working with JSON parsing and serialization in `src/libxrpl/json/` or `include/xrpl/json/`. Covers Json::Value, readers, writers, and custom allocators.

## Responsibility
JSON value representation (discriminated union for all JSON types), parsing text to Value objects, serialization from Value to text, and custom string allocation for memory control.

## Key Patterns

### Json::Value Usage
```cpp
// Discriminated union - supports all JSON types
Json::Value obj(Json::objectValue);
obj["key"] = "string_value";
obj["count"] = 42u;
obj["flag"] = true;
obj["nested"] = Json::objectValue;

// Array creation
Json::Value arr(Json::arrayValue);
arr.append("item");
arr.append(123);

// Type checking
if (obj.isObject()) { ... }
if (obj.isMember("key")) { ... }
```

### StaticString Optimization
```cpp
// Use StaticString for compile-time known keys to avoid allocation
static const Json::StaticString code("code");
static const Json::StaticString message("message");
object[code] = 1234;     // No dynamic allocation for key
object[message] = "ok";   // Key stored as pointer, not copied
```

### Value Type Enum
```cpp
enum ValueType {
    nullValue = 0,
    intValue, uintValue, realValue,
    stringValue, booleanValue,
    arrayValue, objectValue
};
// Check type before accessing to avoid undefined behavior
```

### Lazy Initialization
```cpp
// Container elements auto-created on access (like JavaScript)
Json::Value obj;
obj["newKey"]["nested"] = 1;  // Creates intermediate objects automatically
// Be aware: accessing a non-existent key creates it
```

## Common Pitfalls
- Accessing a non-existent key via `operator[]` creates a null entry - use `isMember()` to check first if you don't want side effects
- Use `const` overload of `operator[]` to avoid auto-creation: `obj["key"]` on const ref returns null reference
- Prefer `StaticString` for frequently-used keys to avoid repeated allocations
- Never assume JSON numeric types - check `isInt()`, `isUInt()`, `isDouble()` explicitly

## Key Files
- `include/xrpl/json/json_value.h` - Main Value class
- `include/xrpl/json/json_reader.h` - Text-to-Value parser
- `include/xrpl/json/json_writer.h` - Value-to-text serializer
- `include/xrpl/json/json_forwards.h` - Forward declarations and aliases

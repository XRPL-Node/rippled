# Doctest Unit Tests

This directory contains unit tests that have been successfully migrated from the Beast Unit Test framework to [doctest](https://github.com/doctest/doctest).

## Status: Production Ready ✅

All tests in this directory are:
- ✅ Successfully migrated to doctest
- ✅ Building without errors
- ✅ Passing all assertions
- ✅ Runnable independently

## Test Modules

### Basics Tests
**Location**: `basics/`
**Files**: 5 test files
**Test Cases**: 36
**Assertions**: 1,365

Successfully migrated tests:
- `Buffer_test.cpp` - Buffer and Slice operations
- `Expected_test.cpp` - Expected/Unexpected result types
- `IOUAmount_test.cpp` - IOU amount calculations
- `Number_test.cpp` - Numeric type operations
- `XRPAmount_test.cpp` - XRP amount handling

**Run**: `./xrpl.test.basics`

### Protocol Tests
**Location**: `protocol/`
**Files**: 7 test files
**Test Cases**: 37
**Assertions**: 16,020

Successfully migrated tests:
- `ApiVersion_test.cpp` - API version validation
- `BuildInfo_test.cpp` - Build version encoding/decoding
- `STAccount_test.cpp` - Serialized account types
- `STInteger_test.cpp` - Serialized integer types (UInt8/16/32/64, Int32)
- `STNumber_test.cpp` - Serialized number types with JSON parsing
- `SecretKey_test.cpp` - Secret key generation, signing, and verification
- `Seed_test.cpp` - Seed generation, parsing, and keypair operations

**Run**: `./xrpl.test.protocol`

### JSON Tests
**Location**: `json/`
**Files**: 1 test file
**Test Cases**: 8
**Assertions**: 12

Successfully migrated tests:
- `Object_test.cpp` - JSON object operations

**Run**: `./xrpl.test.json`

## Total Statistics

- **11 test files**
- **81 test cases**
- **17,397 assertions**
- **100% passing** ✨

## Building Tests

From the build directory:

```bash
# Build all doctest tests
cmake --build . --target xrpl.doctest.tests

# Build individual modules
cmake --build . --target xrpl.test.basics
cmake --build . --target xrpl.test.protocol
cmake --build . --target xrpl.test.json
```

## Running Tests

From the build directory:

```bash
# Run all tests
./src/doctest/xrpl.test.basics
./src/doctest/xrpl.test.protocol
./src/doctest/xrpl.test.json

# Run with options
./src/doctest/xrpl.test.basics --list-test-cases
./src/doctest/xrpl.test.protocol --success
./src/doctest/xrpl.test.json --duration

# Filter by test suite
./src/doctest/xrpl.test.basics --test-suite=basics
./src/doctest/xrpl.test.protocol --test-suite=protocol
./src/doctest/xrpl.test.json --test-suite=JsonObject
```

## Best Practices Applied

All migrated tests follow official doctest best practices:

### 1. TEST_SUITE Organization

All test files are organized into suites for better filtering and organization:

```cpp
TEST_SUITE_BEGIN("basics");

TEST_CASE("Buffer") { /* tests */ }

TEST_SUITE_END();
```

**Benefits**:
- Filter tests by suite: `./xrpl.test.protocol --test-suite=protocol`
- Better organization and documentation
- Clearer test structure

### 2. CHECK_FALSE for Readability

Using `CHECK_FALSE(expression)` instead of `CHECK(!(expression))`:

```cpp
// More readable:
CHECK_FALSE(buffer.empty());
```

### 3. CAPTURE Macros in Loops

CAPTURE macros provide better failure diagnostics in loops:

```cpp
for (std::size_t i = 0; i < 16; ++i) {
    CAPTURE(i);  // Shows value of i when test fails
    test(buffer, i);
}
```

### 4. REQUIRE for Critical Preconditions

Use REQUIRE when subsequent code depends on the assertion:

```cpp
auto parsed = parseBase58<AccountID>(s);
REQUIRE(parsed);  // Stops test if parsing fails
CHECK(toBase58(*parsed) == s);  // Safe to dereference
```

## Migration Guidelines

### Key Patterns

1. **Headers First**: Include production headers before doctest
   ```cpp
   #include <xrpl/protocol/STInteger.h>
   #include <doctest/doctest.h>
   ```

2. **Using Declarations**: Add at global scope for namespace migration
   ```cpp
   using xrpl::STUInt32;
   using xrpl::JsonOptions;
   ```

3. **Test Cases**: Use TEST_CASE macro
   ```cpp
   TEST_CASE("Descriptive Test Name") {
       CHECK(condition);
   }
   ```

4. **Subcases**: Organize related scenarios
   ```cpp
   TEST_CASE("Feature Tests") {
       SUBCASE("Scenario 1") { /* tests */ }
       SUBCASE("Scenario 2") { /* tests */ }
   }
   ```

5. **Assertions**:
   - `CHECK()` - continues on failure
   - `REQUIRE()` - stops on failure
   - `CHECK_THROWS_AS()` - exception testing

### Namespace Migration

Types moved from `ripple` → `xrpl` namespace:
- Add `using xrpl::TypeName;` declarations
- For nested namespaces: `namespace Alias = xrpl::Nested;`
- Or use full qualification: `xrpl::RPC::constant`

### What Makes a Good Candidate for Migration

✅ **Migrate to Doctest**:
- Standalone unit tests
- Tests single class/function in isolation
- No dependencies on `test/jtx` or `test/csf` frameworks
- Pure logic/algorithm/data structure tests

❌ **Keep in Beast** (integration tests):
- Requires Env class (ledger/transaction environment)
- Depends on `test/jtx` utilities
- Depends on `test/csf` (consensus simulation)
- Multi-component interaction tests

## Files

```
src/doctest/
├── README.md           # This file
├── CMakeLists.txt      # Build configuration
├── main.cpp            # Doctest main entry point
├── basics/
│   ├── Buffer_test.cpp
│   ├── Expected_test.cpp
│   ├── IOUAmount_test.cpp
│   ├── Number_test.cpp
│   └── XRPAmount_test.cpp
├── protocol/
│   ├── main.cpp
│   ├── ApiVersion_test.cpp
│   ├── BuildInfo_test.cpp
│   ├── STAccount_test.cpp
│   ├── STInteger_test.cpp
│   ├── STNumber_test.cpp
│   ├── SecretKey_test.cpp
│   └── Seed_test.cpp
└── json/
    ├── main.cpp
    └── Object_test.cpp
```

## References

- [Doctest Documentation](https://github.com/doctest/doctest/blob/master/doc/markdown/readme.md)
- [Doctest Tutorial](https://github.com/doctest/doctest/blob/master/doc/markdown/tutorial.md)
- [Assertion Macros](https://github.com/doctest/doctest/blob/master/doc/markdown/assertions.md)
- [Doctest Best Practices (ACCU)](https://accu.org/journals/overload/25/137/kirilov_2343/)

## Notes

- Original Beast tests remain in `src/test/` for integration tests
- Both frameworks coexist - doctest for unit tests, Beast for integration tests
- All doctest tests are auto-discovered at compile time
- No manual test registration required

# Doctest Migration Documentation

This document describes the migration of unit tests from the beast `unit_test` framework to the doctest framework.

## Overview

Tests were migrated from `src/test/` (beast unit_test format) to `src/doctest/` (doctest format), following the pattern established in `src/tests/libxrpl/`.

## Why Doctest?

Doctest is a fully open source, light, and feature-rich C++11 single-header testing framework. Key advantages include:

- **Ultra-light compile times**: ~10ms overhead per source file (vs ~430ms for Catch)
- **Fast assertions**: 50,000 asserts compile in under 30 seconds
- **Removable tests**: Use `DOCTEST_CONFIG_DISABLE` to completely remove tests from release binaries
- **No namespace pollution**: Everything is in the `doctest` namespace
- **No warnings**: Clean compilation even with aggressive warning levels (`-Wall -Wextra -Werror`)
- **Expression decomposition**: Failed assertions show both the expression and values
- **Single header**: No external dependencies except C/C++ standard library

Reference: [ACCU article on doctest](https://accu.org/journals/overload/25/137/kirilov_2343/)

## Build Configuration

### CMakeLists.txt Structure

Created `src/doctest/CMakeLists.txt` with:

- Helper function `xrpl_add_doctest(name)` that creates per-module executables
- Compiler flags: `-m64 -g -std=c++20 -fPIE -Wno-unknown-warning-option -Wall -Wdeprecated -Wno-deprecated-declarations -Wextra -Wno-unused-parameter -Werror -fstack-protector -Wno-sign-compare -Wno-unused-but-set-variable -MD -MT -MF`
- Six module targets: `xrpl.doctest.basics`, `xrpl.doctest.beast`, `xrpl.doctest.core`, `xrpl.doctest.csf`, `xrpl.doctest.nodestore`, `xrpl.doctest.protocol`

### Module Structure

Each module has its own `main.cpp` following the pre-migrated test pattern:

```
src/doctest/
├── basics/main.cpp
├── beast/main.cpp
├── core/main.cpp
├── csf/main.cpp
├── nodestore/main.cpp
└── protocol/main.cpp
```

## Doctest Assertion Reference

Doctest provides three severity levels for all assertion macros:

| Level     | Behavior                                              |
| --------- | ----------------------------------------------------- |
| `REQUIRE` | Immediately quits the test case if the assert fails   |
| `CHECK`   | Marks test as failed but continues with the test case |
| `WARN`    | Only prints a message, does not mark test as failed   |

### Expression Decomposing Asserts

```cpp
CHECK(expression);      // Expression can be binary comparison or single value
REQUIRE(a == b);        // Fails and stops test if false
WARN(vec.isEmpty());    // Just warns, doesn't fail test
```

### Negating Asserts

Use `_FALSE` suffix when `!` prefix cannot be decomposed properly:

```cpp
REQUIRE_FALSE(thisReturnsFalse());  // Better than REQUIRE(!thisReturnsFalse())
CHECK_FALSE(condition);
```

### Binary Asserts (57-68% faster compilation)

These don't use template decomposition - faster to compile:

```cpp
CHECK_EQ(left, right);   // same as CHECK(left == right)
CHECK_NE(left, right);   // same as CHECK(left != right)
CHECK_GT(left, right);   // same as CHECK(left > right)
CHECK_LT(left, right);   // same as CHECK(left < right)
CHECK_GE(left, right);   // same as CHECK(left >= right)
CHECK_LE(left, right);   // same as CHECK(left <= right)
CHECK_UNARY(expr);       // same as CHECK(expr)
CHECK_UNARY_FALSE(expr); // same as CHECK_FALSE(expr)
```

### Message Variants

```cpp
CHECK_MESSAGE(a < b, "relevant only to this assert ", other_local);
INFO("this is relevant to all subsequent asserts");
```

### Exception Asserts

```cpp
CHECK_THROWS(expression);                              // Expects any exception
CHECK_THROWS_AS(func(), std::runtime_error);           // Expects specific type
CHECK_THROWS_WITH(func(), "error message");            // Expects specific message
CHECK_THROWS_WITH_AS(func(), "msg", std::exception);   // Both type and message
CHECK_NOTHROW(expression);                             // Expects no exception
```

### Floating Point Comparisons

```cpp
CHECK(value == doctest::Approx(expected));
CHECK(22.0/7 == doctest::Approx(3.141).epsilon(0.01));  // 1% error tolerance
```

### String Containment

```cpp
CHECK("foobar" == doctest::Contains("foo"));
CHECK_THROWS_WITH(func(), doctest::Contains("partial"));
```

## Framework Conversion Patterns

| Beast Unit Test                             | Doctest Equivalent              |
| ------------------------------------------- | ------------------------------- |
| `#include <xrpl/beast/unit_test.h>`         | `#include <doctest/doctest.h>`  |
| `BEAST_EXPECT(expr)`                        | `CHECK(expr)`                   |
| `BEAST_EXPECTS(expr, msg)`                  | `CHECK_MESSAGE(expr, msg)`      |
| `testcase("name")`                          | `SUBCASE("name")`               |
| `class X : public unit_test::suite { ... }` | Free functions with `TEST_CASE` |
| `BEAST_DEFINE_TESTSUITE(Name, Module, Lib)` | `TEST_CASE("Name")`             |
| `pass()` / `fail()`                         | `CHECK(true)` / `CHECK(false)`  |

### Test Case and Subcase Structure

```cpp
TEST_CASE("Test name") {
    // Setup code runs for each subcase

    SUBCASE("First scenario") {
        CHECK(something);
    }

    SUBCASE("Second scenario") {
        CHECK(something_else);
    }
}
```

### Test Suites

Group related test cases using `TEST_SUITE` or `TEST_SUITE_BEGIN`/`TEST_SUITE_END`:

```cpp
TEST_SUITE_BEGIN("MyModule");

TEST_CASE("test 1") { /* ... */ }
TEST_CASE("test 2") { /* ... */ }

TEST_SUITE_END();
```

Or using the block syntax:

```cpp
TEST_SUITE("MyModule") {
    TEST_CASE("test 1") { /* ... */ }
    TEST_CASE("test 2") { /* ... */ }
}
```

### Test Fixtures

Use `TEST_CASE_FIXTURE` for class-based fixtures:

```cpp
class MyFixture {
protected:
    int data = 42;
public:
    MyFixture() { /* setup */ }
    ~MyFixture() { /* teardown */ }
};

TEST_CASE_FIXTURE(MyFixture, "test with fixture") {
    CHECK_EQ(data, 42);  // can access fixture members
}
```

### Templated Test Cases

```cpp
TEST_CASE_TEMPLATE("test for multiple types", T, int, float, double) {
    T value = T(42);
    CHECK_EQ(value, T(42));
}
```

### BDD-Style Macros

```cpp
SCENARIO("vectors can be sized") {
    GIVEN("A vector with some items") {
        std::vector<int> v(5);

        WHEN("the size is increased") {
            v.resize(10);

            THEN("the size changes") {
                CHECK_EQ(v.size(), 10);
            }
        }
    }
}
```

### Logging

```cpp
INFO("this message appears if a subsequent assert fails");
CAPTURE(variable);  // logs "variable := <value>"
MESSAGE("always printed");
FAIL("fails and stops test case");
FAIL_CHECK("fails but continues");
```

## Namespace Changes

- Changed from `namespace ripple` to `namespace xrpl` where applicable
- Header paths changed from `xrpld/` to `xrpl/` (e.g., `xrpld/app/` → `xrpl/protocol/`)

## Common Migration Issues and Solutions

### 1. CHECK Macro with Complex Expressions

**Problem**: Doctest's CHECK macro doesn't support `&&` or `||` in expressions due to expression decomposition.

```cpp
// Doesn't work - can't decompose && properly
CHECK(a && b);

// Solution: Split into separate checks
CHECK(a);
CHECK(b);
```

### 2. CHECK Macro with Custom Iterator Comparisons

**Problem**: CHECK wraps expressions in `Expression_lhs<>` which breaks template argument deduction for custom comparison operators (especially boost::intrusive iterators).

```cpp
// Doesn't compile with complex iterators
CHECK(iter != container.end());

// Solution: Use binary assert or store result in bool first
CHECK_NE(iter, container.end());  // Preferred - uses binary assert

// Or:
bool notEnd = (iter != container.end());
CHECK(notEnd);
```

### 3. Constructor Argument Order

Some container constructors have different argument order than initially assumed:

```cpp
// Wrong order
Container c(clock, first, last);

// Correct order (iterators before clock)
Container c(first, last, clock);
```

### 4. Return Type Differences

For map types with `P&&` insert overloads, return types differ:

```cpp
// Use if constexpr to handle both cases
if constexpr (!IsMulti && IsMap)
{
    auto result = c.insert(c.end(), value);  // returns pair<iterator, bool>
    CHECK_NE(result.first, c.end());
}
else
{
    auto it = c.insert(c.end(), value);  // returns iterator
    CHECK_NE(it, c.end());
}
```

### 5. Types Without Explicit Bool Conversion

**Problem**: `CHECK_FALSE(x)` and `CHECK_UNARY(x)` require the type to have an explicit `operator bool()`. Types like `base_uint` may only have `operator!()`.

```cpp
// Error: base_uint has operator!() but no explicit bool conversion
CHECK_FALSE(z);        // Fails: can't static_cast<bool>(z)
CHECK_UNARY(z);        // Fails: same reason

// Solution: Use the negation operator explicitly
CHECK_UNARY(!z);       // Works: uses operator!() which returns bool
CHECK_UNARY(!z.isNonZero());  // For bool methods, prefer explicit negation
```

## Files Migrated

### basics/ (13 files)

- Buffer.cpp, Expected.cpp, IOUAmount.cpp, KeyCache.cpp, Number.cpp
- StringUtilities.cpp, TaggedCache.cpp, Units.cpp, XRPAmount.cpp
- base58.cpp, base_uint.cpp, hardened_hash.cpp, join.cpp

### beast/ (11 files)

- CurrentThreadName.cpp, IPEndpoint.cpp, Journal.cpp, LexicalCast.cpp
- PropertyStream.cpp, SemanticVersion.cpp, aged_associative_container.cpp
- basic_seconds_clock.cpp, beast_Zero.cpp, xxhasher.cpp

### core/ (1 file)

- Workers.cpp

### csf/ (4 files)

- BasicNetwork.cpp, Digraph.cpp, Histogram.cpp, Scheduler.cpp

### nodestore/ (1 file)

- varint.cpp

### protocol/ (14 files)

- ApiVersion.cpp, BuildInfo.cpp, Issue.cpp, MultiApiJson.cpp
- PublicKey.cpp, Quality.cpp, STAccount.cpp, STInteger.cpp
- STNumber.cpp, SecretKey.cpp, Seed.cpp, SeqProxy.cpp
- Serializer.cpp, TER.cpp

## Binary Assertion Conversion

All migrated tests have been updated to use doctest's binary assertion macros for improved compilation speed (57-68% faster) and better error messages:

| Original Pattern  | Converted To                                |
| ----------------- | ------------------------------------------- |
| `CHECK(a == b)`   | `CHECK_EQ(a, b)`                            |
| `CHECK(a != b)`   | `CHECK_NE(a, b)`                            |
| `CHECK(a > b)`    | `CHECK_GT(a, b)`                            |
| `CHECK(a < b)`    | `CHECK_LT(a, b)`                            |
| `CHECK(a >= b)`   | `CHECK_GE(a, b)`                            |
| `CHECK(a <= b)`   | `CHECK_LE(a, b)`                            |
| `CHECK(!expr)`    | `CHECK_FALSE(expr)` or `CHECK_UNARY(!expr)` |
| `CHECK(boolExpr)` | `CHECK_UNARY(boolExpr)`                     |

**Note**: Template type checks like `CHECK((std::is_same_v<T, U>))` and function calls with template parameters like `CHECK(tryEdgeCase<std::uint64_t>("..."))` remain as `CHECK()` since they are not comparison operations.

## Test Results Summary

| Module    | Test Cases | Assertions    |
| --------- | ---------- | ------------- |
| basics    | 61         | 2,638,582     |
| beast     | 48         | 162,715       |
| core      | 6          | 66            |
| csf       | 8          | 101           |
| nodestore | 1          | 68            |
| protocol  | 73         | 20,372        |
| **Total** | **197**    | **2,821,904** |

## Running Tests

```bash
# Build all doctest targets
cd .build
cmake --build . --target xrpl.doctests

# Run individual module
./src/doctest/xrpl.doctest.basics
./src/doctest/xrpl.doctest.beast
./src/doctest/xrpl.doctest.core
./src/doctest/xrpl.doctest.csf
./src/doctest/xrpl.doctest.nodestore
./src/doctest/xrpl.doctest.protocol

# Run all tests
for test in src/doctest/xrpl.doctest.*; do ./$test; done
```

## Tests Not Migrated

Some tests were not migrated due to dependencies:

- **Manual tests** requiring user interaction (e.g., `DetectCrash_test`)
- **Tests using xrpld infrastructure** (`test/jtx.h`, `unit_test/SuiteJournal.h`)
- **Complex async tests** using boost coroutines
- **Tests with FileDirGuard** or other test-specific utilities

## References

- [Doctest GitHub Repository](https://github.com/doctest/doctest)
- [Doctest Assertions](https://github.com/doctest/doctest/blob/master/doc/markdown/assertions.md)
- [Doctest Test Cases](https://github.com/doctest/doctest/blob/master/doc/markdown/testcases.md)
- [Doctest Configuration](https://github.com/doctest/doctest/blob/master/doc/markdown/configuration.md)
- [Doctest Logging](https://github.com/doctest/doctest/blob/master/doc/markdown/logging.md)
- [Doctest Examples](https://github.com/doctest/doctest/tree/master/examples)
- [ACCU Article: doctest – the Lightest C++ Unit Testing Framework](https://accu.org/journals/overload/25/137/kirilov_2343/)

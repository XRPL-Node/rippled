# Doctest Migration Documentation

This document describes the migration of unit tests from the beast `unit_test` framework to the doctest framework.

## Overview

Tests were migrated from `src/test/` (beast unit_test format) to `src/doctest/` (doctest format), following the pattern established in `src/tests/libxrpl/`.

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

## Framework Conversion Patterns

| Beast Unit Test | Doctest Equivalent |
|-----------------|-------------------|
| `#include <xrpl/beast/unit_test.h>` | `#include <doctest/doctest.h>` |
| `BEAST_EXPECT(expr)` | `CHECK(expr)` |
| `BEAST_EXPECTS(expr, msg)` | `CHECK_MESSAGE(expr, msg)` |
| `testcase("name")` | `SUBCASE("name")` |
| `class X : public unit_test::suite { ... }` | Free functions with `TEST_CASE` |
| `BEAST_DEFINE_TESTSUITE(Name, Module, Lib)` | `TEST_CASE("Name")` |
| `pass()` / `fail()` | `CHECK(true)` / `CHECK(false)` |

## Namespace Changes

- Changed from `namespace ripple` to `namespace xrpl` where applicable
- Header paths changed from `xrpld/` to `xrpl/` (e.g., `xrpld/app/` → `xrpl/protocol/`)

## Common Migration Issues and Solutions

### 1. CHECK Macro with Complex Expressions

**Problem**: Doctest's CHECK macro doesn't support `&&` or `||` in expressions.

```cpp
// Doesn't work
CHECK(a && b);

// Solution: Split into separate checks
CHECK(a);
CHECK(b);
```

### 2. CHECK Macro with Custom Iterator Comparisons

**Problem**: CHECK wraps expressions in `Expression_lhs<>` which breaks template argument deduction for custom comparison operators (especially boost::intrusive iterators).

```cpp
// Doesn't compile
CHECK(iter != container.end());

// Solution: Store result in bool first
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
    CHECK(result.first != c.end());
}
else
{
    auto it = c.insert(c.end(), value);  // returns iterator
    CHECK(it != c.end());
}
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

## Test Results Summary

| Module | Test Cases | Assertions |
|--------|------------|------------|
| basics | 61 | 2,638,582 |
| beast | 48 | 162,715 |
| core | 6 | 66 |
| csf | 8 | 101 |
| nodestore | 1 | 68 |
| protocol | 73 | 20,372 |
| **Total** | **197** | **2,821,904** |

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


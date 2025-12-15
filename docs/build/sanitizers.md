# Sanitizer Configuration for Rippled

This document explains how to properly configure and run sanitizers (AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer) with the rippled project.
Corresponding suppression files are located in the `sanitizers/suppressions` directory.

- [Sanitizer Configuration for Rippled](#sanitizer-configuration-for-rippled)
  - [Building with Sanitizers](#building-with-sanitizers)
    - [Summary](#summary)
    - [Build steps:](#build-steps)
      - [Install dependencies](#install-dependencies)
      - [AddressSanitizer (ASan) + UndefinedBehaviorSanitizer (UBSan)](#addresssanitizer-asan--undefinedbehaviorsanitizer-ubsan)
      - [ThreadSanitizer (TSan) + UndefinedBehaviorSanitizer (UBSan)](#threadsanitizer-tsan--undefinedbehaviorsanitizer-ubsan)
      - [Just AddressSanitizer (ASan)](#just-addresssanitizer-asan)
      - [Just UndefinedBehaviorSanitizer (UBSan)](#just-undefinedbehaviorsanitizer-ubsan)
      - [Build](#build)
  - [Running Tests with Sanitizers](#running-tests-with-sanitizers)
    - [AddressSanitizer (ASan)](#addresssanitizer-asan)
    - [ThreadSanitizer (TSan) + UndefinedBehaviorSanitizer (UBSan)](#threadsanitizer-tsan--undefinedbehaviorsanitizer-ubsan-1)
    - [LeakSanitizer (LSan)](#leaksanitizer-lsan)
  - [Suppression Files](#suppression-files)
    - [`asan.supp`](#asansupp)
    - [`lsan.supp`](#lsansupp)
    - [`ubsan.supp`](#ubsansupp)
    - [`tsan.supp`](#tsansupp)
    - [`sanitizer-ignorelist.txt`](#sanitizer-ignorelisttxt)
  - [Troubleshooting](#troubleshooting)
    - ["ASan is ignoring requested \_\_asan_handle_no_return" warnings](#asan-is-ignoring-requested-__asan_handle_no_return-warnings)
    - [Sanitizer Mismatch Errors](#sanitizer-mismatch-errors)
  - [References](#references)

## Building with Sanitizers

### Summary

Follow the same instructions as mentioned in [BUILD.md](../../BUILD.md) but with the following changes:

1. Make sure you have a clean build directory.
2. Set the `SANITIZERS` environment variable when running CMake to enable sanitizers.
3. Optionally use `--profile sanitizers` with Conan to build dependencies with sanitizer instrumentation.
4. Set `ASAN_OPTIONS`, `LSAN_OPTIONS`, `UBSAN_OPTIONS` and `TSAN_OPTIONS` environment variables to configure sanitizer behavior when running executables.

---

### Build steps:

```bash
cd /path/to/rippled
rm -rf .build
mkdir .build
cd .build
```

#### Install dependencies

The `SANITIZERS` environment variable is used by both Conan and CMake.

```bash
# Standard build (without instrumenting dependencies)
SANITIZERS=Address,UndefinedBehavior conan install .. --output-folder . --build missing --settings build_type=Debug

# Or with sanitizer-instrumented dependencies (takes longer but fewer false positives)
SANITIZERS=Address,UndefinedBehavior conan install .. --output-folder . --profile:all sanitizers --build missing --settings build_type=Debug
```

#### AddressSanitizer (ASan) + UndefinedBehaviorSanitizer (UBSan)

Set the `SANITIZERS` environment variable when running CMake:

```bash
SANITIZERS=Address,UndefinedBehavior cmake .. -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE:FILEPATH=build/generators/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -Dtests=ON -Dxrpld=ON
```

#### ThreadSanitizer (TSan) + UndefinedBehaviorSanitizer (UBSan)

```bash
SANITIZERS=Thread,UndefinedBehavior cmake .. -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE:FILEPATH=build/generators/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -Dtests=ON -Dxrpld=ON
```

#### Just AddressSanitizer (ASan)

```bash
SANITIZERS=Address cmake .. -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE:FILEPATH=build/generators/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -Dtests=ON -Dxrpld=ON
```

#### Just UndefinedBehaviorSanitizer (UBSan)

```bash
SANITIZERS=UndefinedBehavior cmake .. -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE:FILEPATH=build/generators/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -Dtests=ON -Dxrpld=ON
```

**Note:** Do not mix Address and Thread sanitizers - they are incompatible.

#### Build

```bash
cmake --build . --parallel 4
```

## Running Tests with Sanitizers

### AddressSanitizer (ASan)

**IMPORTANT**: ASan with Boost produces many false positives. Use these options:

```bash
export ASAN_OPTIONS="detect_container_overflow=0 suppressions=path/to/asan.supp halt_on_error=0 log_path=asan.log"
export UBSAN_OPTIONS="suppressions=path/to/ubsan.supp print_stacktrace=1 halt_on_error=0 log_path=ubsan.log"
export LSAN_OPTIONS="suppressions=path/to/lsan.supp halt_on_error=0 log_path=lsan.log"

# Run tests
./rippled --unittest --unittest-jobs=5
```

**Why `detect_container_overflow=0`?**

- Boost intrusive containers (used in `aged_unordered_container`) trigger false positives
- Boost context switching (used in `Workers.cpp`) confuses ASan's stack tracking
- Since we usually don't build boost(because we don't want to instrument boost and detect issues in boost code) with asan but use boost containers in ASAN instrumented rippled code, it generates false positives.
- See: https://github.com/google/sanitizers/wiki/AddressSanitizerContainerOverflow

### ThreadSanitizer (TSan) + UndefinedBehaviorSanitizer (UBSan)

```bash
export TSAN_OPTIONS="suppressions=path/to/tsan.supp halt_on_error=0 log_path=tsan.log"

# Run tests
./rippled --unittest --unittest-jobs=5
```

### LeakSanitizer (LSan)

LSan is automatically enabled with ASan. To disable it:

```bash
export ASAN_OPTIONS="detect_leaks=0"
```

## Suppression Files

### `asan.supp`

- **Purpose**: Suppress AddressSanitizer (ASan) errors only
- **Format**: `interceptor_name:<pattern>` where pattern matches file names. Supported suppression types are:
  - interceptor_name
  - interceptor_via_fun
  - interceptor_via_lib
  - odr_violation
- **More info**: [AddressSanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- **Note**: Cannot suppress stack-buffer-overflow, container-overflow, etc.

### `lsan.supp`

- **Purpose**: Suppress LeakSanitizer (LSan) errors only
- **Format**: `leak:<pattern>` where pattern matches function/file names
- **More info**: [LeakSanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizerLeakSanitizer)

### `ubsan.supp`

- **Purpose**: Suppress UndefinedBehaviorSanitizer errors
- **Format**: `<error_type>:<pattern>` (e.g., `unsigned-integer-overflow:protobuf`)
- **Covers**: Intentional overflows in sanitizers/suppressions libraries (protobuf, gRPC, stdlib)

### `tsan.supp`

- **Purpose**: Suppress ThreadSanitizer data race warnings
- **Format**: `race:<pattern>` where pattern matches function/file names
- **More info**: [ThreadSanitizerSuppressions](https://github.com/google/sanitizers/wiki/ThreadSanitizerSuppressions)

### `sanitizer-ignorelist.txt`

- **Purpose**: Compile-time ignorelist for all sanitizers
- **Usage**: Passed via `-fsanitize-ignorelist=absolute/path/to/sanitizer-ignorelist.txt`
- **Format**: `<level>:<pattern>` (e.g., `src:Workers.cpp`)

## Troubleshooting

### "ASan is ignoring requested \_\_asan_handle_no_return" warnings

These warnings appear when using Boost context switching and are harmless. They indicate potential false positives.

### Sanitizer Mismatch Errors

If you see undefined symbols like `___tsan_atomic_load` when building with ASan:

**Problem**: Dependencies were built with a different sanitizer than the main project.

**Solution**: Rebuild everything with the same sanitizer:

```bash
rm -rf .build
# Then follow the build instructions above
```

Then review the log files: `asan.log.*`, `ubsan.log.*`, `tsan.log.*`

## References

- [AddressSanitizer Wiki](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- [AddressSanitizer Flags](https://github.com/google/sanitizers/wiki/AddressSanitizerFlags)
- [Container Overflow Detection](https://github.com/google/sanitizers/wiki/AddressSanitizerContainerOverflow)
- [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
- [ThreadSanitizer](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)

# Sanitizer Configuration for Rippled

This document explains how to properly configure and run sanitizers (AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer) with the rippled project.
Corresponding suppression files are located in the `sanitizers/suppressions` directory.

- [Sanitizer Configuration for Rippled](#sanitizer-configuration-for-rippled)
  - [Building with Sanitizers](#building-with-sanitizers)
    - [AddressSanitizer (ASan) + UndefinedBehaviorSanitizer (UBSan)](#addresssanitizer-asan--undefinedbehaviorsanitizer-ubsan)
    - [ThreadSanitizer (TSan) + UndefinedBehaviorSanitizer (UBSan)](#threadsanitizer-tsan--undefinedbehaviorsanitizer-ubsan)
- [Running Tests with Sanitizers](#running-tests-with-sanitizers)
  - [AddressSanitizer (ASan)](#addresssanitizer-asan)
  - [ThreadSanitizer (TSan)](#threadsanitizer-tsan)
  - [LeakSanitizer (LSan)](#leaksanitizer-lsan)
- [Suppression Files](#suppression-files)
  - [asan.supp](#asansupp)
  - [lsan.supp](#lsansupp)
  - [ubsan.supp](#ubsansupp)
  - [tsan.supp](#tsansupp)
  - [Ignorelist](#sanitizer-ignorelisttxt)
- [Known False Positives](#known-false-positives)
- [References](#references)

## Building with Sanitizers

### Summary

Follow the same instructions as mentioned in [BUILD.md](../../BUILD.md) but with following changes:

1. Make sure you have clean build directory.
2. Use `--profile sanitizers` to configure build options to include sanitizer flags. [sanitizes](../../conan/profiles/sanitizers) profile contains settings for all sanitizers.
3. Set `ASAN_OPTIONS`, `LSAN_OPTIONS` ,`UBSAN_OPTIONS` and `TSAN_OPTIONS` environment variables to configure sanitizer behavior when running executables.

---

### AddressSanitizer (ASan) + UndefinedBehaviorSanitizer (UBSan)

```bash
cd /path/to/rippled
rm -rf .build
mkdir .build
cd .build

# Build with AddressSanitizer. This also builds rippled with UndefinedBehavior sanitizer.
SANITIZERS=Address conan install .. --output-folder . --profile sanitizers --build missing --settings build_type=Release
# Use `--profile:all sanitizers` if you would like to build all dependencies and libraries (boost etc.) with sanitizers. This might take long time but you won't see some false-positives on sanitizer reports since whole binary will be instrumented.

# To build with Thread+UndefinedBehavior Sanitizer, replace `SANITIZERS=Address` with `SANITIZERS=Thread`.

# Configure CMake
cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE:FILEPATH=build/generators/conan_toolchain.cmake -Dunity=ON -Dtests=ON -Dxrpld=ON

# Build
cmake --build . --parallel 4
```

### ThreadSanitizer (TSan) + UndefinedBehaviorSanitizer (UBSan)

```bash
cd /path/to/rippled
rm -rf .build
mkdir .build
cd .build

# Build dependencies with Thread sanitizer
SANITIZERS=Thread conan install .. --output-folder . --profile sanitizers --build missing --settings build_type=Release

# Configure CMake
cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE:FILEPATH=build/generators/conan_toolchain.cmake -Dunity=ON -Dtests=ON -Dxrpld=ON -DCMAKE_BUILD_TYPE=Release

# Build
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

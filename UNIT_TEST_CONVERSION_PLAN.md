# Unit Test Conversion Plan

## Strategy: Hybrid Approach

Convert only **standalone unit tests** to doctest, while keeping **integration tests** in the original Beast framework.

## Classification Criteria

### Unit Tests (Convert to Doctest)
- ✅ Test a single class/function in isolation
- ✅ No dependencies on test/jtx framework
- ✅ No dependencies on test/csf framework
- ✅ Don't require Env, Config, or Ledger setup
- ✅ Pure logic/algorithm/data structure tests

### Integration Tests (Keep in Beast)
- ❌ Require Env class (ledger/transaction environment)
- ❌ Require test/jtx utilities
- ❌ Require test/csf (consensus simulation)
- ❌ Multi-component interaction tests
- ❌ End-to-end workflow tests

## Test Module Analysis

### ✅ Basics - CONVERT (Mostly Unit Tests)
**Location**: `src/doctest/basics/`
**Status**: Partially working
**Action**:
- Keep: Most files (Buffer, Expected, DetectCrash, IOUAmount, XRPAmount, etc.)
- Exclude: FileUtilities_test.cpp (needs test/unit_test/FileDirGuard.h)

### ✅ Protocol - CONVERT (Many Unit Tests)
**Location**: `src/doctest/protocol/`
**Status**: Partially working
**Action**:
- Keep: ApiVersion, BuildInfo, SecretKey, Seed, SeqProxy, Serializer, TER, STInteger, STNumber, STAccount, STTx
- Exclude: All tests requiring test/jtx (9 files)
- Fix: MultiApiJson (if CHECK pattern issues), PublicKey, Quality (add missing helpers)

### ✅ Conditions - CONVERT
**Location**: `src/doctest/conditions/`
**Status**: Should work
**Action**: Test build

### ✅ JSON - CONVERT
**Location**: `src/doctest/json/`
**Status**: Should work
**Action**: Test build

### ❌ App - KEEP IN BEAST (Integration Tests)
**Location**: `src/test/app/`
**Reason**: All 71 files depend on test/jtx framework
**Action**: Leave in original location

### ❌ RPC - KEEP IN BEAST (Integration Tests)
**Location**: `src/test/rpc/`
**Reason**: All 48 files depend on test/jtx framework
**Action**: Leave in original location

### ❌ JTX - KEEP IN BEAST (Test Utilities)
**Location**: `src/test/jtx/`
**Reason**: These ARE the test utilities
**Action**: Leave in original location

### ❓ Beast - EVALUATE
**Location**: `src/doctest/beast/`
**Status**: Not properly converted
**Action**: Check each file individually:
- IPEndpoint_test.cpp - depends on test/beast/IPEndpointCommon.h (EXCLUDE)
- LexicalCast_test.cpp - has class structure, uses testcase() (FIX or EXCLUDE)
- Other files - evaluate case by case

### ❌ Consensus - KEEP IN BEAST
**Location**: `src/test/consensus/`
**Reason**: Depends on test/csf framework
**Action**: Leave in original location

### ❌ Core - KEEP IN BEAST
**Location**: `src/test/core/`
**Reason**: Depends on test/jtx framework
**Action**: Leave in original location

### ❌ CSF - KEEP IN BEAST
**Location**: `src/test/csf/`
**Reason**: These tests use/test the CSF framework
**Action**: Leave in original location

### ❓ Ledger - EVALUATE
**Location**: `src/doctest/ledger/`
**Status**: Unknown
**Action**: Check dependencies, likely many need test/jtx

### ❓ Nodestore - EVALUATE
**Location**: `src/doctest/nodestore/`
**Status**: Unknown
**Action**: Check dependencies

### ❓ Overlay - EVALUATE
**Location**: `src/doctest/overlay/`
**Status**: Unknown
**Action**: Check dependencies

### ❓ Peerfinder - EVALUATE
**Location**: `src/doctest/peerfinder/`
**Status**: Unknown
**Action**: Check dependencies

### ❓ Resource - EVALUATE
**Location**: `src/doctest/resource/`
**Status**: Unknown
**Action**: Check dependencies

### ❓ Server - EVALUATE
**Location**: `src/doctest/server/`
**Status**: Unknown
**Action**: Check dependencies

### ❓ SHAMap - EVALUATE
**Location**: `src/doctest/shamap/`
**Status**: Unknown
**Action**: Check dependencies

### ❓ Unit_test - EVALUATE
**Location**: `src/doctest/unit_test/`
**Status**: Unknown
**Action**: These may be test utilities themselves

## Implementation Steps

### Phase 1: Fix Known Working Modules (1-2 hours)
1. ✅ Fix basics tests (exclude FileUtilities_test.cpp)
2. ✅ Fix protocol tests that should work (ApiVersion, BuildInfo already working)
3. ✅ Test conditions module
4. ✅ Test json module
5. Update CMakeLists.txt to only build confirmed working modules

### Phase 2: Evaluate Remaining Modules (2-3 hours)
1. Check each "EVALUATE" module for test/jtx dependencies
2. Create include/exclude lists for each module
3. Identify which files are true unit tests

### Phase 3: Fix Unit Tests (Variable time)
1. For each identified unit test file:
   - Fix any remaining Beast→doctest conversion issues
   - Add missing helper functions if needed
   - Ensure it compiles standalone
2. Update CMakeLists.txt incrementally

### Phase 4: Cleanup (1 hour)
1. Move integration tests back to src/test/ if they were copied
2. Update documentation
3. Clean up src/doctest/ to only contain unit tests
4. Update build system

## Expected Outcome

- **~50-100 true unit tests** converted to doctest (rough estimate)
- **~180-230 integration tests** remain in Beast framework
- Clear separation between unit and integration tests
- Both frameworks coexist peacefully

## Build System Structure

```
src/
├── test/                          # Beast framework (integration tests)
│   ├── app/                       # 71 files - ALL integration tests
│   ├── rpc/                       # 48 files - ALL integration tests
│   ├── jtx/                       # Test utilities
│   ├── csf/                       # Consensus simulation framework
│   ├── consensus/                 # Integration tests
│   ├── core/                      # Integration tests
│   └── [other integration tests]
│
└── doctest/                       # Doctest framework (unit tests only)
    ├── basics/                    # ~15-16 unit tests
    ├── protocol/                  # ~12-14 unit tests
    ├── conditions/                # ~1 unit test
    ├── json/                      # ~1 unit test
    └── [other unit test modules TBD]
```

## Next Immediate Actions

1. Test build basics module (exclude FileUtilities)
2. Test build protocol module (with current exclusions)
3. Test build conditions module
4. Test build json module
5. Create comprehensive scan of remaining modules

---

**Status**: Ready to implement Phase 1
**Updated**: December 11, 2024

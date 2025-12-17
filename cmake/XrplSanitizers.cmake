#[===================================================================[
   Configure sanitizers based on environment variables.

   This module reads the following environment variables:
   - SANITIZERS: The sanitizers to enable. Possible values:
     - "Address"
     - "Address,UndefinedBehavior"
     - "Thread"
     - "Thread,UndefinedBehavior"
     - "UndefinedBehavior"

   The compiler type and platform are detected automatically by CMake.
   The sanitizer compile options are applied to the 'common' interface library
   which is linked to all targets in the project.
#]===================================================================]

# Read environment variable
set(SANITIZERS $ENV{SANITIZERS})

if(NOT SANITIZERS)
  return()
endif()

message(STATUS "Configuring sanitizers: ${SANITIZERS}")

# Parse SANITIZERS value to determine which sanitizers to enable
set(ENABLE_ASAN FALSE)
set(ENABLE_TSAN FALSE)
set(ENABLE_UBSAN FALSE)

# Normalize SANITIZERS into a list
set(_san_list "${SANITIZERS}")
string(REPLACE "," ";" _san_list "${_san_list}")
separate_arguments(_san_list)

set(REMAINING_SANITIZERS "")
foreach(_san IN LISTS _san_list)
  if(_san STREQUAL "Address")
    set(ENABLE_ASAN TRUE)
  elseif(_san STREQUAL "Thread")
    set(ENABLE_TSAN TRUE)
  elseif(_san STREQUAL "UndefinedBehavior")
    set(ENABLE_UBSAN TRUE)
  else()
    list(APPEND REMAINING_SANITIZERS "${_san}")
  endif()
endforeach()

if(REMAINING_SANITIZERS)
  message(FATAL_ERROR
    "Unknown SANITIZERS value(s): ${REMAINING_SANITIZERS}. "
    "Supported: Address, Thread, UndefinedBehavior and their combinations.")
endif()

# Detect compiler type
set(IS_GCC FALSE)
set(IS_CLANG FALSE)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(IS_GCC TRUE)
    message(STATUS "  Compiler: GCC ${CMAKE_CXX_COMPILER_VERSION}")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(IS_CLANG TRUE)
    message(STATUS "  Compiler: Clang ${CMAKE_CXX_COMPILER_VERSION}")
endif()

# Detect platform (amd64/x86_64 vs arm64/aarch64)
set(IS_AMD64 FALSE)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(IS_AMD64 TRUE)
    message(STATUS "  Platform: amd64")
else()
    message(STATUS "  Platform: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Frame pointer is required for meaningful stack traces. Sanitizers recommend minimum of -O1 for reasonable performance
set(SANITIZERS_COMPILE_FLAGS "-fno-omit-frame-pointer" "-O1")

# Build the sanitizer flags list
set(SANITIZERS_FLAGS)

if(ENABLE_ASAN)
    list(APPEND SANITIZERS_FLAGS "address")
elseif(ENABLE_TSAN)
    list(APPEND SANITIZERS_FLAGS "thread")
endif()

if(ENABLE_UBSAN)
    # UB sanitizer flags
    if(IS_CLANG)
        # Clang supports additional UB checks
        list(APPEND SANITIZERS_FLAGS "undefined" "float-divide-by-zero" "unsigned-integer-overflow")
    else()
        list(APPEND SANITIZERS_FLAGS "undefined" "float-divide-by-zero")
    endif()
endif()

# Configure code model for GCC on amd64
# Use large code model for ASAN to avoid relocation errors
# Use medium code model for TSAN (large is not compatible with TSAN)
set(SANITIZERS_RELOCATION_FLAGS)
if(IS_GCC AND IS_AMD64)
    if(ENABLE_ASAN)
        message(STATUS "  Using large code model (-mcmodel=large)")
        list(APPEND SANITIZERS_COMPILE_FLAGS "-mcmodel=large")
        list(APPEND SANITIZERS_RELOCATION_FLAGS "-mcmodel=large")
    elseif(ENABLE_TSAN)
        message(STATUS "  Using medium code model (-mcmodel=medium)")
        list(APPEND SANITIZERS_COMPILE_FLAGS "-mcmodel=medium")
        list(APPEND SANITIZERS_RELOCATION_FLAGS "-mcmodel=medium")
    endif()
endif()

# Compiler-specific configuration
if(IS_GCC)
    # Disable mold, gold and lld linkers for GCC with sanitizers
    # Use default linker (bfd/ld) which is more lenient with mixed code models
    set(use_mold OFF CACHE BOOL "Use mold linker" FORCE)
    set(use_gold OFF CACHE BOOL "Use gold linker" FORCE)
    set(use_lld OFF CACHE BOOL "Use lld linker" FORCE)
    message(STATUS "  Disabled mold, gold, and lld linkers for GCC with sanitizers")

    # Suppress false positive warnings in GCC with stringop-overflow
    list(APPEND SANITIZERS_COMPILE_FLAGS "-Wno-stringop-overflow")

    if(ENABLE_TSAN)
        # GCC doesn't support atomic_thread_fence with tsan. Suppress warnings.
        list(APPEND SANITIZERS_COMPILE_FLAGS "-Wno-tsan")
    endif()

    if(ENABLE_ASAN)
        # GCC has issues with PIC and ASAN.
        # https://github.com/google/sanitizers/issues/856
        list(APPEND SANITIZERS_COMPILE_FLAGS "-fno-pic")
        list(APPEND SANITIZERS_RELOCATION_FLAGS "-fno-pic")
        list(APPEND SANITIZERS_COMPILE_FLAGS "-fno-pie")
        list(APPEND SANITIZERS_RELOCATION_FLAGS "-fno-pie")
    endif()

    # Join sanitizer flags with commas for -fsanitize option
    list(JOIN SANITIZERS_FLAGS "," SANITIZERS_FLAGS_STR)

    # Add sanitizer to compile and link flags
    list(APPEND SANITIZERS_COMPILE_FLAGS "-fsanitize=${SANITIZERS_FLAGS_STR}")
    set(SANITIZERS_LINK_FLAGS "${SANITIZERS_RELOCATION_FLAGS}" "-fsanitize=${SANITIZERS_FLAGS_STR}")

elseif(IS_CLANG)
    # Add ignorelist for Clang (GCC doesn't support this)
    # Use CMAKE_SOURCE_DIR to get the path to the ignorelist
    set(IGNORELIST_PATH "${CMAKE_SOURCE_DIR}/sanitizers/suppressions/sanitizer-ignorelist.txt")
    if(NOT EXISTS "${IGNORELIST_PATH}")
        message(FATAL_ERROR "Sanitizer ignorelist not found: ${IGNORELIST_PATH}")
    endif()

    list(APPEND SANITIZERS_COMPILE_FLAGS "-fsanitize-ignorelist=${IGNORELIST_PATH}")
    message(STATUS "  Using sanitizer ignorelist: ${IGNORELIST_PATH}")

    # Join sanitizer flags with commas for -fsanitize option
    list(JOIN SANITIZERS_FLAGS "," SANITIZERS_FLAGS_STR)

    # Add sanitizer to compile and link flags
    list(APPEND SANITIZERS_COMPILE_FLAGS "-fsanitize=${SANITIZERS_FLAGS_STR}")
    set(SANITIZERS_LINK_FLAGS "-fsanitize=${SANITIZERS_FLAGS_STR}")
endif()

message(STATUS "  Compile flags: ${SANITIZERS_COMPILE_FLAGS}")
message(STATUS "  Link flags: ${SANITIZERS_LINK_FLAGS}")

# Apply the sanitizer flags to the 'common' interface library
# This is the same library used by XrplCompiler.cmake
target_compile_options(common INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:${SANITIZERS_COMPILE_FLAGS}>
    $<$<COMPILE_LANGUAGE:C>:${SANITIZERS_COMPILE_FLAGS}>
)

# Apply linker flags
target_link_options(common INTERFACE ${SANITIZERS_LINK_FLAGS})
